# Lab:Multithreading
这个*lab*分为三个部分：第一部分是实现用户级线程的创建、切换，第二部分是使用锁实现线程间的互斥，第三部分是使用锁和条件变量实现线程间的同步。
## Uthread: switching between threads (moderate）
这个部分是要实现用户级线程的创建以及切换。需要修改`thread_create()`、`thread_schedule()`和汇编语言源程序`thread_switch`。             
类似于内核进行进程切换时需要保存上下文一样，要实现用户级线程切换，需要每个线程增加位置来保存每个线程的上下文，在每次切换时，将当前线程的上下文保存至当前进程的结构体中，将下个切换的线程的上下文恢复。 
### 上下文应该是什么
和内核进行进程切换时保存的上下文*context*一样，这个保存的上下文内容应该是寄存器*ra(return address)*
以及一些包括*sp(stack point)*
在内的*callee-save registers*。原因是进程切换和线程切换都是在相同的状态下的转换，*caller-save registers*已经在当前进程调用`sched()`函数或者线程调用`thread_scheduler()`函数时保存下来了，而由于进程或者线程切换*callee-save registers*被调用者都还未来得及保存。

![image.png](https://p6-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/ee2d4f30264748acad28a13ec2d81868~tplv-k3u1fbpfcp-jj-mark:0:0:0:0:q75.image#?w=931&h=767&s=191133&e=png&b=fefdfd)
从上图可以看到寄存器*sp*和*s0~s11*都是*callee_save registers*，而*ra*却是*caller-save registers*，这个是特例，寄存器*ra*也需要归为上下文内容保存，因为在*risc-v*中*ra*虽然为*caller-save registers*，但其却是在被调用者中压入到被调用者的栈帧中的。
```asm
{
    80001176:	1141                	addi	sp,sp,-16
    80001178:	e406                	sd	ra,8(sp)  // 将ra压入被调用者栈帧中
    8000117a:	e022                	sd	s0,0(sp)
    8000117c:	0800                	addi	s0,sp,16
    8000117e:	8736                	mv	a4,a3
```
```asm
  if(mappages(kernel_pagetable, va, sz, pa, perm) != 0)
    80001180:	86ae                	mv	a3,a1
    80001182:	85aa                	mv	a1,a0
    80001184:	00008517          	auipc	a0,0x8
    80001188:	e8c53503          	ld	a0,-372(a0) # 80009010 <kernel_pagetable>
    8000118c:	00000097          	auipc	ra,0x0  // 保存当前地址至ra
    80001190:	f5c080e7          	jalr	-164(ra) # 800010e8 <mappages>  // 根据ra进行pc跳转
    80001194:	e509                	bnez	a0,8000119e <kvmmap+0x28>
```
```asm
}
    80001196:	60a2                	ld	ra,8(sp)  // 从被调用者栈帧中恢复ra
    80001198:	6402                	ld	s0,0(sp)
    8000119a:	0141                	addi	sp,sp,16
    8000119c:	8082                	ret
```
从上面的*risc-v*汇编指令中可以看出在被调用者执行`{`和`}`对应的汇编指令时将*ra*保存了下来（压栈），即使其为*caller-save register*。
```c
// lab7: the first part of multithreading added
struct tcontext{
    uint64 ra;
    uint64 sp;
    // callee-saved
    uint64 s0;
    uint64 s1;
    uint64 s2;
    uint64 s3;
    uint64 s4;
    uint64 s5;
    uint64 s6;
    uint64 s7;
    uint64 s8;
    uint64 s9;
    uint64 s10;
    uint64 s11;
};
```
### thread_create()
这个函数是进行用户级线程的创建。
```c
struct thread {
    char stack[STACK_SIZE]; /* the thread's stack */
    int state; /* FREE, RUNNING, RUNNABLE */
    struct tcontext context; // lab7: the first part of multithreading added
};
```
在线程结构体中增加自己线程的上下文保存的字段。
```c
void
thread_create(void (*func)())
{
    struct thread *t;
    for (t = all_thread; t < all_thread + MAX_THREAD; t++) {
        if (t->state == FREE) break;
    }
    t->state = RUNNABLE;
    // YOUR CODE HERE
    t->context.ra = (uint64)func;
    t->context.sp = (uint64)t->stack + STACK_SIZE;
}
```
需要将上下文中的*ra*和*sp*修改，以便在当前线程被切换时，能正确调用指定函数*func*和使用对应的栈。
### thread_schedule()
```c
void
thread_schedule(void)
{
    struct thread *t, *next_thread;
    /* Find another runnable thread. */
    next_thread = 0;
    t = current_thread + 1;
    for(int i = 0; i < MAX_THREAD; i++){
        if(t >= all_thread + MAX_THREAD)
            t = all_thread;
        if(t->state == RUNNABLE) {
            next_thread = t;
            break;
        }
        t = t + 1;
    }
    if (next_thread == 0) {
        printf("thread_schedule: no runnable threads\n");
        exit(-1);
    }
    if (current_thread != next_thread) { /* switch threads? */
        next_thread->state = RUNNING;
        t = current_thread;
        current_thread = next_thread;
        /* YOUR CODE HERE
        * Invoke thread_switch to switch from t to next_thread:
        * thread_switch(??, ??);
        */
        thread_switch((uint64)&t->context,(uint64)&current_thread->context);
    } else
        next_thread = 0;
}
```
### thread_switch
```asm
thread_switch:
	/* YOUR CODE HERE */
	sd ra, 0(a0)
	sd sp, 8(a0)
	sd s0, 16(a0)
	sd s1, 24(a0)
	sd s2, 32(a0)
	sd s3, 40(a0)
	sd s4, 48(a0)
	sd s5, 56(a0)
	sd s6, 64(a0)
	sd s7, 72(a0)
	sd s8, 80(a0)
	sd s9, 88(a0)
	sd s10, 96(a0)
	sd s11, 104(a0)
        
	ld ra, 0(a1)
	ld sp, 8(a1)
	ld s0, 16(a1)
	ld s1, 24(a1)
	ld s2, 32(a1)
	ld s3, 40(a1)
	ld s4, 48(a1)
	ld s5, 56(a1)
	ld s6, 64(a1)
	ld s7, 72(a1)
	ld s8, 80(a1)
	ld s9, 88(a1)
	ld s10, 96(a1)
	ld s11, 104(a1)

	ret    /* return to ra */
```
## Using threads (moderate)
这个部分是实现线程间的互斥，要修改*ph.c*源文件，使用c语言的*pthread*线程库加速*ph.c*文件中哈希表的插入和查找的速度，并且添加同步互斥机制保证线程安全，保证不会出现插入键的丢失。这里*ph.c*中的哈希表是通过拉链法实现的，每个键值对应的链都是一个哈希桶。
### 实现资源互斥
根据*lab*的*hints*，这里要使用*pthread*线程库中的`pthread_mutex_t lock`线程锁来保证临界资源`struct entry *table[NBUCKET]`哈希表的互斥访问。我们当然可以这一整个哈希表一个大锁来实现，但是这样线程就起不到加速*ph.c*文件执行的效果了，相当于还是单线程。我们要减小锁的粒度，这就很容易想到将锁缩小到每个哈希桶一个锁，因为每个哈希桶之间是互不包含的。
```c
// lab7: the second part of the multithreading added
pthread_mutex_t lock[NBUCKET];
```
我们只需要在*main()*函数中初始化*lock[]*，在*put()* 和 *get()* 函数中获取对应键值哈希桶的锁即可。
## Barrier(moderate)
这个部分是要实现线程间的同步，使用了*pthread*线程库中的`pthread_cond_wait(&cond, &mutex)`和`pthread_cond_broadcast(&cond)`条件变量。
```c
static void
barrier()
{
    // YOUR CODE HERE
    //
    // Block until all threads have called barrier() and
    // then increment bstate.round.
    //
    // 申请持有锁
    pthread_mutex_lock(&bstate.barrier_mutex);
    bstate.nthread++;
    if(bstate.nthread == nthread) {
        // 所有线程已到达
        bstate.round++;
        bstate.nthread = 0;
        pthread_cond_broadcast(&bstate.barrier_cond);
    } else {
        // 等待其他线程
        // 调用pthread_cond_wait时，mutex必须已经持有
        pthread_cond_wait(&bstate.barrier_cond, &bstate.barrier_mutex);
    }
    // 释放锁
    pthread_mutex_unlock(&bstate.barrier_mutex);
}
```
调用`pthread_cond_wait`时，*mutex*必须已经持有。
