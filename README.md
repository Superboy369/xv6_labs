# Lab: traps
这个实验有三部分，第一部分为问答，第二部分为*backtrace*，第三部分为实现用户级中断/故障处理程序的一种初级形式。
## RISC-V assembly (easy)
这个部分目的是要理解*RISC-V*指令集架构的汇编代码。
- RISC-V采用了一种混合的寄存器和栈的传递方式。前八个参数会通过寄存器`a0`~`a7`传递，而后续的参数则会通过栈来传递。
- 编译器可能会将函数内联，将函数调用处的函数体代码直接插入到调用该函数的地方，而不是通过函数调用的方式进行执行，在函数内联汇编代码中，可能会没有函数调用。
- *RISC-V*进行函数调用，执行完函数跳转指令之后，`ra`寄存器中存储的是函数返回地址。
## Backtrace (moderate)
这个函数就是实现曾经调用函数地址的回溯，只用打印程序地址。
- 返回地址位于栈帧帧指针的固定偏移(-8)位置，并且保存的帧指针位于帧指针的固定偏移(-16)位置，栈的增长方向为地址减小的方向。

![p2.png](https://p1-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/61d7365448f149639545b21b4bfcbad9~tplv-k3u1fbpfcp-jj-mark:0:0:0:0:q75.image#?w=1087&h=752&s=169785&e=png&b=fefefe)

```c
// lab4:traps added
void backtrace(){
    uint64 fp = r_fp();
    while(fp != PGROUNDUP(fp)){
        uint64 ra = *(uint64*)(fp - 8);
        printf("%p\n",ra);
        fp = *(uint64*)(fp - 16);
    }
}
```
## Alarm (Hard)
实现用户级中断/故障处理程序的一种初级形式，实现`sigalarm()`系统调用。
- `sigalarm(interval, handler)`系统调用，如果一个程序调用了`sigalarm(n, fn)`，那么每当程序消耗了*CPU*时间达到n个“滴答”，内核应当使应用程序函数`fn`被调用。当`fn`返回时，应用应当在它离开的地方恢复执行。
- 程序计数器的过程是这样的：

1.  `ecall`指令中将*PC*保存到*SEPC*
1.  在`usertrap`中将*SEPC*保存到`p->trapframe->epc`
1.  `p->trapframe->epc`加4指向下一条指令
1.  执行系统调用
1.  在`usertrapret`中将*SEPC*改写为`p->trapframe->epc`中的值
1.  在`sret`中将*PC*设置为*SEPC*的值     
可见执行系统调用后返回到用户空间继续执行的指令地址是由`p->trapframe->epc`决定的，因此在`usertrap`中主要就是完成它的设置工作。
- 在前面的*syscall lab*中我们得知了所有用户的系统调用、时钟中断和异常都会经过`trampoline.S userve`->`usertrap()`函数->`usertrapret`->`trampoline.S userret`这个一系列，而`trampoline.S`中的`uservec`和`userret`都是保存和恢复用户寄存器到内核结构体，原因是操作系统要将用户寄存器状态保存下来，防止在内核代码执行后，无法回到原用户的中断程序。
- 考虑一下没有*alarm*时运行的大致过程
1.  进入内核空间，保存用户寄存器到进程陷阱帧
1.  陷阱处理过程
1.  恢复用户寄存器，返回用户空间   
- 而当添加了*alarm*后，变成了以下过程
1.  进入内核空间，保存用户寄存器到进程陷阱帧
1.  陷阱处理过程
1.  恢复用户寄存器，返回用户空间，但此时返回的并不是进入陷阱时的程序地址，而是处理函数`handler`的地址，而`handler`可能会改变用户寄存器                 
因此我们要在`usertrap`中再次保存用户寄存器
- 内核中断处理程序的保存和恢复借助于进程结构体中的*trapframe*，在这个部分中，我们要在进程结构体中添加一个新的*trapframe*用来再保存一次用户寄存器。在内核返回用户执行`handler`的之前，将原*trapframe*存入新*trapframe*中，并将`trapframe->epc`置为`handler`，在`handler`执行完返回之前要将新的*trapframe*恢复到原*trapframe*中。这样内核返回用户空间执行的函数是`handler`，并且也不会破坏最初的在*trampframe*中存的用户寄存器。
