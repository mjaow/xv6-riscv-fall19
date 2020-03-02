#include "kernel/types.h"
#include "user/user.h"

int main(int argc,char *argv){

    int pid;
    int parentFds[2];
    int childFds[2];
    char buf[100];

    pipe(parentFds);
    pipe(childFds);

    if((pid=fork())==0){
        //child
        //receive ping from parent
        read(parentFds[0],buf,sizeof(buf));

        printf("%d: received %s\n",getpid(),buf);

        //send pong to parent
        write(childFds[1],"pong",4);
    }else{
        //parent
        //send ping to child
        write(parentFds[1],"ping",4);

        //receive pong from child
        read(childFds[0],buf,sizeof(buf));

        printf("%d: received %s\n",getpid(),buf);
    }

    exit();
}
