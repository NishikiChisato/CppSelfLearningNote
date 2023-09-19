# Project 1

- [Project 1](#project-1)
  - [Task 1](#task-1)
    - [Semantic understanding](#semantic-understanding)
    - [Data member \& Detail of implementation](#data-member--detail-of-implementation)
    - [Test](#test)
  - [Task 2](#task-2)
    - [Semantic Understanding](#semantic-understanding-1)
  - [Task 3](#task-3)
  - [Harvest](#harvest)
  - [To do](#to-do)


## Task 1

`Task 1` 要求我们需要实现一个 `LRU-K` 算法（实现 `LRUKReplacer`），以此来作为 `Buffer Pool Manager` 的替换算法

### Semantic understanding

`LRU-K` 的基本控制单位为 `frame`，通过计算每个 `frame` 的 `k-distence` 来决定要将哪些 `fream` 驱逐出去

对于访问次数小于 $k$ 的 `frame` 而言，其 `k-distence` 被设置为 `inf`；对于访问次数大于等于 $k$ 的 `frame` 而言，其 `k-distence` 的值为**当前的时间戳减去上次访问的时间戳**

此外，若有多个 `k-distence` 为 `inf`，则驱逐出**总体访问时间最早的那个**，也就是具有最早访问时间的 `frame`

这一段在 `Writeup` 上面的描述如下：

>The LRU-K algorithm evicts a frame whose backward k-distance is maximum of all frames in the replacer. Backward k-distance is computed as the difference in time between current timestamp and the timestamp of kth previous access. A frame with fewer than k historical accesses is given +inf as its backward k-distance. When multiple frames have +inf backward k-distance, the replacer evicts the frame with the earliest overall timestamp (i.e., the frame whose least-recent recorded access is the overall least recent access, overall, out of all frames).

需要说明的是，这里的**总体访问时间最早**是指该 `frame` 具有**最小的时间戳**

### Data member & Detail of implementation

这里涉及的类有两个，主要的成员含义如下：

* `LRUKNode`：用于记录每个 `frame` 的访问历史，在 `LRUKReplacer` 中有一个哈希表用于存储每个 `frame` 与 `LRUKNode` 之间的映射
  * `history_`：该 `frame` 的访问历史
  * `k_`：`LRU-K` 算法中 `k` 的大小
  * `fid_t`：该 `node` 对应的 `frame` 的值
  * `is_evictable`：该 `frame` 是否可以被 `evict` 
* `LRUKReplacer`：实际提供 `LRUK` 算法的类；管理的对象的单位为帧 `frame_id_t`
  * `node_store_`：一个哈希表，用于记录当前的 `frame id` 与其访问历史之间的映射

我们实现的 `replacer` 中的 `frame_id_t` 是一个 `size_t`，实际上其他的类型也可以

真正需要注意的是对于 `evict` 函数的理解，由于我们区分了 `k-distence` 为 `inf` 与非 `inf` 的情况，因此我们在计算 `k-distence` 时也需要分开

**对于哪些 `k-distence` 大于等于 `k` 的 `frame` 而言，我们取的是最大的 `k-distence`；而对于哪些 `k-distence` 为 `inf` 的 `frame` 而言，我们取的是具有最小的 `frame`**

只要能理解到这里，那么这个也就很容易了

### Test

测试时需要将 `gtest` 的 `DISABLE_` 标签去掉，在文件 `lru_k_replacer_test` 中：

```c++
TEST(LRUKReplacerTest, DISABLED_SampleTest)
```

修改为：

```c++
TEST(LRUKReplacerTest, SampleTest)
```

---

## Task 2

`Task 2` 是真正去实现一个 `Buffer Pool Manager, bpm`，也是这个 `Project` 的难点所在

### Semantic Understanding

`bpm` 需要为上层的应用程序提供 `page`，并且它自身需要控制将 `page` 映射到 `frame` 以及控制 `frame` 从内存到磁盘的换入与换出

对于上层的应用程序而言，它们可以向 `bpm` 申请 `page`，然后对该 `page` 进行写入以及在需要时进行读取

对于 `bpm` 而言，它将每个 `page` 都映射到唯一的一个 `frame`，这里的 `frame` 是实际在物理内存当中的物理帧 `physical frame`。也就是说，`bpm` 与操作系统一样提供一层在 `physical frame` 与 `virtual page` 之间的抽象

`bpm` 为上层应用程序所提供的 `page id` 的值从 `0` 开始递增，但只要条件允许，这个 `page id` 就不会存在上界；而 `bpm` 的 `frame id` 的数量**固定**为 `bpm` 的大小，由于这是实际存储数据的 `physical frame`，因此其数量必然固定

`bpm` 的 `page_table_` 用于提供从 `page id` 到 `frame id` 的映射，而 `free_list_` 用于实际存储空闲的 `frame id`

`NewPage` 说明上层应用程序希望向 `bpm` 申请一个新的 `page` 进行读写，只要 `bpm` 还有空闲 `frame` 或者可以逐出某个 `frame` 所对应的 `page`（将这个 `page` 的内容写入到磁盘，那么这个 `frame` 就暂时空闲了），那么它就可以满足上层应用程序的需求

需要说明的是，无论是从 `free_list` 当中获取 `frame` 还是直接 `evict` 所得到的 `frame`，都需要**从新对 `page_table` 进行赋值**

`FetchPage` 说明上层应用程序希望读取一个已经存在的 `page`，这时 `bpm` 需要通过该 `page id` 将对应的 `page` 的数据加载到内存中（如果允许的话），然后向上层应用程序返回该页

在该函数中，一个空闲的 `frame` 的来源有两种：直接从 `free_list` 中读取；`evict` 掉某个 `frame`。无论是哪种方式，**都需要对 `page_table` 从新赋值**。当然，如果当前的 `buffer pool` 中有，那么就不需要重新赋值

> 这里需要说明一点，`comment` 中关于 `page` 所在位置的表述有两种： `in buffer pool` 和 `in page table` 是不一样的，前者理解为当前内存中存在；而后者理解为在 `page table` 中存在，也就是说这个 `page` 可能不在内存当中

`UnpinPage` 是用于上层应用程序对该 `page` 完成操作后调用的函数。上层应用程序通过 `new` 或者 `fetch` 获取一个 `page` 后，对其进行操作，最后调用 `UnpinPage`，**并设置该 `page` 的 `is_dirty` 位**

需要说明的是，`UnpinPage` 不能将一个 `page` 的 `is_dirty` 从 `true` 改为 `false`，也就是说它只能对 `is_dirty` 为 `false` 的 `page` 进行设置

> 在 `IsDirty` 测试中，我通过 `log` 发现了这个问题，也就是先 `unpin` 设置为 `true`，然后 `fetch` 之后在 `unpin` 设置为 `false`，我们的 `unpin` 需要禁止这种行为

`FlushPage` 用于提供一个强制将 `page` 写入到磁盘的语义，无论 `is_dirty` 位是 `false` 还是 `true`。另外，我们不能在 `FlushAllPages` 中为每个 `page` 单独调用 `FlushPage`，这会照成死锁

`DeletePage` 用于直接将一个 `page` 删除，也就是说该应用程序不会再次使用该 `page id`。该函数只能删除哪些在 `buffer pool` 中的 `page`，对于那些在磁盘中的 `page`，该函数**不会将它们读取到内存中然后再删去**

---

## Task 3

在前面的描述中，我们知道，`NewPage` 或者 `FetchPage` 必须与 `UnpinPage` 搭配使用，否则会出现某个 `page` 始终在内存当中不会被驱逐出去的情况

因此此处引入 `BasicPageGuard`，用于帮助程序员不需要手动 `unpin`

与此同时，对于一个 `page` 的读取和写入的加锁和解锁也是一对的，同样会发生这种问题，因此 `ReadPageGuard` 用于读取页面时帮助程序员自动解锁，`WritePageGuard` 用于写入页面时帮助程序员自动解锁

在 `bpm` 中，`NewPageGuarded` 和 `FetchPageGuarded` 都会返回一个 `BasicPageGuard`，在对 `BasicPageGuard` 进行操作时，其内部会有一个 `is_dirty_` 参数用于自动调用 `UnpinPage` 时的实参

因此如果我们对该 `page` 操作后我们期望 `is_dirty` 为 `true`，那么我们需要调用 `AsMut` 这个接口；如果我们期望 `is_dirty` 为 `false`，那么我们需要调用 `As` 这个接口

> 返回值为 `reinterpret_cast<dst>(src)`，该操作符会用 `dst` 类型重新解释 `src` 变量

在 `BasicPageGuard` 的 `Drop` 函数中，我们直接调用一次 `UnpinPage` 即可，当然为了避免在析构函数中再次调用，我们需要将 `bpm` 和 `page` 指针均设置为 `nullptr`

在 `BasicPageGuard` 的 `Move constructor` 中，我们会**默认当前的对象还未生成**，因此我们直接将 `this` 的相关属性设置为 `that` 的相关属性即可。全部完成后，需要将 `that` 的两个指针设置为 `nullptr`，避免重复析构

在 `BasicPageGuard` 的 `Move operator=` 中，**此时当前对象已经生成**，因此我们需要先对 `this` 所值对象执行一次 `UnpinPage`，然后再将 `this` 的相关属性设置为 `that` 的相关属性。完成后同样需要将 `that` 的两个指针设置为 `nullptr`

---

## Harvest

独立完成了这个实验，对我的提升真的很大，在此写下我的一点经验

1. 确保我对于某个函数行为的理解与该函数实际的行为不存在 `semantic` 上的区别
2. 将一个类中的所有函数的行为全部理解过后再开始敲代码，这些函数可能存在调用上的联系（这个实验中每次获取一个 `page` 后都需要 `unpin`）
3. 在 `guardscope` 中的 `log`，可以试图在本地复现，或者理解该 `bug` 是如何产生的

> `guardscope` 交了七十多次，七十啊七十

## To do

`innoDB` 中的 `bpm` 有两个常见优化：`prefetch` 和 `asyncio`