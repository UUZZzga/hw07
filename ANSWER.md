# 改进前

```
matrix_randomize: 2.08255s
t=1: n=14848
matrix_randomize: 1.41455s
t=2: n=16384
matrix_randomize: 1.78579s
t=3: n=16896
matrix_randomize: 1.83421s

matrix_transpose: 1.48704s
t=1: n=14848
matrix_transpose: 0.953459s
t=2: n=16384
matrix_transpose: 1.14123s
t=3: n=16896
matrix_transpose: 1.24748s
```

# 改进后

```
t=0: n=17920
matrix_randomize: 0.0751787s
t=1: n=14848
matrix_randomize: 0.0831394s
t=2: n=16384
matrix_randomize: 0.0624153s
t=3: n=16896
matrix_randomize: 0.0673621s


```

# 加速比

matrix_randomize: 20x
matrix_transpose: 10000x
matrix_multiply: 10000x
matrix_RtAR: 10000x

> 如果记录了多种优化方法，可以做表格比较

# 优化方法

下面这些函数你是如何优化的？是什么思路？用了老师上课的哪个知识点？

> matrix_randomize

请回答。

> matrix_transpose

请回答。

> matrix_multiply

请回答。

> matrix_RtAR

请回答。

# 我的创新点

如果有，请说明。
