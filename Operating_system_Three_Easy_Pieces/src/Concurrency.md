# Concurrency

- [Concurrency](#concurrency)
  - [Overview](#overview)
  - [Thread API](#thread-api)
    - [Thread](#thread)
    - [Lock](#lock)
    - [Condition](#condition)
  - [Lock](#lock-1)
    - [Test-And-Set](#test-and-set)
    - [Compare-And-Swap](#compare-and-swap)


## Overview

我们可以将进程进一步划分，便可以得到「线程」。一个单线程程序只有一个执行点（一个程序计数器，用来取指和执行），而多线程 `multi threaded` 程序具有多个执行点（多个程序计数器，每个都用来取指和执行）。实际上每个线程都有点像是独立的进程，但与进程不同的是，它们**共享相同的地址空间**

线程的状态与进程的状态类似，每个线程都有一个程序计数器和一组用于计算的寄存器。当需要进行上下文切换转去运行其他线程时，进程间的转换需要将进程的状态信息保存在进程控制块 `Process Control Block, PCB` 中，并**切换地址空间**；线程同样需要将当前线程的状态保存在线程控制块 `Thread Control Block, TCB` 中，但**不需要切换地址空间**

在传统的单线程地址空间模型中，只有一个栈，位于地址空间底部，程序代码位于地址空间最上面。而对应多线程程序而言，**不是地址空间只有一个栈，而是每个线程都有一个栈**，如下图：

![Multi_thread](../img/Multi_thread.png)

我们需要明确这两个定义：

* 临界区 `critical section`：一段访问**共享**资源（变量或数据结构）的代码，必须要保证所有的线程在此处是互斥 `mutual exclusion` 访问
* 竞态条件 `race condition`：出现多个线程同时进入临界区导致一些不希望出现的结果

我们用一个简单的例子来描述临界区和竞态条件这两个概念，考虑如下代码：

```cpp
int cnt = 0;

void* addone(void* arg)
{
    for(int i = 0; i < 1e5; i ++)
        cnt++;
    return NULL;
}

int main()
{
    pthread_t t1, t2;
    cout << "begin: " << cnt << endl;
    pthread_create(&t1, NULL, addone, NULL);
    pthread_create(&t2, NULL, addone, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    cout << "end: " << cnt << endl;
    return 0;
}
```

`cnt` 为全局的**共享**变量，也就是每个线程都可以访问，访问该变量的代码我们称之为 `临界区`。我们创建了两个线程，每个线程都会对 `cnt` 加一，总共应该会加 `2e5` 次

执行的结果如下：

```bash
begin: 0
end: 139023
```

我们发现这个结果并不是 `2e5`，是一个其他的数。其实这里如果我们多执行几次，会发现每次的结果都不相同。由于每次的执行结果都不确定，这便是 `竞态条件`

我们假设 `cnt++` 被翻译为以下代码：

```assembly
mov 0x8049a1c, %eax
add $0x1, %eax
mov %eax, 0x8049a1c
```

我们从地址 `0x8049a1c` 处读取值到寄存器中，对寄存器中的数组加一，然后将该值加载回去

假设初始值为 `50`，假设线程 `1` 执行将值加载到寄存器中并对其加一，次数寄存器中的值为 `51`。如果这个时候触发时钟中断，操作系统将当前线程的状态保存在 `TCB` 中，并切换为线程 `2` 运行。如果线程 `2` 能够全部执行完所有的代码，那么这个时候会将 `51` 写入到该地址处（也就是 `cnt` 变为 `51`）。此时中断再次发生，转为线程 `1` 运行，线程 `1` 会**将 `51` 再次写入到该地址处**，这里便是问题所在了

本应该两个线程独立使 `cnt` 加一，最终的结果应该变为 `52`，但实际上我们的结果只是 `51`。由于不合时宜的时钟中断，导致某个线程执行了重复的操作，这便是每次执行结果不一样的原因

因此，我们才要求线程互斥 `mutual exclusion` 地访问临界区

## Thread API

> 注：这些均为 `C` 语言的多线程函数。所有包含 `pthread.h` 头文件的在链接时需要带 `-lpthread` 参数才能通过

### Thread

线程用 `pthread_t` 表示，需要使用线程时要引入 `pthread.h` 头文件

线程创建函数：

```cpp
int pthread_create(pthread_t* thread,
                   const pthread_attr_t* attr, 
                   void* (*start_routine)(void*), 
                   void* arg)
```

各参数含义如下：

* `thread`：一个指向 `pthread_t` 的指针
* `pthread_attr_t`：通过调用 `pthread_attr_init()` 进行初始化线程属性
* `start_routine`：一个函数指针，表明线程应该从哪里开始执行
* `arg`：传递给线程开始执行的函数的参数

需要说明的是，这个函数指针默认参数为 `void*`，默认返回值为 `void*`。**用 `void*` 是为了能够接收任何类型，哪怕不是指针变量也可以**。**如果需要传递多个参数，只能用结构体将其封装**

该函数在线程**创建成功后，会返回 `0`**；如果线程创建失败，则会返回一个 `error number`：

* `EAGAIN`：系统资源不足以去创建一个新线程
* `EINVAL`：`attr` 无效
* `EPERM`：没有权限设定 `attr` 中指定的参数或调度策略

线程创建后，线程会开始执行，如果我需要等待该线程执行完毕，则需要使用 `pthread_join()` 函数：

```cpp
int pthread_join(pthread_t thread, 
                 void** retval)
```

* `thread`：表示需要等待执行完成的线程
* `retval`：表示该线程的返回值

如果此函数**正确执行，那么返回 `0`**；如果失败，则返回 `error number`：

* `EDEADLK`：检测到死锁
* `EINVAL`：另外一个线程正在等待此线程
* `ESRCH`：没有该线程 `ID`

### Lock

`pthread_mutex_t` 用于表示一个锁变量的类型，以下两组函数可以用于获取锁与释放锁：

```cpp
int pthread_mutex_lock(pthread_mutex_t* mutex)
int pthread_mutex_unlock(pthread_mutex_t* mutex)
```

锁的初始化可以是动态的也可以是静态的：

```cpp
//static
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER
//dynamic
int rc = pthread_mutex_init(&lock, NULL)
assert(rc == 0)
```

动态初始化的第二个参数是一组可选的属性，`NULL` 表示默认值。该函数返回零表示执行成功

### Condition

条件变量用类型 `pthread_cond_t` 表示，静态初始化为：

```cpp
pthread_cond_t cond = PTHREAD_COND_INITIALIZER
```

关于条件变量常用的函数有两个，一个用于当前线程等待某个条件，当条件不满足时该线程进入休眠状态：

```cpp
int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex)
```

另一个用于表示当前条件已经满足，可以唤醒休眠的线程

```cpp
int pthread_cond_signal(pthread_cond_t* cond)
```

## Lock

从前面的例子中我们看到，由于单处理器上的**中断**，我们无法做到原子 `atomic` 地执行一系列的指令。由于我们无法关闭硬件中断，因此我们需要引入锁 `lock` 这个概率来解决这个问题

我们可以对临界区 `critical section` 加锁，保证只有持有锁的线程可以进入该临界区，其余所有没有持有锁的线程都无法进入该临界区，这样我们便可以保证该临界区的代码像单条原子指令一样执行。类似这样：

```cpp
lock_t mutex;
lock(&mutex);
cnt = cnt + 1;//critical section
unlock(&mutex);
```

锁是一个变量，用于保存当前时刻锁的状态，一个锁**要么是被持有的 `acquired, locked, held`，要么是可用的 `avaliable, unlocked, free`**。对于一个可获取的锁而言，`lock` 会对锁变量进行赋值，保存当前锁的状态（该线程获得锁），直到该线程调用 `unlock` 归还锁之前，如果有其他线程调用 `lock`，那么它们会被阻塞（因为锁只有一个，并且已被占用）。当持有锁的线程调用 `unlock` 后，会释放该锁，被 `lock` 卡住的其中一个线程会重新获得该锁

> 为什么不能用控制中断 `control interrupt` 的方法解决并发 `concurrency`
>
> 第一，这将运行线程使用特权 `privilege` 操作。我们在操作系统中引入中断的原因就是保证操作系统每隔一段固定时间就拥有对系统的最高控制权
>
> 第二，不支持多处理器。哪怕可以关闭某个处理器的中断，该线程却可以运行在其他的处理器上
>
> 第三，关闭中断的过程中会导致中断丢失。如果关闭中断的过程中，磁盘设备完成了 `I/O` 请求，由于处理器屏蔽了中断，因此这将导致该中断被忽略

我们可以用锁去解决并发的问题，那么问题是，**硬件和操作系统该如何实现一个锁**，下面我们介绍几种**硬件原语 `hardware primitives`**，操作系统可以利用这些硬件原语来实现一个锁

我们主要的工作为，实现 `lock_t` 类型和 `lock(), unlock()` 函数

### Test-And-Set

`test and set` 也被称为 `atomic exchange`，我们用一个变量来表示该锁是否被某些 线程占用。初始时该变量为零，当第一次调用时将该变量设置为一；当该线程离开临界区时，再将该变量设置为零，有：

```cpp
typedef struct lock_t { int flag; } lock_t;
void init(lock_t* lock)
{
  //0 is available, 1 is held
  lock->flag = 0;
}
void lock(lock_t* lock)
{
    while(mutex->flag == 1)
        ;//spin
    lock->flag = 1;
}
void unlock(lock_t* lock)
{
    lock->flag = 0;
}
```

这个代码抛开性能 `performance` 问题不谈，有一个很严重的问题是，它并不能解决并发

当第一个线程在执行完 `while` 判断后，这个时候判断为 `false`，它可以对 `flag` 设置为一，如果这时时钟中断发送，第二个线程开始执行。我们会发现这两个线程都对 `flag` 进行了设置，也就是二者都进入了临界区，这陷入无法解决并发的问题

我们需要硬件的支持，具体地，我们引入一个函数，硬件可以原子地执行以下代码：

```cpp
int TestAndSet(int* old_ptr, int new_val)
{
    int old = *old_ptr;
    *old_ptr = new_val;
    return old;
}
```

硬件可以原子的执行该函数中的所有指令，因此可以保证在执行该函数的过程中，不会被时钟中断打断。因此我们可以将锁更改为如下设计：

```cpp
typedef struct lock_t { int flag; } lock_t;

void init(lock_t* lock)
{
    lock->flag = 0;
}

void lock(lock_t* lock)
{
    while(TestAndSet(&lock->flag, 1) == 1)
        ;//spin
}

void unlock(lock_t* lock)
{
    lock->flag = 0;
}
```

由于 `TestAndSet` 操作为原子操作，因此我们在执行完 `while` 的判断时，如果锁没有被持有的话，我们已经对该锁进行了赋值，那么当前线程就可以直接跳出循环，进入临界区。通过避免在 `while` 循环后面设置锁变量，我们可以解决让线程互斥地进入临界区这一问题

这种锁被称为自旋锁 `spin lock`，这是因为被阻塞进程会一直自旋，在自己的 `CPU` 时间片内等待，直到锁可用

在单 `CPU` 上，若有 $N$ 个线程并发执行，那么对于一个临界区会有 $N-1$ 个线程等待。对于每个被阻塞的线程，它们在执行的时间片内都会空转，白白浪费 `CPU` 的时间，因此我们可以推断出，自旋锁会导致**饥饿** `starvation` 的情况

### Compare-And-Swap

