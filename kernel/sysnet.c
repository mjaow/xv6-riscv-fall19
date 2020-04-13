//
// network system calls.
//

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"
#include "net.h"

struct sock {
  struct sock *next; // the next socket in the list
  uint32 raddr;      // the remote IPv4 address
  uint16 lport;      // the local UDP port number
  uint16 rport;      // the remote UDP port number
  struct spinlock lock; // protects the rxq
  struct mbufq rxq;  // a queue of packets waiting to be received
};

static struct spinlock lock;
static struct sock *sockets;

void
sockinit(void)
{
  initlock(&lock, "socktbl");
}

int
sockalloc(struct file **f, uint32 raddr, uint16 lport, uint16 rport)
{
  struct sock *si, *pos;

  si = 0;
  *f = 0;
  if ((*f = filealloc()) == 0)
    goto bad;
  if ((si = (struct sock*)kalloc()) == 0)
    goto bad;

  // initialize objects
  si->raddr = raddr;
  si->lport = lport;
  si->rport = rport;
  initlock(&si->lock, "sock");
  mbufq_init(&si->rxq);
  (*f)->type = FD_SOCK;
  (*f)->readable = 1;
  (*f)->writable = 1;
  (*f)->sock = si;

  // add to list of sockets
  acquire(&lock);
  pos = sockets;
  while (pos) {
    if (pos->raddr == raddr &&
        pos->lport == lport &&
	pos->rport == rport) {
      release(&lock);
      goto bad;
    }
    pos = pos->next;
  }
  si->next = sockets;
  sockets = si;
  release(&lock);
  return 0;

bad:
  if (si)
    kfree((char*)si);
  if (*f)
    fileclose(*f);
  return -1;
}

//
// Your code here.
//
// Add and wire in methods to handle closing, reading,
// and writing for network sockets.
//

void
sockclose(struct sock *s)
{
  acquire(&lock);

  struct sock *pos = sockets;
  struct sock *prev=0;

  if(pos==0){
    release(&lock);
    return;
  }

  while (pos) {
    if (pos->raddr == s->raddr &&
        pos->lport == s->lport &&
	      pos->rport == s->rport) {
      break;
    }
    prev = pos;
    pos = pos->next;
  }

  if(pos==0){
    release(&lock);
    return;
  }

  if(prev==0){
    sockets=sockets->next;
  }else{
    prev->next=prev->next->next;
  }

  release(&lock);

  acquire(&s->lock);
  while(!mbufq_empty(&s->rxq)){
    mbuffree(mbufq_pophead(&s->rxq));
  }
  release(&s->lock);

  kfree(s);
}

int
sockread(struct sock *s, uint64 addr, int n)
{
  acquire(&s->lock);
  while(mbufq_empty(&s->rxq)){
    sleep(&s->lport,&s->lock);
  }

  struct mbuf *buf=mbufq_pophead(&s->rxq);

  int len=n>buf->len?buf->len:n;

  if(copyout(myproc()->pagetable,addr,buf->head,len)==-1){
    release(&s->lock);
    mbuffree(buf);
    return -1;
  }

  release(&s->lock);
  mbuffree(buf);

  return len;
}

int
sockwrite(struct sock *s, uint64 addr, int n)
{
  unsigned int headroom=sizeof(struct udp)+sizeof(struct ip)+sizeof(struct eth);
  struct mbuf *buf=mbufalloc(headroom);

  if(buf==0){
    return -1;
  }

  mbufput(buf,n);

  if(copyin(myproc()->pagetable,buf->head,addr,n)==-1){
    mbuffree(buf);
    return -1;
  }

  net_tx_udp(buf,s->raddr,s->lport,s->rport);
  return n;
}

// called by protocol handler layer to deliver UDP packets
void
sockrecvudp(struct mbuf *m, uint32 raddr, uint16 lport, uint16 rport)
{
  //
  // Your code here.
  //
  // Find the socket that handles this mbuf and deliver it, waking
  // any sleeping reader. Free the mbuf if there are no sockets
  // registered to handle it.
  //
  acquire(&lock);

  struct sock *pos=sockets;

  while(pos){
    if(pos->raddr==raddr&&pos->lport==lport&&pos->rport==rport){
      break;
    }
    pos=pos->next;
  }

  release(&lock);
  if(pos==0){
    mbuffree(m);
    return;
  }

  acquire(&pos->lock);
  mbufq_pushtail(&pos->rxq,m);
  release(&pos->lock);

  wakeup(&pos->lport);
}
