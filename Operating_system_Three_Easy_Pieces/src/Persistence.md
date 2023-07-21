# Persistence

- [Persistence](#persistence)
  - [Introduce](#introduce)
  - [Device Interface \& Disk](#device-interface--disk)
    - [System Architecture](#system-architecture)
    - [Canonical Device](#canonical-device)
  - [Redundant Array of Inexpensive Disk](#redundant-array-of-inexpensive-disk)

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

