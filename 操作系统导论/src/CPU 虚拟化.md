# CPU 虚拟化

- [CPU 虚拟化](#cpu-虚拟化)
  - [进程抽象](#进程抽象)
    - [数据结构](#数据结构)
    - [进程 API](#进程-api)
      - [fork 系统调用](#fork-系统调用)
      - [wait 系统调用](#wait-系统调用)
      - [exec 函数](#exec-函数)
  - [机制](#机制)


为了实现 CPU 的虚拟化，操作系统需要实现一些低级机制和高级智能。低级机制称为*机制* `mechanism`，用以实现基本的所需功能，高级智能称为*策略* `policy` ，也就是操作系统用于做出某种决定的算法

更本质地来说，机制主要回答「如何做」而策略主要回答「做哪个」

## 进程抽象

操作系统为正在运行的程序提供**抽象**，这也就是所谓的进程 `process`。进程的机器状态 `machine state` 由以下三类组成：

* 进程可以访问的内存（也就是进程的地址空间 `address space`）。这是因为指令和数据均存储在内存中，而一个进程必须要去访问这些内存
* 寄存器。进程在运行的过程中依赖于寄存器
* 文件列表。程序可能会访问 `I/O` ，因此进程的组成需要包含相应的文件列表

在正式运行进程前，操作系统需要先将**代码**和**静态数据**（需要初始化变量）加载到内存当中（也就是进程的地址空间内）。这是因为程序都是以可执行程序的形式存储在**磁盘**中，因此操作系统需要预先将其加载到内存才可以继续后面的步骤

将代码和静态数据加载到内存后，操作系统需要为进程分配**运行时栈**和**堆**。运行时栈用于存放局部变量、函数参数、函数返回地址；堆用于存放程序内显示请求分配的数据

此外，操作系统还需要去初始化一些与 `I/O` 设置相关的工作。例如系统中每个进程都会默认打开三个文件描述符，分别为：标准输入、标志输出、标志错误

完成上述工作后，操作系统才会开始启动进程，将 CPU 的控制权移交新创建的进程中

### 数据结构

对于每个进程，操作系统需要去跟踪它并记录相关的信息，以下是 `vx6` 中相应的结构体：

```cpp
//包含进程切换时需要保存和回复的寄存器
struct context {
  uint edi;
  uint esi;
  uint ebx;
  uint ebp;
  uint eip;
};


// 进程状态
enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

/*
    UNUSED：未使用，表示该进程控制块当前未被使用。
    EMBRYO：新建，表示该进程正在创建，但尚未准备好运行。
    SLEEPING：休眠，表示该进程正在等待某个事件的发生，例如等待 I/O 完成、等待定时器超时等。
    RUNNABLE：可执行，表示该进程已经准备好运行，并且等待被调度。
    RUNNING：正在运行，表示该进程当前正在 CPU 上执行。
    ZOMBIE：僵尸状态，表示该进程已经终止，但其父进程尚未回收它的资源。
*/

struct proc {
  char *mem;                     // 内存位置，仅在使用kvmalloc时才有意义
  uint sz;                       // 进程占用内存大小
  char *kstack;                  // 进程内核栈的起始地址
  enum procstate state;          // 进程状态：UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE
  int pid;                       // 进程ID
  struct proc *parent;           // 父进程
  struct trapframe *tf;          // 指向当前进程的中断帧
  struct context *context;       // 进程的上下文信息，用于函数调用
  void *chan;                    // 进程等待的channel地址
  int killed;                    // 进程是否已经被杀死
  struct file *ofile[NOFILE];    // 打开文件的指针数组
  struct inode *cwd;             // 进程当前工作目录的inode
  char name[16];                 // 进程名
};

```

### 进程 API

#### fork 系统调用

函数声明如下：

```c
#include <unistd.h>
pid_t fork()
```

返回值为负数表明创建失败，零表示当前进程为子进程，正数表示当前进程为父进程且该数为子进程的 `PID`

当调用 `fork()` 函数时，操作系统会复制当前进程（父进程）的所有代码、数据和堆栈等信息，将这些信息保存在一个新的进程空间中（子进程）。也就是说，子进程拥有自己独立的进程空间，可以独立处理不同的问题

父进程与子进程拥有相同的代码，二者**均会从 `fork()` 函数的下一条语句开始执行**。在父进程中 `fork()` 函数的返回值为子进程的 `PID` ，而在子进程中 `fork()` 函数的返回值为 $0$

需要注意的是，父进程的执行与子进程的执行顺序是不确定的，这依赖于 `CPU` 的调度，因此我们无法假设哪个进程先被执行

#### wait 系统调用

`wait` 系统调用有两种，分别为 `wait()` 和 `waitpid()` ，它们定义分别为：

```c
#include <sys/wait.h>

pid_t wait(int* status);
pid_t waitpid(pid_t pid, int* status, int options)
```

* 等待进程

  * `wait` 函数会等待**任意一个子进程退出**，也就是说如果有多个子进程的话，`wait` 是无法保证等待某个确切的子进程的

  * 对于 `waitpid` 而言，参数 `pid` 可以指定等待子进程的 `pid` ，如果为 `-1` 的话则表明等待任意子进程，此时与 `wait` 一样

* 子进程状态
  
  * 参数 `status` 用以指示**子进程结束时状态**，通常搭配一些宏来获取子进程的退出信息
    * `WIFEXITED`：如果子进程正常退出，则返回 `true` 。其中，`WIF`代表`wait if`，表示等待某个条件是否成立

    * `WEXITSTATUS`：如果 `WIFEXITED` 为 `true`，则此宏可用于获取子进程的退出状态。其中，`EXIT` 代表 `exit status`，表示进程退出的状态码。`EXITED` 表示单词 `exited`

    * `WIFSIGNALED`：如果子进程因为某种信号而异常退出，则返回 `true`。其中，`SIGNAL` 代表 `signal`，表示信号。`SIGNALED` 表示单词 `signaled`

    * `WTERMSIG`：如果 `WIFSIGNALED` 为 `true`，则此宏可用于获取引起子进程异常退出的信号编号。其中，`TERM` 代表 `terminate`，表示中止

    * `WCOREDUMP`：如果 `WIFSIGNALED` 为 `true` 并且产生了 `core dump`，则此宏将返回 `true`。其中，`CORE`代表 `core dump` ，表示进程崩溃时生成的内存映像文件

    * `WIFSTOPPED`：如果子进程目前已经停止，则返回 `true`。其中，`STOPPED` 代表 `stopped`，表示进程停止

    * `WSTOPSIG`：如果 `WIFSTOPPED` 为 `true`，则此宏可用于获取引起子进程停止的信号编号。其中，`STOP` 代表 `stop`，表示停止

    * `WIFCONTINUED`：如果子进程最近被恢复执行，则返回 `true`。其中，`CONTINUED` 代表 `continued`，表示继续执行


* 控制 `waitpid()` 行为
  
  * 参数 `options` 用于控制 `waitpid()` 的行为（可以选择按位或的方式进行组合），具体如下：

    * `WNOHANG`：如果没有任何子进程已经终止，则立即返回，不阻塞当前进程。`NOHANG` 表示 `no hang` ，表示不挂起当前父进程，也就是说不会等待子进程结束，父进程会继续执行代码，此时 `waitpid` 返回 $0$ 。一般的用法是在循环中使用，用来检测各个子进程是否结束

    * `WUNTRACED`：如果子进程进入了暂停状态（例如，被 `SIGSTOP` 信号挂起），则也立即返回。`UNTRACED` 表示 `untraced` ，表明不再继续跟踪子进程

    * `WCONTINUED`：如果子进程之前被暂停了，现在又已经恢复运行，则也立即返回。

    * `__WALL`：返回所有子进程的状态信息，包括被其它进程所控制的进程以及被内核所控制的进程。

    * `__WCLONE`：与 `__WALL` 类似，但只返回当前进程所创建的子进程的状态信息。

#### exec 函数

`exec` 函数有多个变体，这里不过多赘述相关用法，我们只阐述此函数的行为

当进程调用 `exec` 函数时，此进程会加载并允许可执行程序，并且新的程序会**覆盖当前进程的代码和数据空间**，因此此进程 `exex` 函数后面的代码将会全部失效，**永远不会被执行**（如果 `exec` 函数运行成功的话）

也就是说，**如果 `exec` 函数调用失败，会继续执行后面的代码**，我们可以判断该函数的返回值来判断是否执行失败（或者直接在后面接上标准错误输出）

`exec` 函数的所有变体的返回值，$-1$ 均表示执行失败，因此可以通过如下方式判断：

```c
if(exec() == -1)
{
    perror("exec failed");
    exit(EXIT_FAILED);
}
```

或者直接：

```c
exec();
perror("exec failed");
exit(EXIT_FAILED);
```

下面我们给出 `exec()`、`execl()` 和 `execvp()` 的区别

`exec()` 只接受**一个字符串参数**，用于指定**可执行程序的路径名**（此函数需要写上全部的路径名），例如：`exec("/bin/ls")`

`execl()` 接受多个参数，其中第一个是**可执行文件的路径名**（这里与 `exec` 一样），紧接着的为传递给该程序的命令行参数列表，但最后一个必须以 `NULL` 作为结尾。具体定义为：`int execl(const char* path, const char* arg, ...)`

第一个 `path` 指明可执行程序的路径，参数 `arg` 以及之后的则给出可执行程序的参数列表

用法：`execl("/bin/ls", "ls", "-l", NULL)`

`execvp()` 可以通过搜索环境变量 `PATH` 中指定的目录来查找可执行程序，具体定义为：

```c
int execvp(const char* file, char* const argv[])
```

`file` 用以指明可执行程序或者路径，`argv[]` 则是一个字符串数组，用以给出可执行程序的参数，最后一个必须以 `NULL` 作为结尾。用法如下：

```c
cahr* arg[] = { "wc", "file.c", NULL };
execvp(arg[0], arg);
```

需要说明的是，这种 `fork` 与 `exec` 分开设计有很多好处，举个例子：在 `shell` 界面，当我们输入一个命令时，`shell` 首先在文件系统中找到这个可执行程序，然后通过调用 `fork` 来创建一个子进程，在子进程中调用 `exec` 的某个变体来执行这个可执行程序，最后在父进程中调用 `wait` 来等待子进程的执行，`wait` 结束后，`shell` 则完成了此次命令的执行，等待用户输入下一个命令

---

## 机制

