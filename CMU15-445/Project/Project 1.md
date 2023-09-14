# Project 1



## Task 1

### Note

`dis` 的计算和 `min_early_time` 的计算需要分开来

### Test

测试时需要将 `gtest` 的 `DISABLE_` 标签去掉，在文件 `lru_k_replacer_test` 中：

```c++
TEST(LRUKReplacerTest, DISABLED_SampleTest)
```

修改为：

```c++
TEST(LRUKReplacerTest, SampleTest)
```