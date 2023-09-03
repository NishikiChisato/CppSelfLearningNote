# project 0

- [project 0](#project-0)
  - [Task 1](#task-1)
    - [C++ Basic](#c-basic)
    - [Critical Component](#critical-component)


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


