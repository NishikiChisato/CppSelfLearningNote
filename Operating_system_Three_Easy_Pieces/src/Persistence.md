# Persistence

- [Persistence](#persistence)
  - [Introduce](#introduce)
  - [Device Interface \& Disk](#device-interface--disk)
    - [System Architecture](#system-architecture)
    - [Canonical Device](#canonical-device)
  - [Redundant Array of Inexpensive Disk](#redundant-array-of-inexpensive-disk)
    - [RAID Level 0: Striping](#raid-level-0-striping)
    - [RAID Level 1: Mirroring](#raid-level-1-mirroring)
    - [RAID Level 4: Saving Space With Parity](#raid-level-4-saving-space-with-parity)

## Introduce

为了让数据能够更为持久的保存，需要硬件（磁盘 `disk`）和软件（文件系统 `file system`）之间的合作完成，在此我们简单的讨论一下操作系统如何与设备进行通信，我们关注的重点在于文件系统这部分

## Device Interface & Disk

### System Architecture

一个标准的系统架构如下图所示

![SystemArchitecture](../img/SystemArchitecture.png)

图中越粗的线表示的速度越快，`CPU` 通过内存总线 `Memory Bus` 与系统内存相连；图像处理（显卡 `Graphics`）或者高性能设备通过通用 `I/O` 总线 `General I/O Bus` 与系统相连；**磁盘**、鼠标通过最外围的外围总线 `Peripheral I/O Bus` 连接到系统。也就是说，硬盘与系统的交换实际上是很慢的

### Canonical Device

一个标准设备 `Canonical Device` 的接口与内部如下所示：

![CanonicalDevice](../img/CanonicalDevice.png)

外部接口 `interface` 由三个寄存器组成：状态 `status`、命令 `command`和数据 `data`；内部组成 `internals` 由硬件构成：微处理器、内存和其他硬件芯片

操作系统与设备的交换主要通过设备提供的接口实现。`status` 寄存器可以读取并查看当前设备的状态；`command` 寄存器可以通知设备执行某个具体的命令；`data` 寄存器可以将数据传送给设备或从设备中读取数据

一个标准的与设备通信的协议分为以下四部：

1. 操作系统反复读取 `status` 寄存器，等待设备进入可以接收指令的状态，这称为轮询 `polling`
2. 操作系统将**数据**下发到 `data` 寄存器，这个过程中如果 `CPU` 参与数据移动我们称这个过程为 `Programming I/O`
3. 操作系统将命令写入 `command` 寄存器
4. 操作系统反复轮询该设备，判断命令是否执行完毕

当然，这个过程中有很多部分是可以优化的，例如操作系统可以使用中断来避免反复轮询，通过使用 `DMA, Direct Memory Access` 来避免 `Programming I/O`，可以进一步释放 `CPU` 的时间

由于每个设备的接口都不一样，为了将不同的设备纳入操作系统，操作系统需要有这些设备的设备驱动程序 `Device Driver`，通过这一层抽象，操作系统便可以对设备进行操作了（`Linux` 系统中有 `70%` 的代码都是各种驱动程序）

## Redundant Array of Inexpensive Disk

> 我们不讨论磁盘如何设计，我们只关注如何利用磁盘构建一个大型、快速、可靠的存储系统

廉价冗余磁盘阵列（`Redundant Array of Inexpensive Disk`）是将多个磁盘构建成一个更快、更大、更可靠的文件系统。从外部看，`RAID` 是一个磁盘，包含一系列可用读取和写入的块；从内部看，`RAID` 由多个磁盘、内存和一个或多个处理器构成

从性能 `Performance` 上来说，并行地使用多个磁盘可用加快 `I/O` 速度；从容量 `Capacity` 上来说，使用多个磁盘可以显著地提高容量；从可靠性 `Reliability` 上来说，允许一定程度的冗余能够保证系统在部分磁盘损坏的情况下还能正常工作。这三个方面也是评估 `RAID` 的三个指标

`RAID` 的设计有三种：`RAID Level 0 (Striping), RAID Level 1 (Mirroring), RAID Level 4/5 (Parity-Based Redundancy)` 

### RAID Level 0: Striping

这个级别的 `RAID` 没有冗余，它按照条带化 `Striping` 的方式将磁盘阵列中的块组织在磁盘上。我们将同一行中的块 `block` 称为条带，如下图（这里我们假定有四个磁盘 `disk`，每个 `block` 的大小为 `4KB`）：

![Striping](../img/Striping.png)

在这里，每一条 `stripe` 包含四个 `block`，大小为 `16KB`，**一共有四条 `stripe`**。当然，我们也可以将相邻的两个 `block` 放在同一个磁盘上以此构成一个 `chunk`，那么每个 `chunk` 的大小为 `8KB`，每一条 `stripe` 包含四个 `chunk`，大小为 `32KB`，**一共有两条 `stripe`**，如下图：

![AnotherStripe](../img/AnotherStripe.png)

在这里，如果 `chunk size` 过小，也就意味着许多文件可用跨越多个磁盘，这增加了文件并行读取的效率，但会增加跨多个磁盘访问的定位时间；相应地，如果 `chunk size` 过大，则会减小这种并行性，但定位时间则会降低

在评价性能时，我们主要考虑两种工作负载 `workload`：顺序 `sequential` 和随机 `random`。对于前者，我们假定对阵列的请求大部分是连续的；对于后者，我们假定每次请求都很小，并且随机分布在磁盘上的不同位置。我们设 `sequential` 下以 $S\ \rm MB/s$ 传输数据，`random` 下以 $R\ \rm MB/s$ 传输数据

> 我们简单计算一下二者的差距，假设我们的磁盘的属性如下：
> 
> * 平均寻道时间 $7\ \rm ms$
> * 平均旋转延迟 $3\ \rm ms$
> * 磁盘传输速率 $50\ \rm MB/s$
> 
> 我们设 $10\ \rm MB$ 为连续传输，$10\ \rm KB$ 为随机传送，那么有：
> 
> $$
> \begin{align*}
> &S=\frac{10\rm MB}{(7 + 3 + 200)\rm ms} = 47.62\ \rm MB/s\\
> &R=\frac{10\rm KB}{(10+\frac{10\rm KB}{50\rm MB/s})\rm ms}=0.981\ \rm MB/s
> \end{align*}
> $$
>
> 二者相差了 $50$ 倍

从性能的角度来看，单个块的请求延迟与磁盘延迟相一致；从容量的角度来看，我们并没有冗余，因此 $N$ 个磁盘全部用于存储数据；从可靠性的角度来看，顺序吞吐量为：$N\times S$，随机吞吐量为：$N\times R$

### RAID Level 1: Mirroring

`RAID 1` 仅仅是在 `RAID 0` 的基础上添加冗余，如下图：

![Mirroring](../img/Mirroring.png)

对于镜像 `mirror` 阵列，读取时可以读取任意一个副本。但在写入时，则需要更新两个副本的数据，以保持数据的可靠性。由于冗余分布在不同的磁盘上，因此这些写入可以**并行**进行

> 我们可以同时对多个磁盘进行并行读写，但对于单个磁盘的读写只能按顺序执行。因此我们选择**将冗余分布在多个磁盘而不是单个磁盘上**

从容量的角度来看，在有 $N$ 个磁盘的前提下，可以用作存储的空间为 $N/2$；从可靠性的角度来看，`Merror Level = 2`的 `RAID` 最多允许 $N/2$ 个磁盘发生损坏（运气爆炸的情况下）；最后我们分析性能，这其实稍微有点复杂

对于**顺序写入**，由于我们可用的磁盘实际上只有 $N/2$ 个，因此**顺序写入**的峰值带宽为 $\frac{N}{2}\times S$（也就是我们只能使用 $N/2$ 个磁盘进行顺序读取）

而对于**顺序读取**，直观上来看，我们可用从两个磁盘中任意读取一个，貌似会比写入的性能更好，但不幸的是，**顺序读取的峰值带宽**依旧为 $\frac{N}{2}\times S$，我们具体分析一下这个问题：

由于对每个 `block` 的读取可以来自两个磁盘，因此如果读取 `block 0, 1, 2, 3, 4, 5, 6, 7`，我们假定它们分布被定位到 `disk 0, 2, 1, 3, 0, 2, 1, 3`。发现问题了吗，**对于同一个磁盘而言，两次读取的块并不是连续的**，因此在这种情况下每个磁盘只能提供其峰值带宽的一半，因此顺序读取的峰值带宽依旧为 $\frac{N}{2}\times S$

对于**随机读取**，我们可以使用全部 $N$ 个磁盘进行随机读取，因此其峰值带宽为 $N\times S$

对于**随机写入**，由于只能使用 $N/2$ 个磁盘，因此其峰值带宽为 $\frac{N}{2}\times S$

### RAID Level 4: Saving Space With Parity

