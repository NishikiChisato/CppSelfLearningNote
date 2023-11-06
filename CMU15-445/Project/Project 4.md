# Project 4

- [Project 4](#project-4)
  - [Concurrency Problem](#concurrency-problem)
    - [Dirty Read](#dirty-read)
    - [Unrepetable Read](#unrepetable-read)
    - [Phantom Read](#phantom-read)
    - [Lost Update](#lost-update)
  - [Issue](#issue)


## Concurrency Problem

### Dirty Read

|$T_1$|$T_2$|
|:-:|:-:|
|$W(A)$||
||$R(A)$|
|`commit / abort`|`commit`|

$T_2$ 读取到了 $T_1$ 未提交的值

### Unrepetable Read

|$T_1$|$T_2$|
|:-:|:-:|
|$R(A)$||
||$R(A)$|
|$W(A)$||
||$R(A)$|
|`commit`|`commit`|

$T_2$ 两次读取的值不一样

### Phantom Read

|$T_1$|$T_2$|
|:-:|:-:|
|$R(A)$||
||$R(A)$|
|$D(A)$||
||$R(A)$|
|`commit`|`commit`|

$T_2$ 第二次读取的值被删除了

### Lost Update

|$T_1$|$T_2$|
|:-:|:-:|
|$W(A)$||
||$W(A)$|
|`commit`|`commit`|

$T_1$ 的写入被覆盖了


## Issue

可以将 `LockRequestQueue` 中的 `request_queue_` 中的 `raw pointer` 修改为 `smart pointer`，具体看这里 [Discord](https://discord.com/channels/724929902075445281/1014055970634215434/1053312638014206093)

