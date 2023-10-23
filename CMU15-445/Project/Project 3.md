# Project 3

- [Project 3](#project-3)
  - [Task 1](#task-1)
  - [Seq Scan](#seq-scan)
  - [aggregrate](#aggregrate)

## Task 1



## Seq Scan

只要当前 `table` 中有值，那么 `next` 就必须返回一个 `tuple`，不能返回 `false`（有一些被删除的 `tuple` 会混在没有被删除的 `tuple` 中间）

`InsertTuple` 会返回该 `tuple` 在 `table` 中的 `RID`

## aggregrate

对于 `count(*)` 而言，`null` 值无所谓，但如果本身为空的话需要**初始化为零**

对于 `cound(col)` 而言，不能加入 `null` 值

---

```SQL
EXPLAIN SELECT min(colB), max(colA) FROM __mock_table_1;  -- ok
EXPLAIN SELECT colA, min(colB) FROM __mock_table_1 GROUP BY colA; --ok
EXPLAIN SELECT colB, max(colA) FROM __mock_table_1;       -- error
```

`aggregate operator` 的输出，也就是 `output schema` 必须为 `group bys` 和 `aggreagtes` 的子集

---

```SQL
EXPLAIN SELECT min(colB), count(*) FROM __mock_table_1 GROUP BY colA;
EXPLAIN SELECT min(colB), count(*) FROM __mock_table_1;
```

无论 `group bys` 是否为空，`agg operator` 的输出 `output schema` 均不会发生改变，始终为 `group bys` 和 `aggregates`

---

```SQL
CREATE TABLE t1(v1 int, v2 int, v3 int);
EXPLAIN SELECT min(v1), sum(v2), count(*), count(v1) FROM t1 GROUP BY v1;           --ok, output is empty
EXPLAIN SELECT min(v1), sum(v2), count(*), count(v1) FROM t1;                       --ok, output is null value
EXPLAIN SELECT v1, min(v1), sum(v2), count(*), count(v1) FROM t1;                   --error
EXPLAIN SELECT v1, min(v1), sum(v2), count(*), count(v1) FROM t1 GROUP BY t1;       -- ok 
```

对于空表而言，如果存在 `group bys`，那么无输出；如果不存在 `group bys`，那么输出 `null value`

并且，对于后一种情况，其 `output schema` 一定与 `aggregates` 相同，否则无法通过

---

从 `child` 取 `tuple` 时，其 `schema` 为 `child schema`，每个 `operator` 的 `output schema` 为该算子的输出 `schema`

---

`next` 语义：

只要返回 `true`，则表示存在 `tuple`；返回 `false` 则表示当前 `query` 不存在输出

