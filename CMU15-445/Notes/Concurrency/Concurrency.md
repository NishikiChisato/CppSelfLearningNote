# Concurrency

- [Concurrency](#concurrency)
  - [Concurrency Control Thery](#concurrency-control-thery)
    - [Architecture](#architecture)
    - [Transaction](#transaction)
    - [Transaction Execution](#transaction-execution)
      - [Strawman](#strawman)
    - [Definition](#definition)
    - [Atomictiy](#atomictiy)
    - [Consistency](#consistency)
    - [Isolation](#isolation)
      - [Concepts](#concepts)
      - [Example](#example)
      - [Schedule](#schedule)
      - [Conflict](#conflict)
        - [Read-Write Conflicts](#read-write-conflicts)
        - [Write-Read Conflicts](#write-read-conflicts)
        - [Write-Write Conflicts](#write-write-conflicts)
      - [Serializability](#serializability)
        - [Conflict Serializablity](#conflict-serializablity)
          - [Example](#example-1)
        - [Dependency Graph](#dependency-graph)
      - [View Serializability](#view-serializability)
      - [Universe on Schedule](#universe-on-schedule)
    - [Durability](#durability)
    - [Conclusion](#conclusion)
  - [Two-Prase Locking (2PL)](#two-prase-locking-2pl)
    - [Intro](#intro)
    - [Type of Lock](#type-of-lock)
    - [Concept](#concept)
      - [Strong Strict 2PL](#strong-strict-2pl)
      - [Detection \& Prevention](#detection--prevention)
      - [Detection](#detection)
        - [Concept](#concept-1)
        - [Handing](#handing)
      - [Prevention](#prevention)
    - [Hierarchical Lock](#hierarchical-lock)
      - [Lock Granularity](#lock-granularity)
      - [Intention Lock](#intention-lock)
  - [Timestamp Ordering (T/O)](#timestamp-ordering-to)
    - [Basic Timestamp Ordering (T/O) Protocol](#basic-timestamp-ordering-to-protocol)
      - [Thomas Write Rule](#thomas-write-rule)
    - [Optimistic Concurrency Control (OCC)](#optimistic-concurrency-control-occ)


## Concurrency Control Thery

### Architecture 

`Database` 的组件从上至下依次为：

* `Query Planning`：解析 `SQL` 生成 `AST` 并对其进行 `optimize` 得到过程
* `Operator Execution`：实际执行 `physical plan`
* `Access Method`：定义底层数据的访问，通过 `B+Tree`、`Hash Table` 等
* `Buffer Pool Manager`：绕过 `OS` 对文件系统的管理，通过自定义的组件来实现文件的读写
* `Disk Manager`：与磁盘进行交互

并发控制 `concurrency control` 和恢复 `recorver` 分别跨越 `Operator Execution, Access Method` 和 `Buffer Pool Manager, Disk Manager`，如下图所示：

![Permeate](./img/Permeate.png)

`Concurrency control` 用于解决数据竞争 `data race` 的相关问题；`Recovery` 用于保证数据库系统的一致性 `data consistence`

### Transaction

事务 `Transaction` 是一系列操作的执行。例如，我们将多条 `SQL` 语句的执行看成一个整体，这个整体便是事务。`transaction` 是 `DBMS` 改变的基本单元，即只有一个完整的 `transaction` 对 `DBMS` 做出的修改才是有意义的，而不完整的事务 `partical transcation` 是不允许出现的

> A **transaction** is the execution of a sequence of one or more operations (e.g., SQL queries) on a database to perform some higher-level function. 
> 
> It is the basic unit of change in a DBMS:
> 
> → Partial transactions are not allowed!


因此，对于 `transcation` 中的操作而言，要么**全部被执行**，要么**一条都没有被执行**

举个具体的例子，假如 $A$ 要向 $B$ 转账 $100$ 元，那么事务为：

* 检查 $A$ 是否有 $100$ 元
* 将 $A$ 账户减去 $100$ 元
* 将 $B$ 账户增加 $100$ 元

### Transaction Execution

#### Strawman

关于事务如何执行，最简单的办法就是一次使用一个 `worker(or thread)` 来执行一个是事务。为了执行该事务，`DBMS` 会将整个 `database` 文件复制一份，然后对新复制的文件做修改。如果事务成功执行，那么新的文件将覆盖原先的文件；如果事务执行失败，那么新的文件将被丢弃

![StrawmanSystem](./img/StrawmanSystem.png)

这种方式非常的慢，因为它不仅无法做到事务的并发执行，还需要对每个事务都复制一遍整个数据库文件

我们希望对于独立的事务而言，它们可以并发地执行，因为这可以：

* 增大系统的利用率 `utilization` 和吞吐量 `throughput`
* 减小系统对用户的响应时间 `response time`

如果我们随意交叠 `arbitrary interleaving` 不同的操作的执行，那么会导致很多的问题，在开始分析哪些 `interleaving` 是有效的之前，我们需要一个形式上的定义

### Definition

一个 `transaction, txn` 可能会对数据库中的数据进行许多的操作，**`DBMS` 只关心那些对 `database` 进行读 `read` 和写 `write` 的操作**。因此，对于数据库 `database` ，我们给出以下形式化的定义：

![FormalDefinition](./img/FormalDefinition.png)

我们用 $A,B,C$ 来表示 `database` 中的数据对象 `data object`，用 $R(X),W(X)$ 来表示对对象 $X$ 进行读取和写入操作

在 `SQL` 中，`txn` 开始于 `begin` 字段，结束于 `commit` 或 `abort` 字段

![TransactionInSQL](./img/TransactionInSQL.png)

`transaction` 正确性的评价标准为 `ACID`，具体为：

![ACID](./img/ACID.png)

### Atomictiy

`Atomicity` 是指，`txn` 的所有操作要么被**全部执行**（对应 `commit`），要么**全部没有执行**（对应 `abort`）。一个 `txn` 被 `abort` 的对象，既可以是用户，也可以是系统。

![AtomicityForTxn](./img/AtomicityForTxn.png)

实现 `Atomicity` 的方法有两种：

* `Logging`：`DBMS` 在事务执行的过程中，每一步都会记录「如果要撤销这一步该怎么办」。这样的话如果事务被 `abort`，`DBMS` 也可以做到对已执行的操作进行撤销 `undo`。这种日志被称为回滚日志 `undo-log`，**在内存和硬盘中同时存在**

实际上，在我们对 `tuple` 的存储结构中，除了使用 `slotted plag` 以外，还可以使用 `log structured`。也就是我们每次对 `tuple` 的更新都只将操作类型和具体的更新数据写入内存，并不会实际去更新对应的 `tuple`。这样当我们需要撤销某个操作时，直接将先去的 `log` 给删去即可

![AtomicityLogging](./img/AtomicityLogging.png)

* `Shawod Paging`：对于那些 `txn` 要修改的 `page`，我们专门复制一份让其修改，等到 `txn` 提交后再将这些 `page` 替换回去

![ShadowPaging](./img/ShadowPaging.png)

这种思想本质上和 `strawman` 的一样，由于开销过大，因此很少系统会支持这种方法

### Consistency

由 `database` 所表示的「世界」在逻辑上 `logical` 是正确的，因此所有关于数据的询问所得到的结果也应该在逻辑上是正确的

![ConsistencyConcept](./img/ConsistencyConcept.png)

`Consistency` 分为两部分：

* `Database Consistency`

数据库表示现实世界中的实体必须要遵循其本身的约束。例如，如果表示年龄的话那么就不能为负数。此外，在未来的事务应该能看到过去**已执行**的事务的结果

![DatabaseConsistency](./img/DatabaseConsistency.png)

* `Transaction Consistency`

如果数据库在 `txn` 开始前是一致的 `consistent`，那么在事务结束后也应当是一致的

![TransactionCOnsistency](./img/TransactionConsistency.png)

### Isolation

#### Concepts

`DBMS` 会提供一种假象 `illusion`，使得每个事务都认为整个系统中只有它本身在运行。因此，每个事务只需要考虑数据库在事务开始时候的状态，而不需要考虑其他事务的执行是否会妨碍当前事务的执行

这种操作类似于操作系统的虚拟化，`OS` 本身没有那么大的内存空间，但它可以通过许多虚拟化的技术，让每个进程都认为自己能够使用 `64` 位的内存空间

![IsolationConcept](./img/IsolationConcept.png)

`DBMS` 通过运行不同事物中的操作交错 `interleaving` 执行来实现 `concurrency`。具体的 `concurrency protocol` 有两种实现方式：

* `Pessimistic`：悲观地认为问题发生的概率大，做出额外的操作让问题不发生
* `Optimistic`：乐观地认为问题发生是少数的，当问题发生时再解决

![ConcurrencyProtocol](./img/ConcurrencyProtocol.png)

#### Example

我们考虑下面这个例子：

![IsolationExample1](./img/IsolationExample1.png)

![IsolationExample2](./img/IsolationExample2.png)

`txn` 执行的顺序有非常多，但如果想要最终的结果满足 `consistency`，就必须要保证这次执行的净效应 `net effect` 等价于两个 `txn` 顺序 `serially` 执行的 `net effect`

以下的两个执行都能产生相同的 `logical outcome`，因此它们是等价的

![IsolationExample3](./img/IsolationExample3.png)

两个 `txn` 的调度图如下

![IsolationExample4](./img/IsolationExample4.png)

换句话说，两个 `txn` 的**顺序**执行是不会出现问题的

但为了提高磁盘的 `I/O` 以及提高 `CPU` 的利用率，我们需要让两个 `txn` 的执行产生 `interleaving`。这么做的好处是当一个 `txn` 需要等待某种资源时，系统内的其他 `txn` 可以继续执行

![IsolationExample5](./img/IsolationExample5.png)

这种情况下，由于其最终结果等价于顺序执行的结果，因此 `interleaving` 不会产生问题

![IsolationExample6](./img/IsolationExample6.png)

这种情况下，违背了 `consistency`，因此这应当被 `undo`

![IsolationExample7](./img/IsolationExample7.png)

如果我们站在 `DBMS` 的视角来看待这个 `schedule`，会发现第二个 `txn` 对第一个 `txn` 的中间结果进行了读取，因此这便造成了最终结果的错误。因为如果两个 `txn` 是顺序执行的话，那么后执行的 `txn` 必然不可能会读取到前一个 `txn` 的中间结果，而我们想要的效果就是两个 `txn` 交叠后所产生的结果等价于两个 `txn` 顺序执行所产生的结果

#### Schedule

那么，现在我们的问题是，我们该如何判断一个调度 `shcedule` 算法的正确性

![InterleavingProblem](./img/InterleavingProblem.png)

`DBMS` 执行操作的顺序被称为执行调度 `execution schedule`，我们的目标是生成与顺序执行 `serial execution` 等价的 `execution schedule`

在此之前，我们需要引入三个概念：

* `Serial Schedule` 序列化调度：调度 `shcedule` 是以序列化的形式执行的，不会 `interleaving` 两个不同 `txn` 的执行
* `Equivalent Schedule` 等价调度：如果第一个 `shcedule` 所产生的结果与第二个 `shcedule` 所产生的结果相同，这两个 `schedule` 是等价的 `equivalent`
* `Serializable Schedule` 可序列化调度：如果一个 `schedule` 与一个 `serial shcedule` 相等价 `equivalent`，那么该 `schedule` 为可序列化调度 `serializable schedule`

原文如下：

> • **Serial Schedule**: Schedule that does not interleave the actions of different transactions.
> 
> • **Equivalent Schedules**: For any database state, if the effect of execution the first schedule is identical to the effect of executing the second schedule, the two schedules are equivalent.
>
> • **Serializable Schedule**: A serializable schedule is a schedule that is equivalent to any serial execution of the transactions. Different serial executions can produce different results, but all are considered “correct”.

有了 `serializable schedule` 的概念后，我们可以很容易的判断一个 `schedule` 所产生的结果是否有效，只需要简单的判断其等价 `schedule` 是否是 `serial schedule` 即可

#### Conflict

那么现在的问题是，我们该如何判断两个 `schedule` 为等价的。为了实现这个目的，我们需要引入冲突 `conflict` 的概念

如果两个操作是 `conflict`，当且仅当：

* 它们来自**两个不同的 `txn`**
* 它们作用于**同一个对象**，并且**至少有一个是写入**

我们可以很容易地枚举出所有 `conflict` 的情况，一共只有三种：

> • **Read-Write Conflicts (“Unrepeatable Reads”)**: A transaction is not able to get the same value when reading the same object multiple times.
> 
> • **Write-Read Conflicts (“Dirty Reads”)**: A transaction sees the write effects of a different transaction before that transaction committed its changes.
> 
> • **Write-Write conflict (“Lost Updates”)**: One transaction overwrites the uncommitted data of another
concurrent transaction.

![ConflictingOperation](./img/ConflictingOperation.png)

##### Read-Write Conflicts

![Read-Write-Conflict](./img/Read-Write-Conflict.png)

$T_2$ 进行读取时，$T_1$ 还没有提交，因此会导致错误的结果

##### Write-Read Conflicts

![Write-Read-Conflict](./img/Write-Read-Conflict.png)

类似于第一个，$T_2$ 读取了 $T_1$ 还未提交的结果，如果 $T_1$ 最终是 `abort`，那么同样会导致错误的结果

##### Write-Write Conflicts

![Write-Write-Conflict](./img/Write-Write-Conflict.png)

两个 `txn` 都对同一个对象进行写入，因此会造成覆盖写入 `overwrite` 的问题

#### Serializability

有了 `conflict` 的概念后，我们可以判断一个 `schedule` 的可序列化性 `serializability`。`serializability` 有两个等级：

* `Conflict Serializability`：可理解为「基于冲突判断的可序列化性」
* `View Serializability`：可理解为「基于视图判断的可序列化性」

![Serializability](./img/Serializability.png)

##### Conflict Serializablity

对于 `conflict serializability` 而言，如果两个 `schedule` 满足以下条件，则称它们为冲突等价 `conflict equivalent`

* 两个 `schedule` 在相同的 `txn` 中具有相同的 `action`
* **每一对** `conflict action` 都以相同的顺序组织

如果一个 `schedule` $S$ 满足以下条件，则称它为冲突可序列化 `conflict serializable`

> 一个 `schedule` 由多个 `txn` 组成，这里我们只考虑两个的情况

* $S$ 冲突等价 `conflict equivalent` 于 `serial schedule`
* 我们可以通过在 $S$ 的两个不同的 `txn` 中交换**连续**的 `non-conflict operation`，将 $S$ 转换为 `serial schedule`

![ConflictSerializable](./img/ConflictSerializable.png)

我们简单总结一下：

* 我们首先知道一个 `serial schedule` 是一定满足条件的，因此我们需要判断一个 `schedule` 是否与 `serial schedule` 互为 `equivalent schedule`
* 在此我们引入了 `conflict` 的概念，如果一个 `schedule` 与一个 `serial schedule` 互为 `conflict equivalent`，那么我们可以认为它们互为 `equivalent schedule`
* 而为了判断 `conflict equivalent`，我们需要判断该 `schedule` 是否满足 `conflict serializable`

###### Example

举个例子：

![ConflictExample1](./img/ConflictExample1.png)

$R(B)$ 与 $R(A)$ 对**不同对象进行操作**，不满足 `conflict`，因此我们可以将其执行顺序进行交换，得到下图：

![ConflictExample2](./img/ConflictExample2.png)

同理，$R(B)$ 与 $R(A)$ 也是对**不同对象**进行操作，因此也不是 `conflict`，可以交换：

![ConflictExample3](./img/ConflictExample3.png)

$W(B)$ 与 $W(A)$ 作用于**不同对象**，也不是 `conflict`，可以交换：

![ConflictExample4](./img/ConflictExample4.png)

最后，我们交换 $W(B)$ 和 $R(A)$，得到最终结果：

![ConflictExample5](./img/ConflictExample5.png)

因此，原先的 `schedule` 与一个 `serial schedule` 是 `conflict equivalent`，也就是二者等价

我们来看另外一个例子：

![ConflictExample6](./img/ConflictExample6.png)

由于 $W(A)$ 和 $W(A)$ 互为 `conlict`，因此我们不能将其交换，换句话说左边的 `schedule` 无法转换成右边的 `serial schedule`

这种算法对于两个事务 `transaction, txn` 的情况很容易，但对于多个 `txn` 的处理则较为棘手

![SwapTransaction](./img/SwapTransaction.png)

为了解决这个问题，再次我们引入依赖图 `dependency graph` 来判断多个 `txn` 之间的调度是否等价于一个 `serial schedule`

##### Dependency Graph

对于两个 `txn` 而言，从 $T_i$ **指向** $T_j$ 的边 `edge` 定义为：

* $T_i$ 中的操作 $O_i$ 与 $T_j$ 中的操作 $O_j$ 构成 `conflict`
* 操作 $O_i$ 出现在操作 $O_j$ 之前

![DependencyGraph](./img/DependencyGraph.png)

如果一个 `schedule` 是 `conflict equivalent`，当且仅当整个 `dependency graph` 是**无环的 `acyclic`**

举个例子：

![GraphExample1](./img/GraphExample1.png)

由于 $W(A)$ 与 $R(A)$ 是对**同一个对象**进行操作，并且有一个是写入，因此二者为 `conflict`；由于 $W(A)$ 出现在 $R(A)$ 的前面，因此 $T_1$ 会有一条指向 $T_2$ 的边

由于 $W(B)$ 与 $R(B)$ 也是 `conflict`，并且 $W(B)$ 在 $R(B)$ 之前指向，因此 $T_2$ 会有一条指向 $T_1$ 的边

由于图中存在环，因此该 `schedule` 不是 `conflict equivalent`

另外一个例子：

![GraphExample2](./img/GraphExample2.png)

由于 $T_1$ 与 $T_3$ 的操作中满足 `conflict`，并且 $T_1$ 的操作先于 $T_2$ 的操作，因此 $T_1$ 会指向 $T_2$

> 注：实际上，只要在 $T_1$ 的执行过程中，$T_3$ 也在执行，并且 $T_1$ 的**某些**操作与 $T_3$ 的某些操作满足 `conflict`，就可以认定二者之间一定存在一条边，并不需要这些操作满足什么顺序：
> 
> * $T_1$ 的 $R(A)$ 可以与 $T_3$ 的 $W(A)$ 造成 `conflict`
> 
> * $T_1$ 的 $W(A)$ 也可以与 $T_3$ 的 $R(A)$ 造成 `conflict`
> 
> * $T_1$ 的 $W(A)$ 也可以与 $T_3$ 的 $W(A)$ 造成 `conflict`

![GraphExample3](./img/GraphExample3.png)

需要说明的是，使用 `dependency graph` 得到的结果有时不一定是正确的，这里我们与后面的 `view serializability` 的例子一起说明

#### View Serializability

由于没有系统支持这个，因此我们把注意力主要集中在后面的例子上，用于说明 `dependency graph` 有时会产生错误的结果

![ViewSerializability](./img/ViewSerializability.png)

![ViewExample1](./img/ViewExample1.png)

![ViewExample2](./img/ViewExample2.png)

在这个例子中，我们看出，`dependency graph` 是有环的，也就是说这个 `schedule` 不是 `conflict equivalent`。但是，由于 $T_3$ 对于 $A$ 的写入是最后执行的，因此这会**覆盖**前面的两次写入。换句话说，哪怕这不是与序列化调度 `serial shcedule` 冲突等价 `conflict equivalent`，它依旧等价于序列化调度 `serial schedule`

`view serializability` 相比于 `conflict serializability` 而言，对于 `schedule` 的限制更松，因此其有效执行 `enforce effectily` 也非常困难

这两种方式都无法直接给出一个 `schedule` 一定等价于 `serial schedule`，这是因为它们并不理解操作或者数据的含义（最后一个例子）。而 `conflict serializability` 被更多的系统所使用，原因在于这更容易实现

#### Universe on Schedule

所有的 `schedule` 的 `venn graph` 如下：

![UniverseSchedule](./img/UniverseSchedule.png)

### Durability

所有已提交的 `txn` 所造成的改变应当是永久的 `persistent`，即：

* 不存在断裂的 `torn` 更新（也就是只更新一半）
* 不允许一个已经 `abort` 的事务对系统进行更新

![Durability](./img/Durability.png)

### Conclusion

总结如下：

在实际的事务执行过程中，`concurrency control` 是自动进行的，这样可以减去事务编写的压力

![ConcurrencyConceptConclusion](./img/ConcurrencyConceptConclusion.png)

---

## Two-Prase Locking (2PL)

### Intro

在有了 `conflict serializable` 的概念后，我们已经可以判断一个 `schedule` 是否等价于 `serial schedule`。但这有一个问题是，我们需要执行一遍这个 `schedule` 才能知道到底是否等价（我们需要对某些 `non-conflict` 操作进行 `swap`）。我们现在的问题是，能否有一种办法能够在不实际执行的情况下，保证这个 `schedule` 等价于 `serial schedule`

![2PLIntro](./img/2PLIntro.png)

一个朴素的加锁方案如下：

![NaiveLock](./img/NaiveLock.png)

每次需要对 `share resource` 进行读取或写入等操作时，都需要进行 `lock` ；在操作完毕后，马上 `release`

`Lock` 和 `Latch` 的区别如下：

![LockLatch](./img/LockLatch.png)

### Type of Lock

`Lock` 有两种基本的 `type`，分别为：

> • **Shared Lock (S-LOCK)**: A shared lock that allows multiple transactions to read the same object at the same time. If one transaction holds a shared lock, then another transaction can also acquire that same shared lock.
> 
> • **Exclusive Lock (X-LOCK)**: An exclusive lock allows a transaction to modify an object. This lock prevents other transactions from taking any other lock (S-LOCK or X-LOCK) on the object. Only one transaction can hold an exclusive lock at a time.

在引入 `lock` 之后，`txn` 的执行过程如下：

![WithLock](./img/WithLock.png)

* `txn` 首先请求 `lock`
* `lock manager` 授予 `grant` 或阻塞 `block` 该 `txn`
* `txn` 释放 `lock`
* `lock manager` 更新内部的 `lock table`

如果我们在 `schedule` 内单纯使用 `lock`，实际上是无法避免 `conflict` 的：

![LockWithConflict](./img/LockWithConflict.png)

在这个例子中，我们第一次在使用完 $A$ 之后释放了锁，然后 $T_2$ 就对 $A$ 进行了 `overwrite`，而 $T_1$ 之后还会对 $A$ 进行一次写入，这将导致了 `conflict`

在此，我们引入两阶段锁 `two-phase locking (2PL)` 来解决这个问题

![ConceptOf2PL](./img/ConceptOf2PL.png)

### Concept

`Two-Phase Locking (2PL)` 的概念如下：

> Two-Phase locking (2PL) is a ***pessimistic*** concurrency control protocol that uses locks to determine whether a transaction is allowed to access an object in the database on the fly

![2PL](./img/2PL.png)

需要说明的是，`2PL` 是针对 `txn` 的，而一个 `schedule` 中会有多个 `txn` 交叠执行。对于 `lock` 的获取与释放，`2PL` 将其划分为两个阶段：

* `Growing Phase`：`txn` 在这个阶段中只能获取 `lock`。如果已经对某个资源执行完操作后，**不能**释放 `lock`
* `Shrinking Phase`：`txn` 在这个阶段中只能释放 `lock`

举个例子，如果某个 `txn` 需要依次操作对象 $A,B,C$，那么在操作对象前，就需要获取该对象的 `lock`。如果某时刻已经对 $A$ 操作完毕，但还没有对 $C$ 操作，那么 $A$ 的 `lock` 就不能释放，因为此时还处于 `growing phase`。只有当 $C$ 的 `lock` 已经获取完毕后，我们才可以释放**已经执行完毕的对象的 `lock`**

![2PLDiagram](./img/2PLDiagram.png)

在 `shrinking phase` 中，呈阶梯下降的原因是他会**依次释放已经执行完毕的资源的 `lock`**

上面那个例子中，我们引入 `2PL` 后，结果如下：

![With2PL](./img/With2PL.png)

在第一次对 $A$ 执行完 $R$ 和 $W$ 后，由于之后还需要对 $A$ 执行操作，因此我们不会在这个时候释放 `lock`，而是等到对 $A$ 执行完所有操作后再释放。这样可以避免出现 `conflict`

`2PL` 可以保证所产生的 `dependence graph` 是满足 `acyclic`，但 `2PL` 却会导致级联终止 `cascading aborts`

![2PLProblem](./img/2PLProblem.png)

我们看下面这个例子：

![CascadingAborts](./img/CascadingAborts.png)

$T_1$ 已经对 $A$ 执行完所有操作并且释放了 $A$ 的 `lock`，而此时 $T_2$ 则开始对 $A$ 进行读取和写入。如果之后 $T_1$ 被 `abort`，那么这就会导致 $T_2$ 也需要被 `abort`。因为 $T_2$ 读取了一个本不该被读取的值（这被称为 `dirty read`）。实际上，这个 `abort` 可以**一直传递下去**，这便是 `cascade`

除了 `cascading abort`，`2PL` 还会导致 `deadlock` 的问题

![2PLObservation](./img/2PLObservation.png)

* 为了解决 `cascading abort`，我们引入 `Strit 2PL`
* 为了解决 `deadlock`，我们引入 `detection` 和 `prevention`

#### Strong Strict 2PL

`strict schedule` 和 `strong strict 2PL` 的定义如下：

> A schedule is **strict** if any value written by a transaction is **never read or overwritten by another transaction until the first transaction commits**. 
> 
> **Strong Strict 2PL** (also known as Rigorous 2PL) is a variant of 2PL where the transactions **only release locks when they commit**.

![Strict2PL](./img/Strict2PL.png)

我们在 `txn` 提交的时候再释放所有的 `lock`，这便是 `strong strict 2PL`，这么做的好处有两点：

* 可以避免 `cascading abort`
* 对于那些已经 `abort` 的 `txn` 而言，我们只需要恢复最开始的那个 `txn` 开始时的状态，而不需要去关注中间那些 `txn` 所造成的改变。换句话说，我们需要记录的仅仅是这个 `txn` 开始时的状态，如果这个 `txn` 一旦发生 `abort`，那么我们直接恢复到那个状态即可

举个例子：

![Strict2PLSample1](./img/Strict2PLSample1.png)

不引入 `2PL`，则可能会导致出错：

![Strict2PLSample2](./img/Strict2PLSample2.png)

引入 `2PL`，但有可能导致 `cascading abort`（但这里并没有显示出来），运行结果如下：

![Strict2PLSample3](./img/Strict2PLSample3.png)

引入 `Strong Strict 2PL`，其结果如下：

![Strict2PLSample4](./img/Strict2PLSample4.png)

我们加入 `2PL` 后，`schedule` 的整体 `veen` 图如下：

![UniverseScheduleWith2PL](./img/UniverseScheduleWith2PL.png)

#### Detection & Prevention

下面是关于 `Detection` 和 `Prevention` 的内容：

![DetectionPrevention](./img/DetectionPrevention.png)

实际上，哪怕我们引入了 `Strong Strict 2PL`，我们还是会遇到 `deadlock` 的问题，也就是下图：

![DeadLock](./img/DeadLock.png)

哪怕我们的加锁方式满足 `2PL`，但我们依然有可能会出现循环等待

解决方法如下：

![2PLDeadlock](./img/2PLDeadlock.png)

#### Detection

死锁检测是指，我们在发生了 `deadlock` 的时候，考虑去将 `deadlock` 给打破

##### Concept

我们创建一个 `warts-for graph`，用于记录每个 `txn` 需要等待的 `lock`，图满足如下条件：

- 每个节点都是 `txn`
- 从 $T_i$ 指向 $T_j$ 的边，表示 $T_i$ 等待 $T_j$ 释放 `lock`

如果图中存在环，则说明发生死锁。此时系统需要判断该如何打破这个环。需要注意的是，以何种频率去检测这个环和死锁发生直到被打破的时间之间，是一个 `trade-off`

![Detection](./img/Detection.png)

我们看一个例子：

![DetectionSample](./img/DetectionSample.png)

$T_1$ 在等待 $T_2$ 释放 `lock`，因此会有一条**从 $T_1$ 指向 $T_2$ 的边**；同理，$T_3$ 在等待 $T_1$，因此会有一条从 $T_3$ 指向 $T_1$ 的边

##### Handing

当我们检测到环时，则需要选择一个 `victim txn` 来进行回滚 `rollback`。而被选中的 `txn` 要么重启要么 `abort`

![DetectionHanding](./img/DetectionHanding.png)

关于这个 `victim txn` 的选择，我们需要考虑的因素有很多，原因在于我们希望 `rollback` 这个 `txn` 对系统造成的开销尽可能小，需要关注的点如下：

- 这个 `txn` 的起始时间（我们不希望选择一个已经执行了很长时间的 `txn`）
- 这个 `txn` 已经执行了多少语句（我们不希望选择一个已经执行了很多语句的 `txn`）
- 这个 `txn` 已经获取了多少 `lock`（我们希望选择一个获取了更多 `lock` 的 `txn`，因为让它 `rollback` 后，系统中的其他 `txn` 在执行时被阻塞的概率会降低）
- 如果选择这个 `txn`，那么我们需要 `rollback` 多少 `txn`（我们也是希望选择一个尽可能少 `rollback` 的 `txn`）

![VictimSelect](./img/VictimSelect.png)

在我们选定一个 `txn` 之后，有两种 `rollback` 策略：

> 注：我们 `rollback` 的对象是**这个 `txn`**中的操作，我们不会 `rollback` 其他的 `txn`，这里要与 `cascading aborts` 区分

- `Completely`：完全回滚，我们将这个 `txn`，使其好像没有被执行过
- `Partial`：部分回滚，我们只 `rollback` 该 `txn` 的部分操作，使得 `graph` 中的环被打破即可

![RollbackLength](./img/RollbackLength.png)

#### Prevention

死锁预防是指，我们会尽力去避免 `deadlock` 的发生

![Prevention](./img/Prevention.png)

如果某个 `txn` 试图去获取一个已经被其他 `txn` 获取过的 `lock`，由于这种操作**有可能**会导致 `deadlock`，因此我们直接将这个 `txn`（或者那个获取了 `lock` 的 `txn`） 给 `abort`

> 可以看出，`detection` 为 `optimistic`，`prevention` 为 `pessimistic`

由于我们可以 `abort` 的对象有两个，因此这便引申出两种不同的策略：

- 我们会依据时间戳 `timestamp` 来为每个 `txn` 赋予优先级。时间戳越早优先级越大
- `Wait-Die`：如果高优先级的 `txn` 请求低优先级的 `txn` ，那么高优先级的等待；如果低优先级的 `txn` 请求高优先级的 `txn`，那么低优先级的 `abort`
- `Wound-Wait`：如果高优先级的 `txn` 请求低优先级的 `txn`，那么低优先级的 `abort`；如果低优先级的 `txn` 请求高优先级的 `txn`，那么低优先级的等待

> 需要说明的是，我们 **`abort` 的对象只能是低优先级的 `txn`**。换句话说，高优先级的 `txn` 要么执行，要么等待；而低优先级的 `txn` 要么执行，要么等待，要么 `abort`
>
> - 在 `Wait-Die` 中，只会出现高优先级等待低优先级，不会出现低优先级等待高优先级
> - 在 `Wound-Wait` 中，只会出现低优先级等待高优先级，不会出现高优先级等待低优先级
>
> 即，等待的方向总是**单向的**

![DeadlockPrevention](./img/DeadlockPrevention.png)

我们看一个例子：

![PreventionSample](./img/PreventionSample.png)

由于 $T_1$ 的执行先于 $T_2$ 的执行，因此：

- 在 `Wait-Die` 下，我们会优先保证 $T_2$ 的执行，也就是 $T_1$ 在无法获取 `lock` 时，我们会阻塞 $T_1$
- 在 `Wound-Wait` 下，我们会优先保证 $T_1$ 的执行，也就是 $T_2$ 在尝试获取 `lock` 时，我们会直接 `abort` 掉 $T_2$

关于 `Prevention`，有两点需要注意：

- 采用这种方法后，我们等待的方向只有一个，因此在图中不会出现环
- 当一个 `txn` 重启后，我们**不能改变其优先级**，否则会出现饥饿 `starve` 的情况

![PreventionQuestion](./img/PreventionQuestion.png)

### Hierarchical Lock

#### Lock Granularity

在讨论完 `2PL` 后，我们接着讨论一下锁的细粒度 `lock granularity`

当某个 `txn` 试图修改很多个 `tuple` 时，由于每一个 `tuple` 都需要有一个 `lock` 来保护，因此这个 `txn` 便需要不断地获取 `lock`，这会造成很大的性能开销

![LockGranularity](./img/LockGranularity.png)

因此，我们可以加大锁的粒度 `granularity`，对更大范围的对象（`attribute, tuple, page, table`）提供更高层次的 `lock`，用于减少 `lock` 的获取

![GranularityObservation](./img/GranularityObservation.png)

锁的细粒度之间的 `trade-off` 体现在，如果细粒度过大，那么系统的并发性会降低；如果细粒度过小，那么获取 `lock` 的次数会增大

锁的层次结构 `Lock Hierarchy` 如下:

1. Database level (Slightly Rare)
2. Table level (Very Common)
3. Page level (Common)
4. Tuple level (Very Common)
5. Attribute level (Rare)

![LockHierarchy](./img/LockHierarchy.png)

当我们需要获取某一节点的 `lock` 时，需要检测其**所有子节点**的 `lock` 是否被获取，如果没有那么我们才可以获取该节点的 `lock`。也就是，我们需要遍历其所有的子节点

#### Intention Lock

遍历所有子节点的开销过大，我们在此引入 `Intention Lock` 的概念

> **Intention locks** allow a higher level node to be locked in shared mode or exclusive mode without having to check all descendant nodes. If a node is in an intention mode, then explicit locking is being done at a lower level in the tree.
>
> • **Intention-Shared (IS)**: Indicates explicit locking at a lower level with shared locks.
> 
> • **Intention-Exclusive (IX)**: Indicates explicit locking at a lower level with exclusive or shared locks.
>
> • **Shared+Intention-Exclusive (SIX)**: The sub-tree rooted at that node is locked explicitly in shared mode and explicit locking is being done at a lower level with exclusive-mode locks.

意象锁 `intention lock` 分为三类：

- `Intention-Shared`：表明对于当前节点和其**部分**子节点将被**显式**锁定为 `Shared`
- `Intention-Exclusive`：表明对于当前节点和其**部分**子节点将被**显式**锁定为 `Exclusive`
- `Shared+Intention=Exclusive`：表明对于当前节点被显式锁定为 `Shared`，其**部分**子节点将被**显式**锁定为 `Shared` 或 `Exclusive`

各个锁之间的互斥关系如下：

![LockMatrix](./img/LockMatrix.png)

`Intention Lock` 的关键在于，我会对当前节点的**部分**子节点施加什么类型的 `lock`，因此：

- 对于 `IS` 而言，说明当前节点和其部分子节点将会被显式锁定为 `shared`，而其他的节点将不受影响。因此我们可以获取在其基础上 `IS, IX, S, SIX`
- 对于 `IX` 而言，说明当前节点和其部分子节点将会被显式锁定为 `exclusive`，所以我们可以获取 `IS, IX`，但我们无法获取 `S, SIX, X`。因为 `S, SIX, X` 均不能与 `X` 共存

具体的获取协议如下：

![IntentionLockProtocol](./img/IntentionLockProtocol.png)

我们来看个例子：

![IntentionLockSample1](./img/IntentionLockSample1.png)

$T_1$ 将会读取和更新，因此它对上层节点获取 `SIX`，这说明当前这棵树的根节点为锁定为 `shared`，其子节点**将被锁定**为 `exclusive`

![IntentionLockSample2](./img/IntentionLockSample2.png)

$T_2$ 只会读取某些 `tuple`，因此需要对根节点获取 `IS`，这可以与 `SIX` 共存

![IntentionLockSample3](./img/IntentionLockSample3.png)

由于 $T_3$ 需要读取所有的 `tuple`，这不满足只对部分进行操作，因此我们需要获取 `S`，者将被阻塞

![IntentionLockSample4](./img/IntentionLockSample4.png)

当 $T_2$ 执行完毕后，根节点的 `SIX` 锁被释放，$T_3$ 才可以开始执行

![IntentionLockSample5](./img/IntentionLockSample5.png)

在我们引入 `Intention Lock` 之后，不同的 `lock` 之间便有了细粒度 `granularity` 的区别

![MultipleGranularity](./img/MultipleGranularity.png)

`DBMS` 将会自动转换 `lock` 的细粒度，这样可以避免多次获取较低细粒度的锁

![LockEscalation](./img/LockEscalation.png)

---

## Timestamp Ordering (T/O)

关于 `T/O` 的定义，`lecture notes` 中给出的说明很精辟：

> **Timestamp ordering (T/O)** is an **optimistic** class of concurrency control protocols where the DBMS assumes that transaction conflicts are **rare**. Instead of requiring transactions to acquire locks before they are allowed to read/write to a database object, the DBMS instead uses timestamps to determine the serializability order of transactions.

我们会对每个 `txn` $T_i$ 分配一个**唯一固定**的 `timestamp` $TS(T_i)$，并且 `timestamp` 是单调递增的

关于 `timestamp` 的实现，有许多的方式：

- 使用系统时钟 `system clock`，但二月的天数会发生改变，因此会有边界问题
- 使用逻辑计数器 `logical counter`，但问题在于随着时间的累加这会导致溢出；并且在分布式系统中我们需要跨越多个机器来维护计数器
- 使用二者的混合 `hybrid`

![TimestampAllocate](./img/TimestampAllocate.png)

### Basic Timestamp Ordering (T/O) Protocol

我们不使用 `lock` 来读取或写入数据，我们通过对每个对象 `object` 赋予一个 `timestamp` 来序列化 `txn` 的执行

- $W-TS(X)$ 表示对于对象 $X$，上一个成功执行的读取事务的 `timestamp`
- $T-TS(X)$ 表示对于对象 $X$，上一个成功执行的写入事务的 `timestamp`

我们的原则是，**不允许 `txn` 访问未来读取或写入过的对象**

![BasicT_O](./img/BasicT_O.png)

对于读取，规则如下：

- 如果当前事务的 $TS(T_i)$ 小于读取对象的 $W-TS(X)$，说明我们在对一个**已经在未来修改过的对象**进行读取，这是不允许的，因此我们需要 `restart` 这个事务，并赋予一个**新的 `timestamp`**
- 否则，我们则对该对象进行读取，并将 $R-TS(X)$ 更新为 $\max(R-TS(X), TS(T_i))$；随后在将 $X$ 拷贝一份以便于 $T_i$ 后续的读取（$T_i$ 有可能会读取多次，我们不需要每次读取都做一次判断）

![BasicT_ORead](./img/BasicT_ORead.png)

对于写入，规则如下：

- 如果当前事务的 $TS(T_i)$ 小于该对象的 $R-TS(X)$ 并且小于该对象的 $W-TS(X)$，那么说明我们在对一个**已经在未来读取过并且写入过的对象进行修改**，这同样是不允许的，因此我们需要 `restart` 这个事务，并赋予一个**新的 `timestamp`**
- 否则，允许 $T_i$ 去写入 $X$ 并更新 $W-TS(X)$；随后拷贝一份 $X$ 以便后续 $T_i$ 的读取

![BasicT_OWrite](./img/BasicT_OWrite.png)

我们来看一个简单的例子：

![BasicSample1](./img/BasicSample1.png)

![BasicSample2](./img/BasicSample2.png)

![BasicSample3](./img/BasicSample3.png)

![BasicSample4](./img/BasicSample4.png)

这里虽然 $TS(T_1)\lt TS(T_2)$，但不会发生冲突，因为读取冲突的对象是 $W-TS(X)$

![BasicSample5](./img/BasicSample5.png)

![BasicSample6](./img/BasicSample6.png)


#### Thomas Write Rule

对于**写入**的情况，原先的条件为：当满足 $TS(T_i)\lt R-TS(X)\ \&\&\ TS(T_i)\lt W-TS(X)$，我们将 `txn` 给 `abort` 掉，我们将此条件修改为：

- 如果 $TS(T_i)\lt R-TS(X)$，说明当前事务**写入**的对象**在未来被读取过**，而当前事务是要对该对象进行修改，那么我们只能将当前事务 `abort` 掉以保证未来读取的值不会发生改变
- 如果 $TS(T_i)\lt W-TS(X)$，说明当前事务**写入**的对象**在未来被写入过**，而由于当前事务对该对象的写入会被未来的那次写入所覆盖，因此我们可以之间**忽略**这次写入，继续执行当前事务
  - 这虽然违背了 $T_i$ 中的 `timestamp`，但由于后续 $T_i$ 只会**读取本地的值**，因此不会产生什么影响
- 如果不满足以上两个条件，那么我们更新 $W-TS(X)$


![ThomasWriteRule](./img/ThomasWriteRule.png)

我们看个具体的例子：

$T_1$ 先于 $T_2$ 执行，因此前者的 `timestamp` 比后者小，我们最终要求得到的结果是与按**序列化**执行 $T_1$ 和 $T_2$ 的结果相同

![ThomasSample1](./img/ThomasSample1.png)

这里发生了冲突，但我们直接忽略这条写入语句，继续去执行后续的读取。在执行读取的时候，我们会对 $T_1$ 执行一次 `abort`，然后最终得到的结果等价于顺序执行 $T_1$ 和 $T_2$ 的结果

![ThomasSample2](./img/ThomasSample2.png)

如果我们不采用 `Thomas Write Rule`，那么 `T/O` 得到的 `schedule` 就是 `conflict schedule` 的，因为这不会产生 `deadlock`（不会发生循环等待的情况）

![BasicT_OProblem](./img/BasicT_OProblem.png)

但这依然存在问题：有可能会发生饥饿 `starvation`。考虑一个需要执行很长时间的 `txn`，那么它需要更新的对象必然会被后续的 `txn` 所更新，换句话说那些对象会具有更大的 `timestamp`，那么我们就需要去 `restart` 这个 `txn` 以保证 `schedule` 是顺序的。但如果不断地有「年轻」的 `txn` 到来，那么这个过程将会不断重复，这便导致了 `starvation`

![BasicT_OIssue](./img/BasicT_OIssue.png)

### Optimistic Concurrency Control (OCC)








