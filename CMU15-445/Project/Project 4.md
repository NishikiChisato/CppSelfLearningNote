# Project 4

- [Project 4](#project-4)
  - [Concurrency Problem](#concurrency-problem)
    - [Dirty Read](#dirty-read)
    - [Unrepetable Read](#unrepetable-read)
    - [Phantom Read](#phantom-read)
    - [Lost Update](#lost-update)


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
