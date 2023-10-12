# Project 2

- [Project 2](#project-2)
  - [Basic Concepts](#basic-concepts)
    - [Flexible layout](#flexible-layout)
    - [Page Layout](#page-layout)
    - [Binary Search](#binary-search)
  - [Insert](#insert)
    - [Split Leaf](#split-leaf)
    - [Split Internal](#split-internal)


## Basic Concepts

### Flexible layout

假设 `B+ Tree` 的 `leaf node` 与 `internal node` 的最大大小分别为：`leaf_max_size` 和 `internal_max_size`，那么：

* `leaf node` 的元素个数最少为：$\lceil (n-1)/2 \rceil$，最多为：$n-1$
* `internal node` 的元素个数最少为：$\lceil n/2 \rceil$，最多为：$n$ 

> 这里有一个问题是，`internal node` 的第一个 `key` 为空，那么其对应的元素个数是否应该也发生改变。实际上，这个问题关系到 `internal node` 的理解，后面有答案

在我们的 `page` 的定义中，无论是 `internal page` 还是 `leaf page`，都是以一个 `pair<KeyTYpe, ValueType>` 为类型的 `flexible array` 作为结尾，因此**二者本质上对于每个元素所采取的存储方式都是一样的**

换句话说，对于 `leaf page`，`flexible array` 中的每一个下标都对应存储一个 `pair`，而 `internal page` 当中的 `flexible array` 中的每一个下标也对应存储一个 `pair`，**唯一不同的是 `internal page` 的第一个 `pair` 的 `first` 不存储元素**

因此，无论是 `internal page size` 还是 `leaf page size`，记录的都是当前该 `page` 已经存储的 `pair` 的数量，不会出现在实际存储的 `ValueType` 一样的情况下，`internal size` 比 `leaf size` 要少一个的情况，二者的下标均是从 $0$ 到 $internal/leaf size - 1$ 

对于二分查找而言，我们查找的对象是 `KeyType`，而 `KeyType` 是存储在 `pair` 的 `first` 部分里面，因此对于 `internal page`，我们需要查找的范围为：$[1,n)$；而对于 `leaf page`，我们需要查找的范围为：$[0,n)$

对于二者的 `flexible array` 的详细结构，可以参考下图：

![B+Tree_node_layout](./img/B+Tree_node_layout.png)

### Page Layout

`BPlusTreePage` 为 `BPlusInternalPage` 和 `BPlusLeafPage` 的基类，它只提供最基本的三个元素，一共占 `12 bytes`

* `BPlusInternalPage`：`metadata` 占 `12 bytes`；在这基础上增加了一个 `flexible array`，后续的所有内容均由其填充
* `BPlusLeafPage` 在这基础上增加了一个指向下一个 `leaf page id` 的元素，占 `4 bytes`，因此 `metadata` 占 `16 bytes`；剩余的所有内容由 `flexible array` 进行填充

需要说明的是，`B+Tree` 的一个节点占一个 `page`，这里的意思是：**在 `4KB` 的内容中，`B+Tree` 的页面 `metadata` 分别占 `12/16 bytes`，其余的才是每一个 `pair` 存储的内容**。而每个 `page` 自身的 `metadata` 并不会存放在这 `4KB` 当中，这部分的数据是**单独存放的**

值得一提的是，对于 `internal page`，实际上每个 `pair` 的定义是：`pair<KeyType, page_id_t>`，而 对于 `leaf page`，每个 `pair` 的定义是：`pair<KeyType, ValueType>`。我们可以使用在 `b_plus_tree.h` 开头的 `InternalPage` 和 `LeafPage` 来对变量进行定义

在实际的调试过程中，`RID` 一般是占 $8$ 字节而 `page_id_t` 则是 $4$ 字节，如果发现 `internal page` 的 `flexible array` 出现了 $4$ 字节的偏移，那么大概率就是页面的定义出现了错误

> 因此到此为止，我们也就理解了，`internal page` 当中存放的是 $n$ 个元素（也就是 $n$ 个 `pair`），$n$ 个叶节点的 `page_id` 和 $n-1$ 个 `KeyType`

### Binary Search

由于我们需要频繁使用 `std::lower_bound` 和 `std::upper_bound`，因此在这里我们需要对二者的行为详细说明一下：

我们假定待查找区间内的元素为 `element`，我们传入的元素为 `value`

`std::lower_bound` 接受一个范围 $[start, end)$ 作为参数，它会将这个范围划分为两部分，前半部分满足 $element<value$，而后半部分自然是 $element>=value$。该函数会返回第一个使得 `element < value` 为 `false` 的值，也就是**后半区间的起点**

`std::upper_bound` 接受一个范围 $[start, end)$ 作为参数，它同样会将这个范围划分为两部分，前半部分满足 $element<=value$，后半部分自然是 $element>value$。它会返回第一个使得 `value < element` 为 `true` 的值，也就是**后半区间的起点**

举个例子，假设待查找区间为：$[1,2,3,4,5,6,7,8,9,10]$，假设 `value = 5`，那么：

对于 `lower_bound` 而言，区间被划分为：$[1,2,3,4]$ 和 $[5,6,7,8,9,10]$，后半区间的起点为 $5$

对于 `upper_bound` 而言，区间被划分为：$[1,2,3,4,5]$ 和 $[6,7,8,9,10]$，后半区间的起点为 $6$

## Insert

`insert` 函数的大致思路在书中有伪代码给出，在此我们只关注几个重要的点：

### Split Leaf

由于 `leaf page` 最多只能存放 $n-1$ 个元素，因此当其大小**已经**等于 $n-1$ 时，再次插入一个元素就需要进行分裂。而 `internal page` 最多能存放 $n$ 个指针，因此当其大小**已经**等于 $n$ 时，再插入一个元素，就需要进行分裂

书中的伪代码给出的做法是，先将原先旧节点的所有内容复制到一块新开的内存空间当中，然后将待插入的 `pair` 插入进去，最后在分别复制回原先的节点和新的节点

实际上，这一步我们可以简化。我们先确定待插入的 `pair` 应该在旧节点的哪个下标 `idx` 之后进行插入（可以用 `std::upper_bound` 找到），然后再确定总的元素个数 `size` 的大小（这等于旧节点的元素个数 `OldSize` 再加一）

然后，我们可以将旧节点中，区间 $[size/2,idx]$ 复制到新节点，**单独对新的 `pair` 进行复制**，然后再将旧节点中区间 $[idx + 1, OldSize)$ 的值复制到新节点

需要说明的是，这里的 `size` 有可能为奇数，那么我们在分裂后，旧节点和新节点谁多一个元素都是允许的，只要保证前后统一即可。在我们实现当中，我是允许旧节点相比于新节点多一个元素，因为我认为这在一定程度上可以减少未来再次分裂的可能性。如果是这样做的话，那么旧节点的复制区间改为 $[size/2+1,idx]$ 和 $[idx+1,OldSize]$ 即可

具体可参考下图：

![Split_Leaf](./img/Split_Leaf.png)

### Split Internal

`internal page` 的分裂与 `leaf page` 的分裂在本质上都是一样的，只不过 `internal page` 的分裂多了一个不断向父节点插入新的 `pair` 的过程

由于 `internal page` 的第一个 `key` 为空，因此我们按照 `leaf page` 的分裂方式得到两个节点后，新节点的第一个 `key` 需要置空

由于 `internal page` 有一个向上传递的过程，传递的值为：**新节点的第一个 `key` 以及新节点的 `page_id`**，也就是说我们需要将第一个 `key` 复制一遍后，将其置空，然后将 `pair` 向上传递，这样便完成了 `internal page` 的分裂操作

需要说明的是，在课本当中的伪代码里面，函数的定义为：`insert_in_parent(node N, value K', node N')`。这里的意思是，向节点 `N` 的父节点中插入一个 `pair`，其内容为 `(K', N')`

> 关于如何快速求出当前节点的父节点，由于对父节点插入的前提一定是对叶节点插入，因此我们可以在找叶节点的时候先将这条路径上的每个节点的 `page_id` 记录在 `vector` 里面，然后每次从 `back()` 取值，并 `pop` 一遍，这样子可以保证每次的 `back()` 都是当前节点的父节点

具体参考下图：

![Split_Parent](./img/Split_Parent.png)

---

对于B+树的内部节点，如果是找 `second` 的话，那么不能用二分查找