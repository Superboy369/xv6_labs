// my sleep.c OK

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc,char *argv[]){
    if(argc < 2){
        fprintf(2, "Usage: sleep...\n");
        exit(1);
    }

    int sleep_time = atoi(argv[1]);
    if(sleep(sleep_time) < 0){
        fprintf(2, "sleep: failed to sleep\n");
    }
    
    exit(0);
}