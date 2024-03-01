# Lab Utilities
- 这个实验中要求实现的是五个shell命令，其实就是编写5个c语言的可执行文件，要完成指定的工作，分别为sleep、pingpong、primes、find、xargs。
- 这个实验目的是理解具体的shell命令的原理，理解shell命令与系统调用之间的关系，并且获取一些编写应用程序的经验。
## sleep
这个程序编写很简单，就是获取传给main函数的参数，并且调用系统调用sleep()。
## pingpong
pingpong实现中，父进程fork()创建子进程，父子进程通过定义的pipe管道传输数据，父进程需要等待子进程的退出。
## primes
这个部分比较有意思，使用多进程和多管道实现了筛法求质数。   
- 当前进程要从左边的管道中拿出数字筛掉当前第一个数(质数)的倍数之后放入右管道，而其子进程则传递管道参数继续重复。
- 使用多进程和管道，每一个进程作为一个 stage，筛掉某个素数的所有倍数（筛法）。
- 每一个 stage 以当前数集中最小的数字作为素数输出（每个 stage 中数集中最小的数一定是一个素数，因为它没有被任何比它小的数筛掉），并筛掉输入中该素数的所有倍数（必然不是素数），然后将剩下的数传递给下一 stage。最后会形成一条子进程链，而由于每一个进程都调用了 `wait(0);` 等待其子进程，所以会在最末端也就是最后一个 stage 完成的时候，沿着链条向上依次退出各个进程。
- 这个实现的坑在于需要关闭不需要的文件描述符，否则会耗尽xv6的文件描述符。
fork 会将父进程的所有文件描述符都复制到子进程里，而 xv6 每个进程能打开的文件描述符总数只有 16 个 。内核文件`\kernel\param.h`中定义了每个进程所能打开的文件最大为16。由于一个管道会同时打开一个输入文件和一个输出文件，所以一个管道就占用了 2 个文件描述符，并且复制的子进程还会复制父进程的描述符，于是跑到第六七层后，就会由于最末端的子进程出现 16 个文件描述符都被占满的情况，导致新管道创建失败。

```c
#define NOFILE 16 // open files per process
```
## find
find实现的是查找指定目录下的指定文件。      
采用递归的方式，递归函数的目的是遍历当前目录文件的所有文件查找指定文件，如果为普通文件则看其名字是否与指定find的文件相同，否则为目录文件则递归的调用函数。

## xargs
实现了类linux的xargs命令，为命令增加调用参数。实现是将标准输入中的字符串按照行来读入依次添加到定义的argument数组中，并调用`exec(argv[1],argument)`系统调用来执行argv\[1\]中原本指定的程序。
