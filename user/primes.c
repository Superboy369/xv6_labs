// my prime.c OK

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void filter(int pleft[2]){
    int current_prime;
    read(pleft[0],&current_prime,sizeof current_prime); // the first num must be prime.
    if(current_prime == -1){
        exit(0);
    }
    printf("prime %d\n",current_prime);

    int pright[2];
    pipe(pright);

    if(fork() == 0){
        // child process
        close(pleft[0]);
        close(pright[1]);
        filter(pright);
    }else{
        // father process
        close(pright[0]);
        int buf;
        // After filtering out multiples of the current prime number in the left pipe, the remaining numbers are fed into the right pipe
        while(read(pleft[0],&buf,sizeof buf) && buf != -1){ 
            if(buf % current_prime != 0){
                write(pright[1],&buf,sizeof buf);
            }
        }
        buf = -1;
        write(pright[1],&buf,sizeof buf);
        wait((int *)0);
    }
    exit(0);
}

int main(int argc,char *argv[]){
    int input_pipe[2];
    pipe(input_pipe); 

    if(fork() == 0){
        // child process
        close(input_pipe[1]);
        filter(input_pipe);
    }else{
        // father process
        close(input_pipe[0]);
        int i;
        for(i = 2;i <= 35;i ++ ){ //  put 2~35 in the input_pipe
            //printf("i = %d\n",i);
            write(input_pipe[1],&i,sizeof i);
        }
        i = -1;
        write(input_pipe[1],&i,sizeof i);
        wait((int *)0);
    }
    exit(0);
}