#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"


#define BUFSIZ 512

int getchar(void){
    static char buf[BUFSIZ]; 
    static char *bufp = buf; 
    static int n = 0;
    if(n==0){ /*bufferisempty*/ 
        n = read(0, buf, sizeof buf); 
        bufp = buf;
    }
    return (--n >= 0) ? (unsigned char) *bufp++ :'\0'; 
}

void redirect(int n,int *fds){
    close(n);
    dup(fds[n]);

    close(fds[0]);
    close(fds[1]);
}

int readline(char *c){

    char ch;
    int count=0;

    while(1){
        ch=getchar();

        if(ch=='\n'){
            count++;
            break;
        }

        if(ch=='\0'){
            break;
        }

        *c++=ch;
    }

    *c='\0';

    return count;
}

int parse(char *s,char **tokens){
    int n=0;

    while(*s=='\t'||*s==' '){
        s++;
    }

    if(*s=='\n'){
        return n;
    }

    static char buf[BUFSIZ];
    char *t=buf;
    char *c=buf;

    while(*s!='\0'){
        if(*s==' '||*s=='\t'){
            if(t>c){
                *t++='\0';
                tokens[n++]=c;     
                c=t;
            }
            s++;

        }else{
            *t++=*s++;
        }
    }

    *t='\0';

    tokens[n++]=c;

    return n;
}

int runExec(int start,int end,char **argv){
    char *pass[128];

    int i;
    for(i=start;i<=end;i++){
        pass[i-start]=argv[i];
    }

    pass[i-start]=0;

    return exec(pass[0],pass);
}

int min(int a,int b){
    if(a<b){
        return a;
    }else{
        return b;
    }
}

int runRedirect(int start,int end,char **argv){

    char *s;
    int in=0;
    int out=0;
    for(int i=start;i<=end;i++){
        s=argv[i];

        if(*s=='<'){
            in=i+1;
        }

        if(*s=='>'){
            out=i+1;
        }
    }

    if(in){
        close(0);
        if(open(argv[in],O_RDONLY)<0){
            fprintf(2, "open %s failed\n", argv[in]);
            exit(-1);
        }
        end=min(end,in-2);
    }
    
    if(out){
        unlink(argv[out]);

        close(1);

        if(open(argv[out],O_WRONLY|O_CREATE)<0){
            fprintf(2, "write %s failed\n", argv[out]);
            exit(-1);
        }
        end=min(end,out-2);
    }

    return runExec(start,end,argv);
}

int run(int start,int end,char **argv){
    if(start>end){
        return 0;
    }

    int i;
    char *s;
    for(i=start;i<=end;i++){
        s=argv[i];

        if(*s=='|'){
            break;
        }
    }

    if(i>end){
        return runRedirect(start,end,argv);
    }

    int fds[2];

    pipe(fds);

    if(fork()){
        //parent
        redirect(0,fds);

        //不可加wait(0)，避免父进程无法读取子进程的输出，导致子进程block
        run(i+1,end,argv);
    }else{
        //child
        redirect(1,fds);
            
        runRedirect(start,i-1,argv);
    }

    return 0;
}

int loop(){
    
    char line[128];
    char *tokens[128];
    int n;

    while(1){
        printf("@ ");

        if(!readline(line)){
            return 0;
        }

        n=parse(line,tokens);

        //TODO 不开进程会导致循环无法继续
        if(fork()){
            wait(0);
        }else{
            run(0,n-1,tokens);
        }
    }
}

int main(int args,char **argv){
    loop();
    exit(0);
}

