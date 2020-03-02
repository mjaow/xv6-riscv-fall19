#include<kernel/types.h>
#include<user/user.h>

int main(int args,char *argv[]){
    char *argExec[32];
    char M[32][32];
    for(int i=0;i<32;i++){
        argExec[i]=M[i];
    }

    for(int i=1;i<args;i++){
        strcpy(argExec[i-1],argv[i]);
    }

    char buf[64];
    char *p;
    p=buf;

    int i=args-1;
    while(read(0,p,1)>0){
        if(*p=='\n'){
            *p='\0';

            if(strlen(buf)>0){
                strcpy(argExec[i++],buf);
            }

            argExec[i]=0;
            i=args-1;
            p=buf;

            if(fork()){
                wait();
            }else{
                exec(argExec[0],argExec);
            }
        }else{
            p++;
        }
    }

    exit();
}
