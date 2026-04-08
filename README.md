# 智能日志处理系统骨架

这是一个面向操作系统课程作业的 C 语言项目骨架。
实验主题是生产者-消费者问题，场景设定为“智能日志处理系统”。

## 目录结构

- `src/main.c`：程序入口，负责基本初始化与占位线程调用
- `src/buffer.c`：循环缓冲区实现，使用互斥锁和条件变量完成同步
- `src/log_ai.c`：日志分类与评分模块的占位实现
- `src/output.c`：结果输出模块的占位实现
- `include/common.h`：公共常量与 `LogEntry` 结构体定义
- `include/buffer.h`：缓冲区模块头文件
- `include/log_ai.h`：日志分析模块头文件
- `include/output.h`：输出模块头文件
- `Makefile`：Linux 下基于 `gcc -pthread` 的编译规则

## 编译方法

```bash
make
```

## 运行方法

```bash
./logsystem
```

## 当前功能

- 已创建 2 个生产者线程和 1 个消费者线程的基础骨架
- 已准备基于 pthread 互斥锁和条件变量的循环缓冲区
- 日志分析与输出模块目前保留为最小可扩展实现
- 运行示例程序时会生成 `analysis.csv` 和 `alerts.log`

## 后续可扩展内容

1. 让生产者线程调用 `buffer_push` 写入日志
2. 让消费者线程调用 `buffer_pop` 读取日志
3. 补充真实的日志生成逻辑
4. 完善日志分类、评分与告警输出逻辑
