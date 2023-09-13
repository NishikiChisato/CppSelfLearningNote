# Project 0

- [Project 0](#project-0)
  - [Task 1](#task-1)
    - [C++ Basic](#c-basic)
    - [Critical Component](#critical-component)
  - [Task 2](#task-2)
    - [Concurrency](#concurrency)
  - [Task 3](#task-3)
    - [Debug bustub](#debug-bustub)
  - [Task 4](#task-4)


## Task 1

### C++ Basic

我们需要编写一个 `Copy-On-Write` 的 `Trie` 树，`Trie` 树的定义在 `trie.h` 中，我们需要在 `trie.c` 中完成三个函数

`trie.h` 中有三个类，分别如下：

* `TrieNode`：表示 `Trie` 树的普通节点（该节点不含 `value`），内部由一个表示当前节点是否存在 `value` 的变量 `is_value_node_` 和一个 `map<char, shared_ptr<const TrieNode>>` 组成。需要说明的是，`map` 中的 `value` 是一个指向 `const TrieNode` 的智能指针
* `TrieNodeWithValue`：由 `TrieNode` 派生而来，在 `TrieNode` 的基础上新增了一个用于表示 `value` 的智能指针
* `Trie`：`Trie` 树的本体，内部有一个指向 `const TrieNode` 的智能指针（**该智能智能被初始化，因此无法修改其值**）

一个 `shared_ptr<const TrieNode>` 智能指针指向一个 `TrieNode` 节点，而 `TrieNode` 节点中的 `map` 则含有多个可以指向下一个 `TrieNode` 节点的智能指针。也就是说，当 `TrieNode` 调用 `clone` 时，会复制一份**当前节点**并返回。由于返回的是 `unique_ptr`，因为我们只能用 `shared_ptr` 的构造函数来接收，然后再对 `shared_ptr` 进行赋值，类似于：

```cpp
std::shared_ptr<TrieNode> new_root = std::shared_ptr<TrieNode>(this->root_->Clone());
```

其次，当我们用遍历指针 `ptr` 去遍历 `Trie` 树的节点时，由于我们需要修改节点，因此该指针的定义为`TrieNode`。但对于该指针所指向的节点而言，我们需要将下一个节点赋值给当前的 `ptr`，这里有一个问题是，`map` 中的 `value` 类型为 `const TrieNode`，我们无法直接将一个指向 `const TrieNode` 的智能指针赋值给指向 `TrieNode` 的智能指针，为了能够使该转型能够发生，我们需要使用 `std::const_pointer_cast`，类似于：

```cpp
std::shared_ptr<TrieNode> ptr;
ptr = std::const_pointer_cast<TrieNode>(ptr->children_[key[i]]);
```

`std::const_pointer_cast` 当中的类型只需要写指针指向的类型，该操作符可以将一个指向 `const` 对象的指针转型成一个指向非 `const` 对象的指针

此外，我们还需要使用 `dynamic_cast` 来对指针进行转型，有几点需要注意：

* 该操作符用于**继承**关系中的向下转型 `downcasting`，也就是说它可以将**基类的指针或引用转换为派生类的指针或引用**
* 该操作符**只适用于指针或引用类型**，其余的基本类型不适用
* 如果转换合法，则返回目标类型的指针或引用，否则返回 `nullptr`

### Critical Component

`Copy-On-Write, COW` 的核心在于写时复制，我们想象有一个进程 $A$ 需要与另外一个进程 $B$ 共享某一块内存区域，通常的做法是将 $B$ 中的这块内存复制一份给 $A$，如果使用 `COW` 技术，我们可以让这个过程的开销更小

具体来讲，只要两个进程**均不向该内存区域写入**，那么我们就完全不需要复制一份一模一样的给 $A$，在这种情况下，我们可以节省一次复制的成本，这便是写时复制

这里 `Trie` 树的核心在插入和删除操作。对于插入操作而言，由于我们已经确定要修改该 `Trie` 树的某一条路径，因此我们需要**逐个复制该路径上的所有节点**，这一点对于删除操作也是同理

对于插入操作，分两种情况：

* 在非叶子节点处插入时，我们需要 `clone` 一遍原先的节点，然后再对该节点赋值
* 在叶子节点插入时，我们直接创建新节点然后对该节点赋值即可

对于删除操作，同样分两种情况：

* 在非叶子节点处删除时，我们需要 `clone` 一遍原先的节点，然后将其从 `TrieNodeWithValue` 转换成 `TrieNode`
* 在叶子节点处删除时，在该路径上回溯，只要当前节点没有后继，那么我们便将当前节点删除

其次，在下图中，由于每个节点是由一个智能指针管理的，因此**那些灰色的节点会被自动释放**

![](https://15445.courses.cs.cmu.edu/spring2023/project0/trie-03.svg)

> 这个问题当时把我卡死了，我一直在想那些灰色的节点该如何释放



## Task 2

### Concurrency

在这一部分，我们需要让 `Trie` 树支持并发。对于常规的数据结构（不使用 `COW`），我们所使用的是 `reader/writer lock`，也就是说，允许**多个读者同时读取**，但写者**必须等待所有读者读取完毕后才能写入**

这种设计的弊端是读者与写者不能同时进行，并发度会降低，并且如果一直有读者到来，那么写者将会出现饥饿情况

使用 `COW` 则可以提高并发度，因为每次对于 `Trie` 树的修改都会 `clone` 一条新的支路，因此**读取和写入可以同时进行**，也就是说在多个读者读取的时候，写者依旧可以写入

由于读取时可能该节点之后被删除，因此每次读取除了返回该节点对应的值，还会返回整颗 `Trie` 树的根节点

这里使用了 `std::optional` 作为返回值，这个类是对返回值的结果做了一层封装。如果一个函数的返回值没有意义，但并不是错误，仅仅是作为某种标识（例如返回 `-1, nullptr` 用于表示该函数返回一个不需要的值），那么我们便可以使用 `std::optional`。如果是正常的结果那么直接赋值即可，特殊的标识则返回 `nullopt`

`TrieStore` 中含有两个 `mutex`，`root_lock_` 用于保证对根节点的**访问与修改**是按序列化的 `sequential`，换句话说，它用于保证所有线程对根节点的访问与修改只能依次进行，不允许出现同时进行；`write_lock_` 用于保证写者是按序列化的，因为我们不允许两个写者同时修改 `Trie` 树

对于一个共享变量的修改，处于 `critical section` 的操作只有三个：读取、更新、写回。因此对于写者而言，这三个操作**务必要保证其原子性**，也就是说只有当前线程完全执行完这三个操作后，才允许其他线程执行这三个操作。因此，`write_lock_` 的作用就是保证**只能有一个线程执行这三个操作**

## Task 3

### Debug bustub

`bustub` 所采用的均为 `gtest`，在调试模式下如果我们想使用 `gdb` 进行调试的话，可以进行如下操作：

```bash
make trie_debug_test
cgdb ./test/trie_debug_test
```

在 `cgdb` 中，如果我们需要跳转到 `trie_debug_test.cpp` 文件的某一行，那么我们只需要在那一行打断点即可：

```cgdb
b trie_debug_test.cpp:22
```

上面这一行代码就是在 `trie_debug_test.cpp` 文件的第 `22` 行打断点

需要说明的是，我们在本地计算得出的结果无法通过 `gradescope` 的测评，在 `discord` 上有对应的解答：

```
likern — 2023/02/15 20:33
Has anyone solved TrieDebugger.TestCase?

I've written manually (on the paper) and still get this test failed. While I'm pretty sure I understand how this works (all other tests passed)

Alex Chi — 2023/02/15 23:29
It is possible that your environment produces different random numbers than the grading environment.
In case your environment is producing different set of random numbers than our grader, replace your TrieDebugger test with:
auto trie = Trie();
trie = trie.Put<uint32_t>("65", 25);
trie = trie.Put<uint32_t>("61", 65);
trie = trie.Put<uint32_t>("82", 84);
trie = trie.Put<uint32_t>("2", 42);
trie = trie.Put<uint32_t>("16", 67);
trie = trie.Put<uint32_t>("94", 53);
trie = trie.Put<uint32_t>("20", 35);
trie = trie.Put<uint32_t>("3", 57);
trie = trie.Put<uint32_t>("93", 30);
trie = trie.Put<uint32_t>("75", 29);
```

## Task 4

这个部分很简单，在 `string_expression.h` 当中完成两个大小写转换函数，然后再在 `plan_func_call.cpp` 当中去写一下接口即可

这两个文件可能不好找，可以使用 `find` 指令来进行查找：

```bash
find ./ -name string_expression.h
find ./ -name "plan_func_call.cpp"
```

用法为：

```bash
# filename 加不加引号都可以
find /path/to/search -name "filename"
```