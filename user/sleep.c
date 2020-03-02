#include "kernel/types.h"
#include "user/user.h"

int main(int argc,char *argv[]){
    if(argc<2){
        printf("usage: %s time\n",argv[0]);    
        exit();
    }
    sleep(atoi(argv[1]));
    exit();
}
