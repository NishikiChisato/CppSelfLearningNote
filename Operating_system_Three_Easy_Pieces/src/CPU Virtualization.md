# CPU Virtualization

- [CPU Virtualization](#cpu-virtualization)
  - [进程抽象](#进程抽象)
    - [数据结构](#数据结构)
    - [进程 API](#进程-api)
      - [fork 系统调用](#fork-系统调用)
      - [wait 系统调用](#wait-系统调用)
      - [exec 函数](#exec-函数)
    - [进程空间](#进程空间)
  - [运行机制](#运行机制)
    - [受限操作](#受限操作)
    - [进程切换](#进程切换)
      - [协作](#协作)
      - [非协作](#非协作)
  - [调度策略](#调度策略)
    - [引入](#引入)
    - [FIFO](#fifo)
    - [SJF](#sjf)
    - [STCF \& PSJF](#stcf--psjf)
    - [RR](#rr)
    - [结合 I/O \& 无法预知](#结合-io--无法预知)
  - [调度：多级反馈队列 - MLFQ](#调度多级反馈队列---mlfq)
  - [调度：比例份额 - Proportional-share](#调度比例份额---proportional-share)


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

### 进程空间

一个进程的空间可以被划分为以下的几个部分：

* 代码段：用于存储程序的机器语言指令，也称为文本段。通常是只读的，因为程序运行时不应该修改自己的代码

* 数据段：存储程序中已初始化的全局变量和静态变量

* `BSS` 段（`Block Started by Symbol`）：存储程序中未初始化的全局变量和静态变量，其内容被初始化为 `0` 或 `null`

* 堆区：由动态内存分配函数（例如malloc）进行管理，用于动态分配内存

* 栈区：用于存储函数调用时的临时变量、函数参数和返回值等信息

* 内核栈：用于处理与进程相关的内核级别事件和中断，上下文切换时会将进程状态保存在此，通常通过内核栈指针来访问（内核栈指针存储在 `PCB` 中）

除了上述部分之外，还有一些其他的辅助数据结构和控制块，用于辅助操作系统对进程进行管理和调度，例如进程控制块 `PCB` 、页表、文件描述符表、信号处理函数表等。这些数据结构通常会被存储在内核空间中


---

## 运行机制

虚拟化 CPU 一个非常重要的做法是时分共享 `time sharing` ，也就是操作系统需要解决两个问题：

* 如果只运行一个进程，操作系统如果确保它不做我们不希望它做的事情同时高效运行它
* 当我们运行一个进程时，操作系统该如何将其停下来去运行另一个进程

我们将基于这两个问题阐述虚拟化 CPU 的相关底层机制

### 受限操作

> 问题：一个进程必须执行 `I/O` 或一些其他受限制的操作，但又不能让进程完成控制操作系统，该如何实现这一点

硬件通过提供不同的**执行模式**来协助操作系统。具体来说，CPU 有两种运行模式：用户模式 `user mode` （也称为用户态）和内核模式 `kernel mode` （也成为内核态）（这里是 CPU 的运行模式）

**进程运行在用户态下**，只能执行受限制的指令，不能执行 `I/O` 、文件、网络相关的指令。如果进程需要执行这些指令，只能通过委托操作系统来执行的方式来进行

操作系统在**用户模式和内核模式均会运行**，在用户模式下，操作系统会提供一些受保护的接口供进程调用以及管理各个进程的状态和资源，而在内核模式下操作系统则拥有更高的指令权限，能够执行权限级别更高的指令

也就是说，在用户模式下会运行进程和「部分操作系统」，而在内核模式下只会运行操作系统

在用户态下，**进程只能访问自己的地址空间和受限资源**，如果需要执行「受限制操作」则需要通过操作系统提供的**系统调用**来执行

这里的受限操作包括：

* 文件系统相关操作：例如打开、创建、删除文件，读写文件内容等。
* 内存管理相关操作：例如分配、释放内存，修改内存权限等。
* 进程管理相关操作：例如创建、暂停、终止进程，获取进程状态等。
* 网络通信相关操作：例如建立网络连接，发送和接收数据等。
* 设备驱动程序相关操作：例如控制设备的启动、停止、重启等。

由于这些操作均为**特权级别**的操作，因此 CPU 需要运行在**内核态**时，操作系统才能够执行

当 CPU 运行在内核态时，操作系统可以执行特权级别更高的指令，如读写所有进程的地址空间、直接访问硬件设备等。而当 CPU 运行在用户态时，进程只能访问自己的地址空间和受限资源，不能直接访问硬件设备或其他进程的地址空间

因此，内核态和用户态的区别在于它们所**能够执行的指令集合不同**。在内核态下，操作系统可以执行更加底层的、具有特权级别的指令，以便对系统资源进行管理和控制；而在用户态下，进程只能执行受限指令，以保证系统的安全性和稳定性

由于这种不同，这里便有了一种特殊的指令：陷阱 `trap` 指令，用于将 CPU 从用户态切换为核心态（也就是说，如果进程要执行系统调用，必须先执行陷阱指令）

当执行陷阱指令时，CPU 会**陷入**内核态，此时操作系统可以执行特权操作，当完成进程希望执行的工作后，操作系统指令一条特殊的指令：从陷阱中返回 `return-from-trap` ，**此后 CPU 由内核态转为用户态**，继续进程的执行

在执行 `trap` 时，必须保证在从 `trap` 中返回后能够继续刚才的状态执行下去，为此必须存储相关的寄存器。在 `x86-64` 上，CPU 会将程序计数器、标志和一些寄存器保存在每个进程的内核栈 `kernel stack` 上，以便能够正确返回

除了在系统调用前使用 `trap` 来使 CPU 陷入内核态以外，`trap` 还可以用于处理各种**硬件和软件错误**，例如除零错误、页面访问错误等。当发生这些异常时，CPU也会执行陷阱指令，**将控制权转移到内核态下的异常处理程序中**，以便对异常进行处理或终止进程的执行

那么这里又引出了另外一个问题：当执行 `trap` 时，操作系统该去执行哪些代码

显然发起 `trap` 的过程**不能指定要跳转的地址**，否则程序便可以跳到内核当中的任意位置了。实际上，**操作系统在系统启动时会设置陷阱表 `trap table`**

当系统启动时，操作系统会先在内核模式下执行，此时操作系统做的第一件事为：**告诉硬件在发生某些异常错误时该去执行哪些代码**，这便是设置 `trap table`，直到下一次重启机器前，硬件都知道在出现错误时该执行哪些代码

---

### 进程切换

> 问题：如果一个进程在 CPU 上运行，这意味着操作系统没有运行，那操作系统该如何重新获得 CPU 的控制权 `regain control` 以便它可以在进程之间切换

#### 协作

在这种方式下，操作系统**相信**系统的进程会合理的运行，运行时间过程的进程会自动放弃 CPU（通过一个特殊的**系统调用**指令），以便操作系统可以决定运行其他任务

此外，如果进程产生了某些错误，此时控制也能够回到操作系统的手中

但有个问题是，如果进程只是在执行死循环，既不进行系统调用，也不产生错误，那么操作系统**将不能做任何事情**

如果要解决这个问题，只能通过重启机器解决。那么这在某种程度上等同于没有解决操作系统该如何获得控制权的问题

因此这便引出了非协作模式

#### 非协作

我们在硬件上设计一个时钟中断 `timer interrupt` ，这使得时钟设备每隔几毫秒便可以产生一次中断。当中断发生时，系统调用中断处理函数 `interrupt handle` ，此时操作系统便可以重新获得 CPU 的控制权

类似 `trap table` 一样，在时钟中断发生时操作系统也需要直到该去执行哪些代码，因此**在系统启动时，操作系统也会启动时钟**

当操作系统重新取得 CPU 的控制权后，需要决定：继续运行当前进程还是切换到另一个进程。这一判断由调度程序 `scheduler` 给出，属于*策略*的范围，我们将在后面进行阐述

我们假设操作系统决定切换进程，也就是上下文切换 `context switch` ，那么此时硬件的责任是必须要**保存当前正在执行的进程的状态到该进程的内核栈中，并为即将要执行的进程从其内核栈中恢复状态**

当操作系统决定切换一个进程时，它会执行以下步骤：

* 保存当前进程上下文。操作系统会将当前进程的状态（`PC`、通用寄存器等）保存在内核栈中
* 切换内核栈指针。操作系统将当前进程的内核栈指针保存在其 `PCB` 中，然后加载新的内核栈指针，以便执行新的进程
* 恢复新进程的上下文。操作系统会从新进程的内核栈中恢复状态，这样新进程便可以从上次被中断的地方开始执行

至此为止，我们描述了操作系统实现虚拟化 CPU 的一些底层机制，我们称该机制为直接受限执行 `limited direct execution` ，也就是进程通过受限运行的方式在 CPU 上执行非特权指令，如果需要执行特权指令时则委托操作系统代劳

此时我们还有一个非常关键的问题没有解决：在特定的时间，操作系统该决定运行哪个进程，也就是调度策略该如何设计。这便是后面所需讨论的内容了

---

## 调度策略

### 引入

对于操作系统当中的进程（也被称为工作任务），我们做出如下假设：

* 每个工作运行相同的时间
* 所有工作同时到达
* 一旦开始，每个工作一直运行直到完成
* 所有工作只运行 `CPU`（不执行 `I/O` 请求）
* 每个工作的运行时间都是已知的

在之后的讨论中，我们会逐步放开这些假设，以便让我们的讨论更加基于现实

我们首先给出一个用于衡量调度策略的指标：周转时间 `turnaround time` 。一个任务的周转时间定义为完成时间减去到达时间，即：

$$
T_{turnaround}=T_{finish}-T_{arrive}
$$

由于我们假设所有任务同时到达，因此 $T_{arrive}=0$，有：$T_{turnaround}=T_{finish}$

实际上，周转时间是一个性能 `performance` 指标，另一个指标是公平 `fairness` ，我们在后面的讨论中会逐步加入。可以预见的时，二者在调度系统中是对立存在的

### FIFO

最容易想到的是先进先出（`First In First Out` ($\rm FIFO$)），也被称为先来先服务 `First Come First Server` ($\rm FCFS$)

设三个任务 $A,B,C$ 到达时间基本相同（$A$ 比 $B$ 快一点点，$B$ 比 $C$ 快一点点），三者运行时间均为 $10\ \rm s$ ，不难发现 $A$ 在 $10\ \rm s$ 完成，$B$ 在 $20\ \rm s$ 完成，$C$ 在 $30\ \rm s$ 完成，因此平均周转时间为：$(10+20+30)/3=20$

我们放宽第一个假设，即所有工作运行时间不同，我们很容易就能发现 $\rm FIFO$ 的问题

如果运行时间短的任务排在运行时间长的任务后面，就会导致周转时间变长。还是刚刚的例子，$A$ 的运行时间变为 $100\ \rm s$，此时平均周转时间为：$(100+110+120)/3=110$

这通常被称为护航效应 `convoy effect`，也就是消耗较少的排在消耗较多的后面

### SJF

要去解决 $\rm FIFO$ 的问题，最容易想到的是让运行时间短的任务优先运行，这便是最短任务优先 `Shortest Job First` ($\rm SJF$)

在后面的例子中，周转时间变为 $(10+20+120)/3=50$，这有效降低了 $\rm FIFO$ 在最坏情况下的周转时间

事实上，若所有工作同时到达，可以证明 $\rm SJF$ 算法得到的周转时间是最优的

朴素的 $\rm SJF$ 是非抢占式的 `non-preemptive`，即当一个长任务正在运行，如果此时短任务到达了，会导致平均周转时间增加。考虑下面这种情况：

在此我们放宽第二个假设，即工作并不是同时到达的，我们很容易能发现 $SJF$ 的另一个问题

假设$A$ 在 $t=0$ 到达，运行时间为 $100\ \rm s$，而 $B$ 和 $C$ 在 $t=10$ 到达，运行时间均为 $10\ \rm s$。由于 $\rm SJF$ 是非抢占式的，因此平均周转时间为：$(100+(110-10)+(120-10))/3=103.33$

### STCF & PSJF

在此我们放宽第三个假设，引入抢占 `preemptive`。引入抢占式的 $\rm SJF$ 算法被称为最短完成时间优先 `Shortest Time-to-Completion First` ($\rm STCF$) 或者抢占式最短作业优先 `Preemptive Shortest Job First` ($\rm PSJF$)

在刚刚的例子中，引入 $\rm STCF$ 得到的平均周转时间为：$((20-10)+(30-10)+120)/3=50$

貌似在周转时间这一指标上，$\rm STCF$ 的是最优解，那么此时我们需要引入另一个指标：响应时间 `response time` ，这对应于操作系统的公平性

我们定义响应时间为任务首次运行的时间减去首次到达的时间，即：

$$
T_{resopnse}=T_{run}-T_{arrive}
$$

我们定义响应时间这个指标的目的在于表征系统的交互性，用于判断该系统的交互性是否够好

在我们上面的例子中，$A$ 和 $B$ 的响应时间均为 $0$，$C$ 的响应时间为 $10$，平均响应时间为 $3.33$

我们注意到，$C$ 如果要运行的话，需要等待 $B$ 运行结束，也就是需要等待 $10\ \rm s$ 才行。假象你在电脑面前，你想要运行一个程序，双击之后这个程序十秒之后才开始运行，相信我，你会砸了这台电脑的

### RR

为了解决公平性的问题，我们介绍一种新的算法，它被称为轮转 `Round-Robin` ($\rm RR$)

其思想很简单，要保证公平，那么让每个任务**轮流**执行一个时间片 `time slice`，每执行完一个任务之后就切换另一个任务，直到完成所有的任务

我们考虑一个情况：$A,B,C$ 的运行时间均为 $5$，它们同时到达，时间片为 $1\ \rm s$。显而易见的是，$A,B,C$ 的响应时间分别为 $0,1,2$，平均响应时间为：$(0+1+2)/3=1$

进一步，如果时间片更小的话，响应时间也会相应的缩小。但时间片不能太短：频繁切换上下文所造成的系统开销会影响整体的性能。因此在时间片的设计上，最好是足够长，减小上下文切换的次数，以此摊销 `amortize` 上下文切换的成本。同时还需要足够小，以此保证响应时间不至于过长

但是，如果我们考察轮转时间，就会发现 $A$ 在 $t=13$ ，$B$ 在 $t=14$，$C$ 在 $t=15$ 完成，平均周转时间为：$(13+14+15)/3=14$，这却是难以接受的

当然，需要说明的是，时间片的大小必须为**时钟中断的整数倍**

### 结合 I/O & 无法预知

我们以及对放宽前三个假设进行了相应的讨论，在此我们放宽最后两个假设，我们首先讨论任务会执行 `I/O` 操作

当进程发起 `I/O` 请求时，它会被阻塞，进入阻塞队列中以等待 `I/O` 的完成，此过程中该进程不会使用 `CPU`，因此当该进程被阻塞时，操作系统可以**调度其他进程上 `CPU` 运行**，这样可以**保证 `CPU` 始终处于工作状态**。这种做法被称为重叠 `overlap`

至此，我们终于来到了最后一个假设：预先已知所有工作的运行时间。这一点在实际中是不可能预知的，因此我们能做的是**依据历史**来判断该工作未来的运行时间如何。在之后的讨论中，我们讲把这种预测结合在 $\rm SJF/STCF$ 与 $\rm RR$ 上，以求设计出一个在周转时间与响应时间都较优的调度策略

---

## 调度：多级反馈队列 - MLFQ

下面我们介绍一种综合的调度策略：`MLFQ` （`Mutil-level Feekback Queue, MLFQ`）

`MLFQ` 中**有许多独立的队列**，**每个队列有不同的优先级**。任何时刻一个工作只能存在于一个队列在。`MLFQ` 会按如下两规则执行任务：

* 规则 `1`：如果 $A$ 的优先级大于 $B$ 的优先级，那么执行 $A$ 而不执行 $B$
* 规则 `2`：如果 $A$ 的优先级等于 $B$ 的优先级，那么**轮转执行** $A$ 和 $B$

如果按照如上两个规则允许的话我们发现，优先级较低的任务将无法被执行，因为我们尝试**改变**优先级

* 规则 `3`：工作进入系统时，处于最高优先级（位于最上层队列）
* 规则 `4`：
  * 工作用完整个时间片后，降低其优先级
  * 如果工作在其时间片内主动释放 `CPU`，则其优先级不变

我们可以将工作负载分为允许时间短、频繁放弃 `CPU` 的交互型工作，也有需要很多 `CPU` 时间、响应时间却不重要的长时间密集型工作

当单个长工作运行时，它会**逐渐**降低为最低优先级，**之后在此运行**。如果这是来了一个短工作，由于其工作时间短，因此会先运行它，直到它的优先级降为最低为止。因此，这个过程可以近似为 `SJF` 或 `STCF`

具有交互性的工作不止有运行时间短这个特定，还会有**频繁执行 `I/O` 请求**这个特定（想象一个程序需要用户用键盘和鼠标来不断的输入），而规则 `4` 的第二点则可以保证执行 `I/O` 请求后，**该工作的优先级不发生改变**

但是，该设计会导致调度程序发生饥饿 `starvation` 问题。如果系统有过多的交互性短工作，那么就会导致计算密集型工作**一直得不到执行**，因此我们考虑提升工作的优先级

* 规则 `5`：经过一段时间 $S$，将系统中所有工作全部加入最高级优先队列

由于相同优先级的工作会轮转执行，因此长工作在一个 $S$ 周期内至少会执行一次，这就避免了饥饿的问题

其次，这里仍有另外一个问题是，一些恶意的应用程序**为了保持其优先级**，可以在时间片结束之前便**进行 `I/O` 请求而不做任何事**。如此便来该工作便可以一直保持在高优先级，占用更多的 `CPU` 资源

为此，我们需要**为每层队列提供更完善的 `CPU` 计时方式** `acounting`。调度程序记录一个进程**在某一层消耗的总时间，而不是在调度事重新计时**，只要进程完成了这一层的配额，便将其优先级降低。这个过程中，无论是一次用完的，还是拆成多次用完的，都统一对待。我们重写规则 `4`

* 规则 `4`：
  * 如果工作在其时间片内主动释放 `CPU`，则其优先级不变
  * 一旦工作用完其在某一层当中的配额（无论中间主动放弃了多少次 `CPU`），就降低其优先级（移入低一级队列）

最后，我们简单总结一下 `MLFQ` 的调用规则：

* 规则 `1`：如果 $A$ 的优先级大于 $B$ 的优先级，那么执行 $A$ 而不执行 $B$
* 规则 `2`：如果 $A$ 的优先级等于 $B$ 的优先级，那么**轮转执行** $A$ 和 $B$
* 规则 `3`：工作进入系统时，处于最高优先级（位于最上层队列）
* 规则 `4`：
  * 如果工作在其时间片内主动释放 `CPU`，则其优先级不变
  * 一旦工作用完其在某一层当中的配额（无论中间主动放弃了多少次 `CPU`），就降低其优先级（移入低一级队列）
* 规则 `5`：经过一段时间 $S$，将系统中所有工作全部加入最高级优先队列

---

## 调度：比例份额 - Proportional-share

这是一个不同于多级反馈队列的调度策略，被称为比例份额 `Proportional-share`，有时也被称为公平份额 `fair-share`。此算法基于一个最简单的思想：调度程序的最终目标是确保每个工作获得一定比例的 `CPU` 时间，而不是优化周转时间和响应时间

用彩票数来表示**进程的份额**，一个进程拥有的彩票占总彩票的百分比，就是它占有资源的份额。我们每隔一段时间抽取一次彩票，越是频繁运行的进程，它便越有机会拥有更多的彩票

假设 $A$ 和 $B$ 两个进程分别有彩票 `75` 张和 `25` 张，因此我们希望 $A$ 占用 `CPU` $75\%$ 的时间，$B$ 占用 `CPU` $25\%$ 的时间

我们不定时（每个时间片结束）的抽取彩票，每次获得一个**随机数**（在这个例子中，这个数为 $0\sim 99$），拥有这个数对应的彩票的进程则获得 `CPU` 运行的机会

例如，$A$ 拥有编号为 $0$ 到 $74$ 的彩票，$B$ 拥有编号为 $75$ 到 $99$ 的彩票，如果随机数为 $[0,74]$ 当中的任意一个数，则运行 $A$ ，反之则运行 $B$

也许在有限次运行下，该调度算法很难达到我们预期的那样（$A$ 占用 `CPU` $75\%$ 的时间，$B$ 占用 `CPU` $25\%$ 的时间），但只要运行相当多的时间片，便可以近似得到我们所期望的结果

这种调度方式看似并没有解决问题，因为对于一组给定的工作，该如何分配彩票的数目则没有具体的答案。因此我们只能说，利用比例份额分配的随机性，我们能够相对公平的调度各个任务，但各个任务之间的份额具体为多少则没有定论