# Lab: system calls
这个实验需要在在系统中添加两个新的系统调用，一个*trace*，一个*userinfo*，重点在于理解系统调用在内核中的执行流程。
## System call tracing (moderate)
- 实现一个新的系统调用——*trace*，这个系统调用给xv6增添了系统调用跟踪功能，调用*trace*的进程及其子进程在之后的进行系统调用时，内核都会打印处一条信息包含进程id、系统调用的名称和返回值。
     
要实现首先需要理解系统调用在用户和内核中的执行流程。
### 系统调用执行流程

![xv6系统调用流程.drawio.png](https://p6-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/c38801b284eb4c8da36e743790e32700~tplv-k3u1fbpfcp-jj-mark:0:0:0:0:q75.image#?w=631&h=601&s=45744&e=png&a=1&b=f6cccb)

以系统调用fork为例：
- 用户程序调用跳板函数`fork()`
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
汇编代码主要就是设置了*SYS_fork*的系统调用编号，并且调用`ecall`，执行`ecall`指令，硬件会关中断、将用户态转化为内核态、将*pc*保存至*sepc*，并跳转至*pc*     
- 跳转到*trampoline.S*中的*uservec*函数内嵌的汇编代码首地址处执行保存用户寄存器和恢复内核寄存器的汇编代码，*trampoline*最后调用`usertrap()`（具体*trampoline*，包括*trapframe*后面的*lab*才会用到，这里只是简要说明下）
- `uesrtrap()`判断这是由于系统调用而导致的内陷，进而执行`syscall()`
- `syscall()`根据保存的系统调用参数调用对应的`sys_fork()`内核系统函数
- `sys_fork()`则会调用真正的内核系统调用函数，进行*fork*真正的工作，如复制父进程地址空间、*trapframe*、文件描述符等等，并设置子进程的内核结构体的字段。
- 执行完`sys_fork()`，返回`syscall()`，返回`usertrap()`，`usertrap()`继续调用`usertrapret()`
- `usertrapret()`执行跳转到*trampoline.S*中的*userret*函数中
- *userret*恢复用户态寄存器，最后执行`sret`指令
- `sret`与`ecall`正好相反，开中断、切状态为用户态、从*sepc*中恢复*pc*，之后跳转至*pc*，继续执行用户程序

理解了系统调用流程`trace()`系统调用就不难实现了，在每个内核进程结构体中增加一个*mask*字段，由于所有系统调用都会执行`syscall()`，故在`syscall()`中设置埋点，当任何系统调用到达`syscall()`后根据*mask*和系统调用号判断当前的是否在*trace*，并输出Lab指定信息即可。
## Sysinfo (moderate)
这个系统调用提供了一个指向当前用户地址空间中的结构体的指针，我们需要从内核里面统计并填充里面的字段，换句话说，就是要将内核中的数据传入用户地址空间内。
- 我们从上面的例子可以知道，*sys_info*的执行是在内核中，在未修改的*xv6*中，内核线程无法直接访问用户地址，因为未修改的*xv6*中内核页表中没有用户地址空间的数据，硬件无法直接进行虚实地址的转换，我们需要使用`copyout()`将内核的数据传入指定页表的指定地址中。
- `copyout()`是xv6内核提供的将内核数据传入指定地址空间的函数（包括用户地址空间），里面调用了内核提供的`walkaddr()`使用软件来根据传入的页表完成虚实地址的转换。

