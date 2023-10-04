# Project 2

- [Project 2](#project-2)
  - [Page understanding](#page-understanding)


## Page understanding

`B+ Tree` 中的每个节点均是一个 `page`，而对于 `internal page` 和 `leaf page` 存储的节点个数是不同的

假定 `leaf page` 的 `max_leaf_size` 设定为 $n$，那么它最多能存放 $n-1$ 个 `key-value pair`，最少能存放 $\lceil (n - 1) / 2 \rceil$ 个 `key-value pair`

对于 $i=1,2,\cdots ,n-1$，均表示不同的 `KV` 对，而当 $i=n$ 时，仅表示一个指向下一个 `leaf page` 的指针，**没有对于的 `key` 值**

> 对于如何求 $\lceil (n - 1) / 2 \rceil$，可以按照如下方法：
> 
> 当 $n$ 为奇数时，$\lceil (n - 1) / 2 \rceil = \lfloor n/2\rfloor$ 
>
> 当 $n$ 为偶数时，$\lceil (n - 1) / 2 \rceil = \lfloor n/2\rfloor$

