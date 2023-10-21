# Project 3

- [Project 3](#project-3)
  - [Seq Scan](#seq-scan)


## Seq Scan

只要当前 `table` 中有值，那么 `next` 就必须返回一个 `tuple`，不能返回 `false`（有一些被删除的 `tuple` 会混在没有被删除的 `tuple` 中间）

`InsertTuple` 会返回该 `tuple` 在 `table` 中的 `RID`