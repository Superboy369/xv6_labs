# Lab Page tables
这个实验由三部分组成，打印指定页表、给每个进程的内核增添内核页表、使用添加的进程内核页表简化`copyin()`和`copyinstr()`两个函数。
## Print a page table (easy)
这个部分是实现一个`vmprint()`的函数，他打印指定页表的*PTE*内容。
### 具体实现
实现的思路是使用递归*dfs*，递归函数的含义是遍历打印当前页表中的所有有效的页表项，递归的顺序是按照三级页表的树状顺序进行深度优先遍历，如果当前页表项是有效的，会继续调用递归函数。

```c
// lab_3:page tables add the print page tables function
void vmprint(pagetable_t pagetable){
    printf("page table %p\n",pagetable);
    pgtlbprint(pagetable,0);
}
// lab_3:page tables add the print page tables function
void pgtlbprint(pagetable_t pagetable,uint64 deepth){
    // there are 2^9 = 512 PTEs in a page table.
    for(int i = 0; i < 512; i++){
        pte_t pte = pagetable[i];
        if((pte & PTE_V)){
            switch(deepth){
                case 0:
                printf("..%d: pte %p pa %p\n",i,pte,PTE2PA(pte));
                break;
                case 1:
                printf(".. ..%d: pte %p pa %p\n",i,pte,PTE2PA(pte));
                break;
                case 2:
                printf(".. .. ..%d: pte %p pa %p\n",i,pte,PTE2PA(pte));
                break;
                default:;
            }
            if((pte & (PTE_R | PTE_W | PTE_X)) == 0){ // if is not leaf of the page tables
                // this PTE points to a lower-level page table.
                uint64 child = PTE2PA(pte);
                pgtlbprint((pagetable_t)child,deepth + 1);
            }
        }
    }
}
```
## A kernel page table per process (hard)
- 这个部分是给每个进程都添加一个内核页表。
- 在未修改的*xv6*中，每个进程都有一份自己的用户地址空间页表，而没有自己的内核地址空间页表，因为所有进程共用一份在内核中的内核地址空间页表，这个内核页表没有其他进程用户地址空间的映射信息。当内核需要访问用户地址空间时，会调用`walkaddr()`来借助软件来模拟虚实地址的转换。这样实现会有性能问题，在其他条件相同的情况下，硬件的速度显然会比软件快，由于内核页表中没有用户地址空间的数据，故操作系统无法通过硬件进行转换，降低了操作系统的性能。
- 为了优化这个问题，我们可以给每个进程都添加一个自己的内核页表，而非共享，并且将用户地址空间页表的映射信息也加入到这个自己内核页表中，当进程由用户态切换至内核态时，将页表基址寄存器改为这个自己的内核页表，而非改为共享的内核页表。这个当进程在内核中时，内核地址空间和用户地址空间内核都可以直接利用硬件进行虚实地址转换、访问到。   
### 具体实现   
1. 具体实现是在内核代码中进程结构体中添加自己的内核页表字段，在调用`allocproc()`函数分配进程时，`kalloc()`分配页表物理页，初始化设置这个字段，并将内核地址空间初始化映射到自己的内核页表中，并`kalloc()`分配一页物理内存内核栈。在调用`freeproc()`函数释放进程时，先根据自己的内核页表转化虚拟地址，将内核分配给内核栈的一页物理内存释放，再将这个自己的内核页表释放，改回字段为0。
1. 并且还要在每个*cpu*一个的内核调度器线程调用调度器`scheduler()`函数中，在`swtch()`前后更改页表基址寄存器内容为选中进程的自己的内核页表基址，以使用或者恢复要上处理机运行的进程的内核页表。
## Simplify `copyin/copyinstr` (hard)
这个部分要求更改`copyin()/copyinstr()`，让他们可以使用前一部分添加的进程自己的内核页表，让其使用硬件进行地址转换。
- `copyin()/copyinstr()`这两个函数是将用户地址空间的数据传入内核中。原本的`copyin()/copyinstr()`使用调用`walkaddr()`软件进行传入的用户空间地址的转换。
- `userinit()`、`fork()`、`exec()`和`growproc()`中，都修改了用户页表，因此也需要将用户地址空间映射信息添加到进程自己的内核页表中。
