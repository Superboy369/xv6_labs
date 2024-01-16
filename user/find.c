// my find.c OK

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

// gen ju mu lu jie gou di gui,
// han shu yi yi shi cha zhao path luo jing xia de suo you wen jian shi fou 
// he lost_file wen jian tong ming 
void find(char *path,char *lost_file){
    char buf[128],*p;
    int fd,fd1;
    struct dirent de;
    struct stat st,st1;

    if((fd = open(path,0)) < 0){
        fprintf(2, " find:cannot open %s\n",path);
        return;
    }

    if(fstat(fd,&st) < 0){
        fprintf(2, " find:path stat failed...\n");
        close(fd);
        return;
    }

    switch (st.type){
        case T_FILE:
            fprintf(2," path error\n");
            return;
        case T_DIR:
            strcpy(buf,path);
            p = buf + strlen(buf);
            *p ++ = '/';
            while(read(fd, &de, sizeof(de)) == sizeof(de)){
                if(de.inum == 0){
                    continue;
                }
                if(!strcmp(de.name,".") || !strcmp(de.name,"..")){
                    continue;
                }
                memmove(p, de.name, DIRSIZ);
                if((fd1 = open(buf, 0)) >= 0){
                    if(fstat(fd1,&st1) >= 0){
                        switch(st1.type){
                            case T_FILE:
                                if(!strcmp(de.name,lost_file)){
                                    printf("%s\n",buf);
                                }
                                close(fd1);
                                break;
                            case T_DIR:
                                close(fd1);
                                find(buf,lost_file);
                                break;
                            case T_DEVICE:
                                close(fd1);
                                break;
                        }
                    }
                }
            }
            break;
    }
    close(fd);
}

int main(int argc,char *argv[]){
    if(argc != 3){
        fprintf(2, "Ussage:failed to find...\n");
        exit(1);
    }

    find(argv[1],argv[2]);  // di gui

    exit(0);
}