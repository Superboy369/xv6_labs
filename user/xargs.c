// my xargs.c FAIL

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char *argv[]) {
    //从标准输入读取数据
    char stdIn[512];
    int size = read(0, stdIn, sizeof stdIn);
    //将数据分行存储
    int i = 0, j = 0;
    int line = 0;
    for (int k = 0; k < size; ++k) {
        if (stdIn[k] == '\n') { // 根据换行符的个数统计数据的行数
            ++line;
        }
    }
    char output[line][64]; // 根据提示中的MAXARG，命令参数长度最长为32个字节
    for (int k = 0; k < size; ++k) {
        output[i][j++] = stdIn[k];
        if (stdIn[k] == '\n') {
            output[i][j - 1] = 0; // 用0覆盖掉换行符。C语言没有字符串类型，char类型的数组中，'0'表示字符串的结束
            ++i; // 继续保存下一行数据
            j = 0;
        }
    }
    //将数据分行拼接到argv[2]后，并运行
    char *arguments[MAXARG];
    for (j = 0; j < argc - 1; ++j) {
        arguments[j] = argv[1 + j]; // 从argv[1]开始，保存原本的命令+命令参数
    }
    i = 0;
    while (i < line) {
        arguments[j] = output[i++]; // 将每一行数据都分别拼接在原命令参数后
        if (fork() == 0) {
            exec(argv[1], arguments);
            exit(0);
        }
        wait(0);
    }
    exit(0);
}