# Project 3

- [Project 3](#project-3)
  - [Architecture Layout](#architecture-layout)
    - [Catalog Layout](#catalog-layout)
    - [Schema Layout](#schema-layout)
    - [Query Execution Tree](#query-execution-tree)
    - [Expression Tree](#expression-tree)
    - [SQL EXPLAIN](#sql-explain)
  - [Task 1](#task-1)
    - [SeqScan](#seqscan)
    - [Insert](#insert)
    - [Update \&\& Delete](#update--delete)
  - [Task 2](#task-2)
    - [Aggregate](#aggregate)
      - [Aggregate Function](#aggregate-function)
      - [Aggregate Semantic](#aggregate-semantic)
        - [Aggregate Schema](#aggregate-schema)
        - [Group by Issue](#group-by-issue)
      - [Aggregate Implementation](#aggregate-implementation)
    - [Nested Loop Join](#nested-loop-join)
    - [Hash Join](#hash-join)
      - [Inner Join \& Outer Join](#inner-join--outer-join)
      - [Predicate](#predicate)
      - [Implementation](#implementation)
    - [NLJ As Hash Join](#nlj-as-hash-join)
  - [Task 3](#task-3)
    - [Sort \& Limit](#sort--limit)
    - [TopN](#topn)
    - [Sort Limit As TopN](#sort-limit-as-topn)
  - [Harvest](#harvest)

## Architecture Layout

### Catalog Layout

`DBMS` 的 `catalog` 用于存储所有 `table` 和 `index` 的读取和写入，因此每当我们需要对 `table` 或 `index` 进行读取或更新时，我们都需要与 `catalog` 进行交互

我们以 `table` 为例，来说明整个 `catalog` 的结构

`catalog` 中用于**存储** `table` 数据的结构为 `TableInfo`，我们可以使用 `table_id` 来索引 `TableInfo`

`TableInfo` 中用于记录 `table` 相关的 `metadata`，实际**存储** `table` 的结构为 `TableHeap`

`TableHeap` 实际上是一系列的 `page`，我们需要将每个 `page` 以 `TablePage` 的形式进行读取，这样才能够正确读取到这个 `page` 内部的 `tuple`

对于一个 `TablePage` 而言，其基本属性如下：

* `num_tuples_`：`tuple` 的数量
* `num_deleted_tuple_`：以删除的 `tuple` 的数量。我们对一个 `page` 当中 `tuple` 的删除是将其 `metadata` 中的标志位进行修改，也就是对其进行 `logical delete`，在我们对 `transaction` 提交后，再执行 `physical delete`
* `tuple_info_`：每个 `tuple` 的元数据，也就是 `slotted array`。我们只记录每个 `tuple` 的偏移量 `offset`、长度 `length`和元数据 `metadata`

`table` 中的每个 `page` 都是以 `slotted array` 的形式存储 `tuple` 的。换句话说，从 `slotted array` 开始，到第一个 `tuple` 为止的这段空间，都是 `free space`

整体的结构如下图所示：

![Catalog](./img/Catalog.png)

### Schema Layout

由于每个 `tuple` 当中，只有 `RID` 和一个 `vector<char>`，也就是实际存储数据的是一个字符数组，换句话说，所有的 `attribute` 都是存在一起的（每一个 `attribute` 就是一个列 `column`）。因此如果我们想要从 `tuple` 中提取出某些 `attribute`，我们需要在此之上加一个 `schema`，通过 `schema`，我们可以从该 `tuple` 中读取出某些特定的 `attribute`

每一个 `tuple` 都有一个固定的 `schema` 与其对应，因此我们对 `tuple` 进行初始化时，除了需要提供 `Value` 数组以外，还需要提供 `schema`

> `Value calss` 内部封装了各种不同的数据类型，我们可以直接使用 `Value class` 来表示不同的数据

`Schema` 由多个 `Column` 组成，每个 `Column` 都由一个名字 `column_name_`、类型 `column_type_`、该 `column` 在 `tuple` 中的偏移量 `column_offset_`

因此，如果需要创建一个 `Schema`，我们实际上是新建了多个 `Column`，而每个 `Column` 都对应于 `tuple` 中的一个 `attribute`

### Query Execution Tree

对于一条 `SQL` 语句，它所经历的过程如下：

![Query_Execution](./img/Query_Execution.png)

> 可以参考这一篇文章：[BusTub 养成记：从课程项目到 SQL 数据库](https://zhuanlan.zhihu.com/p/570917775)

* `Parser` 会将一条 `SQL` 语句解析为 `Abstract Syntax Tree (AST)`，在 `bustub` 中这里采用系统库
* `Binder` 会将 `AST` 中的每个符号绑定到 `BDMS` 内部每个对象的标识符 `identifier`
* `Planner` 会将 `AST` 输出为查询计划 `Query Plan`
* `Optimizer` 会对 `Query Plan` 进行优化，然后交由 `operators` 进行执行

在这里，对于 `Optimizer` 生成的 `Query Plan`，实际上是一棵树结构（也就是图中的 `Abstract Plan Tree`），每一个节点都是 `AbstractPlanNode`，我们会在**实际执行的时候将该抽象类绑定到具体的 `PlanNode` 上面**。用 `AbstractPlanNode` 的好处在于我们可以将任意的子节点赋值给 `AbstractPlanNode` 类型的指针或引用

`Operators` 内部的每个 `executor` 会具体执行不同算子的行为，每一个 `Plan Node` 都对应一个 `executor`。同样，所有的 `executor node` 都继承自 `AbstractExecutor`，**在实际执行不同算子的行为时再进行绑定**（也就是图中的 `Physical Executor Tree`）

如果我们需要对 `AST` 进行修改，修改方式类似于 `Project 0` 中对 `Trie` 树的修改，就是 `Copy On Write`。我们会先递归当前节点的子节点，然后用新得到的子节点来复制一份当前节点，然后我们用子节点复制一份当前节点，得到 `optimize_plan`。如果 `optimize_plan` 满足我们的优化要求，则对其进行优化，否则直接返回 `optimize_plan`

`Project 3` 中对 `AST` 的修改与 `Project 0` 中对 `Trie` 树的修改如出一辙。说实话，这种优雅的设计让作为学生进行这个实验的我会心一笑

### Expression Tree

有些 `plan node` 会携带一个表达式，表达式本身被解析为一棵**表达式树 `expression tree`**，每个节点都为 `AbstractExpression`，**在实际执行的时候，会将各个节点绑定到具体的类上**

每个 `AbstractExpression` 中都有一个 `Evaluate` 函数，用于计算在给定 `tuple` 与 `schema` 的情况下，当前 `expression` 下的值

`Expression` 相关的类放在 `/src/include/execution/expression` 中，我们举个具体的例子

考虑如下 `SQL` 语句：

```SQL
bustub> EXPLAIN UPDATE test_1 set colB = 15445;
=== BINDER ===
BoundUpdate {
  table=BoundBaseTableRef { table=test_1, oid=20 },
  filter_expr=true,
  target_expr=[(test_1.colB, 15445)],
}
=== PLANNER ===
Update { table_oid=20, target_exprs=[#0.0, 15445, #0.2, #0.3] } | (__bustub_internal.update_rows:INTEGER)
  Filter { predicate=true } | (test_1.colA:INTEGER, test_1.colB:INTEGER, test_1.colC:INTEGER, test_1.colD:INTEGER)
    SeqScan { table=test_1 } | (test_1.colA:INTEGER, test_1.colB:INTEGER, test_1.colC:INTEGER, test_1.colD:INTEGER)
=== OPTIMIZER ===
Update { table_oid=20, target_exprs=[#0.0, 15445, #0.2, #0.3] } | (__bustub_internal.update_rows:INTEGER)
  SeqScan { table=test_1 } | (test_1.colA:INTEGER, test_1.colB:INTEGER, test_1.colC:INTEGER, test_1.colD:INTEGER)bustub> EXPLAIN UPDATE test_1 set colB = 15445;
```

我们重点看 `optimizer` 得到的结果。可以看到在 `target_exprs` 中，与两种类型的数据：

* `[#0.0, #0.2, #0.3]`：这对应表达式 `column_value_expression`，其 `Evaluate` 可以直接求出该 `tuple` 对应列的数据。形式 `x.y` 的含义为：
  * `x` 表示在 `join` 的是处于左边还是处于右边，左边 `left side (x == 0)` 或者右边 `right side (x == 1)`
  * `y` 表示第几列数据，这里分别为第零列、第二列、第三列
* `15445`：这对应表达式 `constant_calue_expression`，我们对其调用 `Evaluate` 可以直接得到具体的值，该表达式也用于存储具体的数值

### SQL EXPLAIN 

我们需要解释一下 `EXPLAIN` 的语句该如何解读，`SQL` 语句的执行可以参考官方提供的 `Shell`: [Live Database Shell](https://15445.courses.cs.cmu.edu/spring2023/bustub/)

还是看上面的那个例子，我们主要看 `PLANNER` 阶段。每一个 `operator` 的形式类似于：`Operator { } | { }`

第一个大括号中用于表示该 `operator` 的相关属性，这主要对应与每个 `executor` 中对应的那个 `plan node`。例如，在 `UpdateExecutor` 类中，含有 `UpdatePlanNode`，而 `UpdatePlanNode` 中的相关属性则正好对应于这里所显示的内容

第二个大括号用于表示该 `operator` 的 `output schema`。由于每个 `executor` 都继承自 `AbstractPlanNode`，也就是每个 `executor` 中都含有 `output_schema_` 属性，第二个大括号中的内容便是这里的 `output_schema_`

## Task 1

在 `Iterator Model` 中，我们每一个 `operator` 都可以对其调用 `Next` 函数，`Next` 函数语义为：

* 若返回 `true`，则表示可以从当前 `operator` 读取 `tuple` 和 `rid`
* 若返回 `false`，则表明当前 `operator` 已执行完毕，**无返回的 `tuple`**

`Next` 函数的语义对于每一个 `operator` 都想同，因此如果**某个 `operator` 没有输出的话，那么其 `Next` 函数需要返回 `false`**

`Init` 函数用于初始化 `operator`，我们在每次调用 `Next` 之前都会调用一次 `Init`

`Task 1` 的四个函数较为简单，这里只将每个函数的关键点写一下：

### SeqScan

`Next` 函数中，由于上层节点会对 `Next` 函数调用多次，因此如果 `table` 中有数据的话，我们每次都需要返回一个 `tuple` 以及 `true`

由于 `tuple` 是逻辑删除 `logical delete`，因此会有一些 `tuple` 会被标记为已删除，那么只要当前 `table` 中有值，那么 `next` 就必须返回一个 `tuple`，不能返回 `false`（有一些被删除的 `tuple` 会混在没有被删除的 `tuple` 中间）

### Insert

`Insert operator` 会将下层子节点传上来的 `tuple` **全部**插入 `table` 和 `index` 中

`InsertTuple` 函数会返回该 `tuple` 在 `table` 中新的 `RID`，因此我们再对索引进行插入时，需要使用新的 `RID` 进行插入

`Insert operator` 的 `Next` 函数返回的 `tuple` 中只有一个 `integer`，用于表示当前已经插入了多少行，因此如果我们插入了 $0$ 个数据，我们也需要返回一个 `tuple` 以及 `true`。如果后面继续调用 `Next`，则返回 `false`，表面当前 `operator` 已执行完对应的操作

### Update && Delete

在我们用**新的 `tuple`** 对 `index` 进行更新时，会需要从 `tuple` 中将 `key` 提取出来。我采用的方法是直接对 `index` 里面的 `key_schema_` 的列进行枚举，如果某一列的**名字**与 `table_info_` 相同，那么我们便将该列记录下来

`delete` 操作感觉上是最简单的。。。。。。

## Task 2

### Aggregate

#### Aggregate Function

我们首先需要对不同的 `aggregate` 函数的行为进行确定

* 对于 `count(*)` 而言，`null` 值无所谓，但如果本身为空的话需要**初始化为 $0$**（这是因为 `CountStarAggregate expression` 的 `evaluate` 函数会自动返回 $1$）；更新时每次加 $1$
* 对于 `cound(col)` 而言，不能加入 `null` 值，但如果本身为空需要**初始化为 $1$**；更新时每次加 $1$
* 对于 `min(), max(), sum()` 而言，不能加入 `null` 值，但初始化时需要用输入值对其赋值；更新时按照对应的函数对 `Value` 进行操作

#### Aggregate Semantic

分别运行以下语句，观察其结果

> 为了完善我对 `aggregate` 函数的理解，光是构造这些例子就把我头给搞秃了（小声

##### Aggregate Schema

```SQL
EXPLAIN SELECT min(colB), max(colA) FROM __mock_table_1;          -- ok
EXPLAIN SELECT colA, min(colB) FROM __mock_table_1 GROUP BY colA; --ok
EXPLAIN SELECT colB, max(colA) FROM __mock_table_1;               -- error
EXPLAIN SELECT colA, min(colB) FROM __mock_table_1;               -- error
```

**`aggregate operator` 的输出，也就是 `output schema` 必须为 `group bys` 和 `aggreagtes`**

---

```SQL
EXPLAIN SELECT min(colB), count(*) FROM __mock_table_1 GROUP BY colA;
EXPLAIN SELECT min(colB), count(*) FROM __mock_table_1;
```

无论 `group bys` 是否为空，`agg operator` 的输出 `output schema` 均不会发生改变，始终为 `group bys` 和 `aggregates`

##### Group by Issue

```SQL
CREATE TABLE t1(v1 int, v2 int, v3 int);
EXPLAIN SELECT min(v1), sum(v2), count(*), count(v1) FROM t1 GROUP BY v1;           --ok, output is empty
EXPLAIN SELECT min(v1), sum(v2), count(*), count(v1) FROM t1;                       --ok, output is null value
EXPLAIN SELECT v1, min(v1), sum(v2), count(*), count(v1) FROM t1;                   --error
EXPLAIN SELECT v1, min(v1), sum(v2), count(*), count(v1) FROM t1 GROUP BY t1;       -- ok 
```

对于空表而言，如果存在 `group bys`，那么无输出；如果不存在 `group bys`，那么输出均为 `null value`

并且，对于后一种情况，其 `output schema` 一定与 `aggregates` 相同，否则无法通过

#### Aggregate Implementation

实际上实现是非常简单的，我们只需要按照 `ppt` 上讲的做即可：

* `Init` 阶段

在构建 `hash table` 时，我们会对**每个** `tuple` 中，将所有满足 `group_bys` 的属性给取出来，作为 `hash table` 的 `key`。而 `hash table` 的 `value` 则是将**每个** `tuple` 中，**所有**满足 `aggregates` 的属性（换句话说，有多少个不同的 `agg` 函数，`value` 中就有多少个值）

注意到 `hash table` 中的 `key` 和 `value` 均为数组 `vector<Value>`。实际上，我们以所有满足 `group_bys_` 的属性作为 `key`，有可能 `group_bys_` 会有多个，因此需要数组来进行存储；而我们输出的 `aggregate` 也有可能有多个，我们也使用数组来进行存储

换句话说，`key` 中的数组表示的是对所有的 `group_bys_` 进行枚举，`value` 中的数组则是每个确切的 `group_bys_` 所对应的多个 `aggregate` 函数

对于 `group_bys_` 为空的情况，做一次特殊判断即可

我们每次往 `hash table` 中加入一个新的 `key-value pair`，如果该 `key` 不存在，那么则新建一个；如果已存在，那么更新对应的 `value`

* `Next` 阶段

我们每次调用 `Next` 函数时，都从 `hash table` 中**取出一个元素**，我们用 `hash table` 的 `key` 和 `value` 作为最终的输出 `tuple`（这样得到的 `tuple` 的 `schema` 一定满足 `aggregate operator` 的 `output schema`）

我们说 `Aggregate` 操作是 `pipeline breaker` 的原因在于，在我们执行 `init` 函数的时候，处理对子节点调用 `init` 函数外，我们需要构建 `hash table`，这时会导致整条流水线全部停止（可以理解为，为了让 `init` 函数执行完毕，我们不得不往整条流水线中插入气泡，因此此时流水线中只有 `init` 函数在执行）

### Nested Loop Join

对于 `inner join` 而言，只匹配列中具有相同的值，不会产生 `null` 在最终结果里面；`left join` 在 `inner join` 的基础上，对于那些列中没有匹配的值，会产生 `null` 值在最终结果里面

举个例子就是：

```sql
bustub> select * from test_simple_seq_1 s1 inner join test_simple_seq_2 s2 on s1.col1 + 5 = s2.col1;
---
bustub> select * from test_simple_seq_1 s1 left join test_simple_seq_2 s2 on s1.col1 + 5 = s2.col1;
```

后者的结果里面会有一些 `null` 值

对于 `expression` 的 `evaluate` 和 `evaluatejoin` 函数

前者用于计算该 `expression` 在单个 `tuple` 上的结果，对象是单个 `tuple`。后者用于计算将两个 `tuple` 合并起来之后，在其基础上该 `expression` 的结果，对象是两个 `tuple` 在执行 `join` 之后的 `tuple`

### Hash Join

运行以下 `SQL` 语句，观察结果：

#### Inner Join & Outer Join

```SQL
EXPLAIN SELECT * FROM __mock_table_1 LEFT OUTER JOIN __mock_table_3 ON colA = colE;
EXPLAIN SELECT * FROM __mock_table_1 JOIN __mock_table_3 ON colA = colE;
EXPLAIN SELECT * FROM __mock_table_1, __mock_table_3 WHERE colA = colE;
```

`left join` 本质上是 `left outer join`。`inner join` 与 `outer join` 的区别在于，对于 `inner join` 而言，对于那些规定的列，我们是忽略 `null` 值的，也就是 `null` 值一定不匹配；对于 `outer join` 而言，我们会对 `null` 值进行匹配，因此这边衍生出了 `left outer join` 和 `right outer join`

#### Predicate

当使用 `where` 时，`NLJ` 中的 `predicate` 始终为 `true`；当使用 `on` 时，`NLJ` 中的 `predicate` 为具体的表达式。对于 `predicate` 始终为 `true` 的情况，我们可以将 `filter` 中的 `predicate` 认为是 `hash join` 中的 `predicate`，因为在 `optimize` 中会对 `filter` 和 `NLJ` 进行合并

运行以下 `SQL` 语句，观察结果：

```SQL
EXPLAIN SELECT * FROM test_1 t1 JOIN test_2 t2 ON t2.colA = t1.colB AND t2.colA = t1.colC;
EXPLAIN SELECT * FROM test_1 t1 JOIN test_2 t2 ON t1.colA = t2.colB AND t1.colA = t2.colC;
```

`NLJ` 的 `predicate` 中，不管 `column_value_expression` 的位置如何，只要 `tuple idx`（也就是 `#0.0` 的第一个数）为 `0`，那么就是 `left key`；只要为 `1`，那么就是 `right key`

#### Implementation

`ppt` 中的思路实际上是 `inner join`，而对于 `left join`，则需要一些修改

我们对 `outer table` 构建 `hash table`，然后每次从子节点中取一个 `tuple`，`hash table` 中的 `key` 是 `tuple` 中满足条件的 `value` 的集合，而 `hash table` 的 `value` 为一个数组（或者称为一个 `bucket`），用于记录满足当前 `key` 的所有 `tuple`

假设当前执行的是 `inner join`，那么：

* 如果子节点的 `tuple` 的 `key` 匹配，我们需要依次遍历 `hash table` 中这个 `key` 所对应的所有 `tuple`，只要找到一个 `key` 是匹配的，那么就可以直接返回。这么做是为了避免 `collision` 所造成的影响
* 由于我们每次只能返回一个 `tuple`，因此如果这个 `key` 所对应的数组内有多个 `tuple` 都满足条件，我们需要依次返回，因此我们还需要一个 `idx` 去标记当前在这个数组中遍历到了哪里

`left join` 是在 `inner join` 之上再多加一层逻辑，因此需要在 `inner join` 执行失败后执行 `left join` 的逻辑

找到 `hash table` 中**所有**还没有进行匹配的 `tuple`（）由于 `right tuple` 始终为 `null`，因此 `left tuple` 可以是哈希表中任意没有匹配的 `tuple` ），然后依次输出即可

### NLJ As Hash Join

需要注意的点在于该如何构建 `hash key`，其实我们上面也已经解释了。将 `nlj` 中 `predicate` 以 `left` 和 `right` 进行分组即可

* 对于 `<column expr> = <column expr>`，那么一共就只有两种可能
* 对于 `<column expr> = <column expr> AND <column expr> = <column expr>`，那么会有四种可能

这些依次枚举即可

## Task 3

### Sort & Limit

`sort` 我们可以直接通过调用标准库 `std::sort` 来进行排序，这里的问题在于比较函数该如何设计的问题

通过查阅 [cpp reference](https://en.cppreference.com/w/cpp/algorithm/sort) 我们可以得知，`comp` 的要求为第一个参数**小于**第二个参数返回 `true`，那么 `std::sort` 函数便可以将给定范围以**非递减**顺序进行排序

因此我们可以直接从小到大遍历 `order_bys_`，对于升序要求而言，返回 `lval < rval`；对于降序要求而言，返回 `lval >= rval`

在我的实现里面，我使用了 `vector` 来存储 `tuple`，在每次初始化时都需要对 `vector` 进行清空，不然前一次的结果会保留到下一次的运算中

`Limit` 的实现非常简单，这里不过多赘述

### TopN

如果我们只是想知道最大或者最小的几个元素，那么我们没必要对所有的元素进行排序，我们只对部分进行排序便可以完成要求。我们可以直接调用 `C++` 的标准库 [`std::partial_sort`](https://en.cppreference.com/w/cpp/algorithm/partial_sort) 来实现该功能

> 这里提一嘴，`std::sort` 使用快速排序来对给定范围进行排序，对于相同的元素**不保证排序前后的顺序相同**；`std::stable_sort` 使用归并排序对给定范围进行排序，对于相同的元素**保证排序前后的顺序相同**；`std::partical_sort` 只是对部分元素进行排序，对于相同元素**不保证排序前后的顺序相同**

需要说明的是，如果数组本身的元素个数要小于 `limit`，那么我们的 `limit` 就需要修改为数组本身的大小，也就是实际的 `limit` 需要去数组大小以及 `limit plan` 中的 `limit` 的较小值

### Sort Limit As TopN

对于**同时**出现 `limit` 和 `sort` 的情况，`limit` 一定在 `sort` 的上面，因此我们将`limit` 和 `sort` 合并为一个节点，再加上原先 `sort` 的子节点，这两个一起新建一个节点返回即可

为了保证 `limit` 的子节点一定为 `sort`，我们做一层额外的判断即可

## Harvest

这个实验给我的感觉比 `Project 2` 要难一点，`Project 2` 的难点在于代码量比较大，以及在多线程场景下 `crabbing lock` 的实现问题。而 `Project 3` 则是需要我对整个 `bustub` 系统有一个理解：

* `Iterator(Volcano) Model` 是如何运行的，`next` 函数的语义 `semantic` 是如何定义的
* `AST` 中的各个 `operator` 的 `schema, predicate` 该如何理解
* `expression` 该如何解析
* `optimizer` 是如何对 `AST` 中的 `oprtator` 进行优化的

这个实验做完，我算是理解了一条 `SQL` 语句解析完生成 `AST` 之后的执行过程是怎样的了。这种实际上手敲一遍代码所学到的知识，是单纯靠看书所无法相比的