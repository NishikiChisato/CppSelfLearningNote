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