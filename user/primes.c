#include<kernel/types.h>
#include<user/user.h>

void redirect(int n,int *fds){
    close(n);
    dup(fds[n]);
    
    close(fds[0]);
    close(fds[1]);
}

void cull(int n){
    int val;
    while(read(0,&val,sizeof(val))){
        if(val%n!=0){
            write(1,&val,sizeof(val));
        }
    }
}

void sink(){
    int val;
    int fds[2];

    while(read(0,&val,sizeof(val))){
        printf("prime %d\n",val);

        pipe(fds);

        if(fork()){
            //parent
            redirect(0,fds);
        }else{
            //child
            redirect(1,fds);
            
            cull(val);
        }
    }
}

void source(){
    for(int i=2;i<=35;i++){
        write(1,&i,sizeof(i));
    }
}

//reference: 
//1. https://www.cs.dartmouth.edu/~doug/sieve/sieve.pdf
//2. https://swtch.com/~rsc/thread
int main(int args,char *argv){
    int fds[2];

    pipe(fds);

    //note: dataflow from child -> parent , not parent -> child
    // ensure that parent read and child write, so that parent will exit after read all of primes
    if(fork()){
        //parent
        //redict input of pipelines to std in
        redirect(0,fds);
        sink();
    }else{
        //child
        //redict output of pipelines to std out
        redirect(1,fds);
        source();
    }

    exit();
}
