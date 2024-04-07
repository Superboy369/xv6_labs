# Lab6:Copy-on-Write Fork for xv6
这个*lab*是要*xv6*增添新特性*COW fork()*。
## 为何增添
在未修改的*xv6*中，`fork()`系统调用会复制父进程的页表和对应的物理页给子进程，如果父进程地址空间很大，那么`fork()`系统调用会耗费很长的时间进行地址空间的复制。更糟糕的是，大部分的进程在调用系统`fork()`之后会紧接着调用`exec()`，`exec()`会丢弃之前所有`fork()`复制的地址空间，相当于之前的`fork()`复制是无效的。
## COW fork() (hard)
我们可以推迟`fork()`复制实际的物理页到发生*page fault*时。在`fork()`复制父进程地址空间时，只是复制页表，父子进程的页表都指向相同的物理页，并且将他们的可写页面都改为只读，当父子进程中的任意一个在修改这个修改为只读的页面时，就会触发*page fault*，在*page fault*处理中判断这个是否是*COW*场景下的*page fault*，如果是的话就复制实际的物理页到对应的进程中，并且修改父子进程的页面为可写。这样做可以加快`fork()`系统调用的速度，提高物理内存的使用效率。
## 细节
### COW场景下的page fault的判定
可以使用*xv6*的*PTE*中预留的的*RSW(reserved for software)* 位来标识*COW*场景。在`fork()`复制父进程地址空间到子进程时，在修改可写页面为只读的时候，设置*PTE*中的*PTE_COW*位来标识场景。在发生*page fault*依照*PTE_COW*位来判断。
```c
int cowfault(pagetable_t pagetable,uint64 va){
    if(va >= MAXVA){
        return -1;
    }
    pte_t *pte;
    uint64 pa1,pa2;
    if((pte = walk(pagetable,va,0)) == 0){
        return -1;
    }
    if(!(*pte & PTE_U) || !(*pte & PTE_V)){
        return -1;
    }
    if((*pte & PTE_COW) == 0){
        return -1;
    }
    pa1 = PTE2PA(*pte);
    if((pa2 = (uint64)kalloc()) == 0){
        return -1;
    }
    memmove((void *)pa2,(void *)pa1,PGSIZE);
    *pte = PA2PTE(pa2) | PTE_V | PTE_U | PTE_W | PTE_R | PTE_X;
    kfree((void *)pa1);
    return 0;
}
```
### 物理页的引用计数
需要给每个物理页都维护一个引用计数，在调用`kalloc()`分配物理页面时将引用计数初始化为1，在`fork()`复制地址空间时将以用计数+1，在`kfree()`释放物理页之前先将引用计数-1，如果为0，则释放物理页，否则什么都不做，在`freerange()`中也需要将引用计数置为1。对于这个内核的数据——引用计数，所有进程都共用同一个，故需要加锁访问保证临界资源访问的互斥。
```c
int uvmcopy(pagetable_t old, pagetable_t new, uint64 sz)
{
    pte_t *pte;
    uint64 pa, i;
    uint flags;
    // char *mem;
    for(i = 0; i < sz; i += PGSIZE){
        if((pte = walk(old, i, 0)) == 0)
            panic("uvmcopy: pte should exist");
        if((*pte & PTE_V) == 0)
            panic("uvmcopy: page not present");
        pa = PTE2PA(*pte);
        flags = PTE_FLAGS(*pte);
        // if((mem = kalloc()) == 0)
            // goto err;
        // memmove(mem, (char*)pa, PGSIZE);
        if((*pte & PTE_W)){
            *pte |= PTE_COW;
            flags |= PTE_COW;
            *pte &= ~PTE_W;
            flags &= ~PTE_W;
        }
        if(mappages(new, i, PGSIZE, (uint64)pa, flags) != 0){
            // kfree(mem);
            goto err;
        }
        incref(pa);
    }
    return 0;
err:
    uvmunmap(new, 0, i / PGSIZE, 1);
    return -1;
}
```
```c
void kfree(void *pa)
{
    struct run *r;
    if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
        panic("kfree");
    acquire(&refc.lock);
    if(--refc.refcount[(uint64)pa / PGSIZE] != 0){
        release(&refc.lock);
        return;
    }
    release(&refc.lock);
    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);
    r = (struct run*)pa;
    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
}
```
```c
struct {
    struct spinlock lock;
    int refcount[PHYSTOP / PGSIZE];
}refc;
```
