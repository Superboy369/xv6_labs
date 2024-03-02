# Lab System calls
这个实验需要在在系统中添加两个新的系统调用，一个trace，一个userinfo，重点在于理解系统调用在内核中的执行流程。
## System call tracing
实现一个新的系统调用，首先需要理解系统调用在用户和内核中的执行流程。
### 系统调用执行流程

![xv6系统调用流程.drawio.png](https://p6-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/c38801b284eb4c8da36e743790e32700~tplv-k3u1fbpfcp-jj-mark:0:0:0:0:q75.image#?w=631&h=601&s=45744&e=png&a=1&b=f6cccb)

以系统调用fork为例：
- 用户程序调用跳板函数fork()
- 跳转到`user/usys.pl`脚本文件中内嵌的汇编代码
```c
sub entry {
    my $name = shift;
    print ".global $name\n";
    print "${name}:\n";
    print " li a7, SYS_${name}\n";
    print " ecall\n";
    print " ret\n";
}
```
汇编代码主要就是设置了SYS_fork的系统调用编号，并且调用ecall，执行ecall指令，硬件会关中断、将用户态转化为内核态、将pc保存至sepc，并跳转至pc     
- 跳转到trampoline.S中的uservec函数内嵌的汇编代码首地址处执行保存用户寄存器和恢复内核寄存器的汇编代码，trampoline最后调用usertrap()（具体trampoline，包括trapframe后面的Lab才会用到，这里只是简要说明下）
- uesrtrap()判断这是由于系统调用而导致的内陷，进而执行syscall()
- syscall()根据保存的系统调用参数调用对应的sys_fork()内核系统函数
- sys_fork()则会调用真正的内核系统调用函数，进行fork真正的工作，如复制父进程地址空间、trapframe、文件描述符等等，并设置子进程的内核结构体的字段。
- 执行完sys_fork()，返回syscall()，返回usertrap()，usertrap()继续调用usertrapret()
- usertrapret()执行跳转到trampoline.S中的userret函数中
- userret恢复用户态寄存器，最后执行sret指令
- sret与ecall正好相反，开中断、切状态为用户态、从sepc中恢复pc，之后跳转至pc，继续执行用户程序

理解了系统调用流程trace()系统调用就不难实现了，在每个内核进程结构体中增加一个mask字段，由于所有系统调用都会执行syscall()，故在syscall()中设置埋点，当任何系统调用到达syscall()后根据mask和系统调用号判断当前的是否在trace，并输出Lab指定信息即可。
## Sysinfo
这个系统调用提供了一个指向当前用户地址空间中的结构体的指针，我们需要从内核里面统计并填充里面的字段，换句话说，就是要将内核中的数据传入用户地址空间内。
- 我们从上面的例子可以知道，sys_info的执行是在内核中，在未修改的XV6中，内核线程无法直接访问用户地址，因为未修改的xv6中内核页表中没有用户地址空间的数据，硬件无法直接进行虚实地址的转换，我们需要使用copyout()将内核的数据传入指定页表的指定地址中。
- copyout()是xv6内核提供的将内核数据传入指定地址空间的函数（包括用户地址空间），里面调用了内核提供的walkaddr()使用软件来根据传入的页表完成虚实地址的转换。

