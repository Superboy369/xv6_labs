// my pingpong.c OK

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc,char *argv[]){
    if(argc != 1){
        fprintf(2, "Ussage: pingpong...");
    }

    int p1[2]; // father->child
    int p2[2]; // child->father
    if(pipe(p1) < 0){
        fprintf(2, "Usage: pipe error.");
        exit(1);
    }
    if(pipe(p2) < 0){
        fprintf(2, "Usage: pipe error.");
        exit(1);
    }

    if(fork() == 0){ // child process
        char buf;
        read(p1[1],&buf,1);
        printf("%d: received ping\n",getpid());
        write(p2[0],&buf,1);
    }else{  // father process
        write(p1[0],"!",1);
        char buf;
        read(p2[1],&buf,1);
        printf("%d: received pong\n",getpid());
        wait((int *)0);
    }

    exit(0);
}