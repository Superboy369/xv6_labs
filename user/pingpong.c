// my pingpong.c OK

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc,char *argv[]){
    if(argc != 1){
        fprintf(2, "Ussage: pingpong...");
    }

    int p1[2]; // father->child pipe
    int p2[2]; // child->father pipe
    if(pipe(p1) < 0){
        fprintf(2, "Usage: pipe error.");
        exit(1);
    }
    if(pipe(p2) < 0){
        fprintf(2, "Usage: pipe error.");
        exit(1);
    }

    if(fork() == 0){ // child process
        close(p1[1]);
        close(p2[0]);
        char buf;
        read(p1[0],&buf,1);  // 2.child jie shou dao father chuan song de yi ge zi jie
        printf("%d: received ping\n",getpid());
        write(p2[1],&buf,1); // 3.child zai jia zhe ge zi jie chuan song gei father
    }else{  // father process
        close(p1[0]);
        close(p2[1]);
        write(p1[1],"!",1);  // 1.father chuan song yi ge zi jie gei child
        char buf;
        read(p2[0],&buf,1);
        printf("%d: received pong\n",getpid()); // 4.father jie shou dao child chuan song hui lai de zi jie
        wait((int *)0);
    }

    exit(0);
}