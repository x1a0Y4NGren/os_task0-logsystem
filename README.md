# 智能日志处理系统

## 项目简介

本项目是操作系统课程作业“任务0：智能日志处理系统”的实现，对应经典的生产者-消费者问题。
程序使用 C 语言、POSIX 线程、互斥锁和条件变量，在 Linux / WSL 环境下完成多线程日志生产、缓冲区同步、日志消费与结果输出。

## 实验任务说明

实验要求使用 `pthread_create` 创建多个生产者线程和至少一个消费者线程，利用互斥锁和条件变量实现有界缓冲区同步。
本项目中，生产者线程负责生成日志，消费者线程负责从缓冲区读取日志，并将处理结果写入输出文件。

## 已实现功能

- 支持多生产者、多消费者线程运行
- 支持有界缓冲区的入队和出队同步
- 支持命令行参数设置缓冲区容量、线程数量和日志总数
- 生产者线程会生成日志并写入共享缓冲区
- 消费者线程会读取日志并完成分析处理
- 程序运行后会生成 `analysis.csv` 和 `alerts.log`
- 程序结束前会完成线程回收和资源释放

## 项目结构

- `src/main.c`：主函数、参数解析、线程创建与回收、共享状态管理
- `src/buffer.c`：循环缓冲区及同步实现
- `src/log_ai.c`：日志分析模块
- `src/output.c`：结果文件输出模块
- `include/common.h`：公共常量和 `LogEntry` 结构体
- `include/buffer.h`：缓冲区模块声明
- `include/log_ai.h`：日志分析模块声明
- `include/output.h`：输出模块声明
- `Makefile`：编译脚本

## 编译与运行方法

### 编译

```bash
make
````

### 运行

```bash
./logsystem
```

示例：

```bash
./logsystem 8 2 1 12
```

## 参数说明

程序参数格式如下：

```bash
./logsystem [buffer_capacity] [producer_count] [consumer_count] [total_logs]
```

默认参数为：

* `buffer_capacity = 16`
* `producer_count = 2`
* `consumer_count = 1`
* `total_logs = 20`

参数含义如下：

* `buffer_capacity`：缓冲区容量
* `producer_count`：生产者线程数
* `consumer_count`：消费者线程数
* `total_logs`：总日志数

## 输出文件说明

### `analysis.csv`

保存每条日志的处理结果，表头如下：

```text
id,source,category,score,needs_alert,message
```

程序运行后，消费者线程会将每条日志的分析结果写入该文件。

### `alerts.log`

用于保存告警输出。
当日志达到相应告警条件时，程序会将告警信息写入该文件。

```

`docs/team_plan.md`

```
# 第6组 任务0 小组实现说明

## 一、任务背景

第6组对应任务0：智能日志处理系统（生产者-消费者问题）。
项目要求使用 C 语言和 POSIX 线程，在 Linux / WSL 环境下实现生产者线程、消费者线程、有界缓冲区同步以及结果输出。

## 二、当前完成情况

本项目当前已经完成以下内容：

- 建立了完整的项目目录和模块划分
- 使用 `pthread_create` 创建生产者线程和消费者线程
- 使用互斥锁和条件变量实现有界缓冲区同步
- 支持按参数设置缓冲区容量、生产者数量、消费者数量和日志总数
- 生产者线程会生成日志并写入缓冲区
- 消费者线程会读取日志并输出处理结果
- 程序运行后会生成 `analysis.csv` 和 `alerts.log`
- 程序可以在 Linux / WSL 环境下通过 `make` 编译运行

## 三、模块分工

- A：主流程整合、参数解析、线程创建与回收
- B：缓冲区模块与生产者-消费者同步
- C：日志分析模块与输出模块
- D：文档整理、实验报告与结果汇总

## 四、运行方式

编译：

```bash
make
````

运行：

```bash
./logsystem
```

带参数运行示例：

```bash
./logsystem 8 2 1 12
```

参数顺序为：

```bash
./logsystem [buffer_capacity] [producer_count] [consumer_count] [total_logs]
```

## 五、项目完成状态总结

当前仓库已完成本次课程作业的主体实现，具备：

* 多线程生产者-消费者运行流程
* 有界缓冲区同步机制
* 日志处理与结果输出
* 基本文件生成与保存