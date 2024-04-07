# Lab: xv6 lazy page allocation
这个*lab*要实现*page table*的*lazy allocation*。
## 思想
在用户调用`sbrk()`系统调用增加堆空间时，不立即分配实际的物理内存，将其推迟到发生*page fault*时在分配。这样做的好处是，大部分应用只会使用自己申请的所有地址空间的一部分，*lazy page alloction*可以只分配用户真正会使用的那一部分物理内存，增加了物理内存的利用率。
## 实现 (moderate)
在`sys_sbrk()`中，修改代码，如果时增加内存，则使其只是增加进程结构体的sz字段，而不进行物理内存分配，如果是减少内存，则直接抹除页表映射和*free*实际的物理页即可。

```c
uint64
sys_sbrk(void)
{
    int addr;
    int n;
    if(argint(0, &n) < 0)
        return -1;
    addr = myproc()->sz;
    if(n > 0){
        myproc()->sz += n;
    }else{
        if(myproc()->sz + n < 0){
            return -1;
        }else{
            myproc()->sz = uvmdealloc(myproc()->pagetable,myproc()->sz,myproc()->sz + n);
        }
    }
    return addr;
}
```
当需要访问本应增加的堆内存时，会发生*page fault*，进入`usertrap()`，判断这个一个*page fault*，之后判断这是否是*page lazy alloction*的场景，如果是，则进行实际的物理内存的分配，否则直接杀掉进程。前者返回用户空间继续执行运行即可。

```c
//
// handle an interrupt, exception, or system call from user space.
// called from trampoline.S
//
void
usertrap(void)
{
    int which_dev = 0;
    if((r_sstatus() & SSTATUS_SPP) != 0)
        panic("usertrap: not from user mode");
    // send interrupts and exceptions to kerneltrap(),
    // since we're now in the kernel.
    w_stvec((uint64)kernelvec);
    struct proc *p = myproc();
    // save user program counter.
    p->trapframe->epc = r_sepc();
    if(r_scause() == 8){
        // system call
        if(p->killed)
            exit(-1);
        // sepc points to the ecall instruction,
        // but we want to return to the next instruction.
        p->trapframe->epc += 4;
        // an interrupt will change sstatus &c registers,
        // so don't enable until done with those registers.
        intr_on();
        syscall();
    } else if((which_dev = devintr()) != 0){
    // ok
    } else if(r_scause() == 15 || r_scause() == 13) {
    // lab5:added
        uint64 va;
        va = r_stval();
        if(is_lazyalloc(va)){
            if(lazyalloc(va) < 0){
                p->killed = 1;
            }
        }else{
            p->killed = 1;
        }
    } else {
        printf("usertrap(): unexpected scause %p pid=%d\n", r_scause(), p->pid);
        printf(" sepc=%p stval=%p\n", r_sepc(), r_stval());
        p->killed = 1;
    }
    if(p->killed)
        exit(-1);
    // give up the CPU if this is a timer interrupt.
    if(which_dev == 2)
        yield();
    usertrapret();
}
```

```c
// lab5:added
int is_lazyalloc(uint64 va){
    pte_t *pte;
    struct proc *p = myproc();
    return va < p->sz
    && PGROUNDDOWN(va) != r_sp()
    && (((pte = walk(p->pagetable,va,0)) == 0) || ((*pte & PTE_V) == 0));
}

// lab5:added
int lazyalloc(uint64 va){
    uint64 pa;
    struct proc *p = myproc();
    if((pa = (uint64)kalloc()) == 0){
        return -1;
    }else{
        memset((void *) pa,0,PGSIZE);
        va = PGROUNDDOWN(va);
        if(mappages(p->pagetable,va,PGSIZE,pa,PTE_R | PTE_W | PTE_U | PTE_X) != 0){
            kfree((void *) pa);
            return -1;
        }
    }
    return 0;
}
```
