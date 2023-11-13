# Project 4

- [Project 4](#project-4)
  - [Concurrency Problem](#concurrency-problem)
    - [Dirty Read](#dirty-read)
    - [Unrepeatable Read](#unrepeatable-read)
    - [Phantom Read](#phantom-read)
    - [Lost Update](#lost-update)
  - [Isolation Level](#isolation-level)
  - [Definition Issue](#definition-issue)
    - [LockRequestQueue](#lockrequestqueue)
    - [Lock Upgrade](#lock-upgrade)
  - [Task 1](#task-1)
    - [Lock](#lock)
    - [Unlock](#unlock)
  - [Task 2](#task-2)
  - [Task 3](#task-3)


## Concurrency Problem

我们首先总结并发情况下可能会出现的问题

### Dirty Read

|$T_1$|$T_2$|
|:-:|:-:|
|$W(A)$||
||$R(A)$|
|$W(A)$||
|`commit / abort`|`commit`|

$T_2$ 读取到了 $T_1$ **未提交**的值，我们称为 `dirty read`

### Unrepeatable Read

|$T_1$|$T_2$|
|:-:|:-:|
|$R(A)$||
||$R(A)$|
|$W(A)$||
||$R(A)$|
|`commit`|`commit`|

$T_2$ 两次读取的值不一样，我们称为 `unrepeatable read`

### Phantom Read

|$T_1$|$T_2$|
|:-:|:-:|
|$R(A)$||
||$R(A)$|
|$D(A)$||
||$R(A)$|
|`commit`|`commit`|

$T_2$ 第二次读取的值被删除了，我们称为 `phantom read`

### Lost Update

|$T_1$|$T_2$|
|:-:|:-:|
|$W(A)$||
||$W(A)$|
|`commit`|`commit`|

$T_1$ 的写入被覆盖了，我们称为 `lost update`

## Isolation Level

依据不同的隔离级别，分别有可能会出现不同的问题：

- `Serializable`：执行 `strict 2PL` 和 `index lock`，不会出现上面的问题

- `Repeatable Read`：只执行 `strict 2PL`，不执行 `index lock`。只存在 `phantom read`
  - 在 `growing` 阶段允许获取所有类型的 `lock`
  - 在 `shrinking` 阶段不允许获取任何类型的 `lock`
 
出现 `phantom read` 的原因在于两个事务对同一个谓词逻辑下的集合执行操作，如果我们加 `index lock`，那么同一时间就只允许一个事务去访问该谓词逻辑下的集合，这便解决了 `phantom read`

在上面的例子中，一旦引入 `index lock`，那么 $T_2$ 就只能在 $T_1$ **对该对象的操作完全执行完毕**后再对该对象进行操作

- `Read Committed`：在 `RR` 的基础上，允许 `shrinking` 阶段获取 `S` 和 `IS`，存在 `phantom read` 和 `unrepeatable read`
  - 在 `growing` 阶段允许获取所有类型的 `lock`
  - 在 `shrinking` 阶段只允许获取 `S` 和 `IS`

在 `2PL` 中，一个事务最开始的状态为 `growing`，一旦其释放了某个 `lock`，那么其状态转换为 `shrinking`。`2PL` 的思想实际是事务在开始时获取所有需要的 `lock`，然后对所有的对象执行完操作后，再一起释放 `lock`

也就是说，进入 `shrinking` 这个阶段后，就说明当前事务已经对某些对象执行完操作了。如果这个时候运行该事务再次读取某些对象的值（也就是获取 `S` 和 `IS`），那么本次读取的值和之前读取的值就有可能存在不同

- `Read Uncommitted`：在 `RC` 的基础上，删去所有与 `S` 相关的 `lock`，这会造成 `dirty read`，因此在 `RU` 隔离级别下，会存在：`phantom read`、`unrepeatable read` 和 `dirty read`
  - 在 `growing` 阶段只允许获取 `X` 和 `IX`
  - 在 `shrinking` 阶段不允许获取任何类型的 `lock`

在有 `S` 相关的 `lock` 时，事务读取某个对象的值的过程中，这个对象的数值一定不会发生改变。也就是说，在这个过程中如果多次对该对象读取的话，每次读取的值都是一样的。而**删去读取锁则没有这层保障**


## Definition Issue

### LockRequestQueue

可以将 `LockRequestQueue` 中的 `request_queue_` 中的 `raw pointer` 修改为 `smart pointer`，具体看这里 [Discord](https://discord.com/channels/724929902075445281/1014055970634215434/1053312638014206093)

因此我们 `LockRequestQueue` 中的定义如下：

```cpp
  class LockRequestQueue {
   public:
    /** List of lock requests for the same resource (table or row) */
    std::list<std::shared_ptr<LockRequest>> request_queue_;
    /** For notifying blocked transactions on this rid */
    std::condition_variable cv_;
    /** txn_id of an upgrading transaction (if any) */
    txn_id_t upgrading_ = INVALID_TXN_ID;
    /** coordination */
    std::mutex latch_;
  };
```

### Lock Upgrade

对于 `lock upgrade`，其主要步骤如下：

> 具体参考 [`discord`](https://discord.com/channels/724929902075445281/1014055970634215434/1053318845240197170) 上 `TA` 给出的解释：
>
> moraceae — 2022/12/16 22:33
> 
> I do not remember where we say this (hopefully it's in the writeup or the code header. or lecture?), but upgrading can be decomposed into three operations:
> 
> - Is it possible to upgrade?
> 
> - Drop my current lock and reserve my position as the only person allowed to upgrade right now.
> 
> - Wait to get the new lock granted.
> 
> So in this case, when txn0 tries to S -> X, the lock manager notices that this is an upgrade. txn0 drops its S lock, reserves its position as the only one upgrade at the front of the ungranted queue, and waits. When txn2 unlocks, the next lock granted will be txn0's upgrade to X

详细的过程我们在下面的 `task 1` 中进行解释

## Task 1

`lock manager` 的主要工作是授予 `grant` 和释放 `release` 相应的 `lock`。这里的 `grant` 还有可能涉及到 `lock upgrade`

### Lock

对于同一个 `table` 而言，`LM` 需要记录**所有对该 `table` 持有 `lock` 的事务**和**所有对该 `table` 请求 `lock` 的事务**。换句话说在 `LockRequestQueue` 中，我们不仅仅需要记录那些还没获取 `lock` 的事务（也就是发起锁请求的那些事务），我们还需要记录那些已经获取 `lock` 的事务

> 一开始我认为只需要记录那些还没获取 `lock` 的事务，但这存在的问题是，在释放 `lock` 时我们无法断定该事务是否先前获取了该 `table` 的 `lock`，以及我们无法判断本次 `lock` 请求是否是一次 `lock upgrade`

我们按照如下步骤来实现 `lock table` 的操作：

1. 判断事务的状态是否能够获取 `lock`，依据不同的隔离类型，对该事务能够获取 `lock` 进行判断：

```
REPEATABLE_READ:
    The transaction is required to take all locks.
    All locks are allowed in the GROWING state
    No locks are allowed in the SHRINKING state

READ_COMMITTED:
    The transaction is required to take all locks.
    All locks are allowed in the GROWING state
    Only IS, S locks are allowed in the SHRINKING state

READ_UNCOMMITTED:
    The transaction is required to take only IX, X locks.
    X, IX locks are allowed in the GROWING state.
    S, IS, SIX locks are never allowed
```

2. 获取该 `table` 对应的 `LockRequestQueue`，如果**不存在则需要新建**，我们将其表示为 `pointer`
3. 先对 `pointer` 进行加锁（使用 `std::unique_lock`，因为后面我们需要用到信号量），然后判断当前 `lock request` 是否为 `lock upgrade`

我们依次遍历整个 `request queue`，对于遍历到的每一项 `iter` ，如果其 `grant` 为 `true` 并且其 `txn id` 等于当前事务的 `id`，那么我们进行以下判断：

- 如果 `iter` 的 `lock_mode` 和当前事务请求的 `lock_mode` 相同，说明这是一次重复的请求，直接返回 `true`
- 是否存在其他的事务在执行 `lock upgrade`（直接判断 `pointer->upgrading_` 即可），如果存在则直接 `abort` 并返回 `false`（我们 `abort` 之后也需要返回 `false`）

`lock upgrade` 主要是以下的几种情况：

```
IS -> [S, X, IX, SIX]
S -> [X, SIX]
IX -> [X, SIX]
SIX -> [X]
```

`lock upgrade` 的过程如下：

- 我们首先将这个 `lock` 给 `drop` 掉，也就是将其从队列中删去

- 然后，我们在队列的头部插入一个插入一个新的 `request`

> 关于为什么不允许在同一个对象上存在多个 `lock upgrade`，我认为的原因在于高级别的 `lock` 不允许共存
> 
> 假设两个事务 $T_1$ 和 $T_2$ 均持有 `S`，某时刻它们同时需要对该对象进行写入，因此它们会同时执行一次 `lock upgrade`，也就是 `S -> X`。但是，`X` 是无法多个事务共存的，因此我们必须要 `abort` 一个事务
>
> 这是我最开始的想法，但如果两个事务都是 `IS -> IX`，那么就是反例了

4. 检查完 `upgrade` 后，如果本次请求不是 `upgrade`，那么我们需要将其插入队列的尾部；而如果是 `upgrade`，则什么也不做（因为我们前面已经执行了插入）

在我们执行完插入操作后，如果当前的隔离级别为 `RR` 的话，需要将当前事务的状态设置为 `GROWING`

5. 随后，该线程调用 `wait` 函数等待条件的满足。这也是我卡最久的地方，因为需要考虑的情况实在是太多了

我们首先考虑最简单的情况，**在队列中不存在 `abort` 的事务所对应的请求**

> 这里需要说明一点，当我们用 `SetState` 将事务的状态设置为 `abort` 时，如果这个事务的请求已经存在于队列中，那么我们必须要将其移出队列，问题的关键是在哪里移出
>
> 事务的声明中有记录当前事务**已获取**的资源的 `lock`，全部都是用 `*_lock_set_` 来表示。由于这些都是表示事务已经获取的 `lock`，而事务在等待时如果被 `abort`，那么对应的 `*_lock_set_` 当中是没有对应值的
>
> 而在 `TransactionManager` 中的 `abort` 函数，则会释放所有当前事务**已经获取的 `lock`**，对于那些还没有获取但卡在队列中的请求，这里同样没办法将其删除
>
> 因此，我们只能在 `LockTable` 或者 `LockRow` 函数中将一个已经 `abort` 事务的请求移出队列

这个时候，我们有两点要求：

- 如果当前队列中存在 `granted` 的请求，那么我们需要检查当前事务的请求与其是否兼容
  - 如果有多个 `granted` 的请求，我们只需要取第一个即可，因为这些 `granted` 的请求之间是相互兼容的
- 如果当前队列中不存在 `granted` 的请求，那么我们需要保证**满足条件**的请求**都能**获得 `lock`
  - 满足条件有两点：
    - 若当前事务在队列的**队头**，那么可以获取 `lock`
    - 若当前事务不在队列的队头，但其获取请求与队头的请求相兼容，那么也可以获取 `lock`

也就是说，对于当前事务而言，**有三种能够获得 `lock` 的可能**。如果当前请求是 `lock upgrade`，那么我们在授予完 `lock` 之后还需要对 `pointer->upgrading_` 重新置为 `INVALID_TXN_ID`

只有**前两种可能**才允许 `lock upgrade`。第二种所对应的情况很简单，主要是第一种所对应的情况，具体的例子如下（默认都是同一个对象）：

```
txn0 (IS granted)
txn1 (IX granted)
txn2 (IX granted)

txn0 (IS -> IX)
```

初始时 `txn0` 持有 `IS`，`txn1` 和 `txn2` 持有 `IX`。此时 `txn0` 尝试 `lock upgrade`，由于此时只有一个锁升级请求并且 `IX` 之间可以共存，因此这是满足条件的

到此为止，我们已经可以实现基础的 `Lock Manager` 的功能，我们需要在此基础上加上 `abort` 事务的处理

一个直观的想法是，当线程在醒来之后先检查以下自己的状态，如果为 `abort` 那么将对应项从队列中删除，然后直接跳出 `wait` 的判断；然后，每个线程都会遍历这个队列找到对应的项，判断是否可以授予 `lock`

但这里有个问题是，当前线程只知道自己的状态是否为 `abort`，无法确定其他线程所对应的事务是否被 `abort`

并且由于线程唤醒后的执行是无法确定的，因此这会导致很大的问题：

```
txn0(abort):  X
txn1:         IS
txn2:         IS
```

当线程唤醒时，如果 `txn0` 的状态为 `abort`，另外两个事务正常。如果是 `txn0` 先被唤醒，那么它会先删除，然后 `txn1` 和 `txn2` 将会获取 `lock`，这不会出现问题

如果 `txn1` 先被唤醒，由于它无法确定 `txn0` 是否被 `abort`，因此它发现自己的请求无法与 `txn0` 相兼容，那么 `txn1` 将会被阻塞；同理，`txn2` 被先唤醒也是一样的情况。这就导致两个线程都被卡死，并且这不是 `deadlock`，因此 `deadlock detection algorithm` 不会 `abort` 掉任何一个事务

这个问题卡了我很久的时间，我所想出的解决办法是，如果当前线程判断自己的状态为 `abort`，在从队列中删除后，再调用一次 `notify_all` 函数来唤醒所有已经沉睡的线程

这么做是因为一个 `abort` 的事务在队列中，它要么不会照成影响，要么**只会导致所有其他的事务都被卡住**。因此我们直接 `notify_all` 一遍即可

> 我想不出在我前面设定的执行逻辑下，一个 `abort` 的事务能够照成除了卡死所有事务以外的影响。事实上，我将我的代码交上去之后能够通过所有的测试，那么这大概率就是正确的

6. 在 `wait` 函数的后面，我们需要**再次检查当前事务的状态是否为 `abort`**，之后就是对事务的 `*_lock_set_` 进行修改

`lock table` 的操作如上，`lock row` 的操作也类似

### Unlock

对于 `unlock` 的操作则相对简单很多

1. 通过 `table_lock_map` 找到对于的 `LockRequestQueue`，然后从队列中将当前事务对应的项删去。当然，我们需要判断当前的 `unlock` 操作是否满足条件，不满足则需要抛出异常并返回 `false`
2. 修改事务的 `lock set`，然后依据隔离级别将其状态修改为 `shrinking`，具体的要求如下：

```
TRANSACTION STATE UPDATE
   Unlock should update the transaction state appropriately (depending upon the ISOLATION LEVEL)
   Only unlocking S or X locks changes transaction state.

   REPEATABLE_READ:
       Unlocking S/X locks should set the transaction state to SHRINKING

   READ_COMMITTED:
       Unlocking X locks should set the transaction state to SHRINKING.
       Unlocking S locks does not affect transaction state.

  READ_UNCOMMITTED:
       Unlocking X locks should set the transaction state to SHRINKING.
       S locks are not permitted under READ_UNCOMMITTED.
           The behaviour upon unlocking an S lock under this isolation level is undefined.
```

3. 最后，调用 `notify_all` 函数唤醒所有的休眠线程

## Task 2

在 `deadlock detection` 函数中，我们不能对 `lock request queue` 中的项进行删除，只能简单地将对应的事务的状态置为 `ABORTED`，然后对所有的事务执行 `notify_all`

我们主要讨论死锁检测算法的实现，因为这是 `task 2` 中最关键的部分

我们的死锁检测算法需要从**最小的 `txn id` 开始**，然后输出图中存在的环，我们**需要 `abort` 的事务是环中 `txn id` 最大的那个**

举个例子：

现在图中存在两个环：

```
1 -> 2 -> 1

3 -> 4 -> 5 -> 3

```

这种情况下我们应当输出 `2`。下面，我们着手使用 $\text{DFS}$ 来设计这个算法

> 主要参考[这篇文章](https://www.geeksforgeeks.org/detect-cycle-direct-graph-using-colors/)

我们考虑用染色法来解决这个问题。图中一共有三种颜色：

- `0`：表示当前节点还没有被访问
- `1`：表示当前节点已经遍历过，但其**子节点还没有完全遍历**
- `2`：表示当前节点和其子节点已经全部遍历完毕

当我们在遍历的过程中，如果遍历到某个的为 `1`，那么就说明当前所走的路径中存在环。但我们还不能马上去整条路径的最大 `txn id`，具体的原因如下：

```
5 -> 2 -> 3 -> 4 -> 2
```

这种情况下，环为 `(2, 3, 4)`，但由于我们的起点为 `5`，并且 `5` 并不在环中，如果我们直接输出的话结果就是错误的，因此我们需要在遍历的时候维护一个邻接表，记录**每个节点的父节点**

然后，当我们确定这条路径上存在环时，我们首先从当前节点的父节点开始走，然后遍历邻接表，直到走到当前节点，这整条路径就是一个完整环，不会带有其他的节点

最后，我们输出环上的最大 `txn id` 即可

## Task 3





