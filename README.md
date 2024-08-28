# fiberserver and cluster-chatroom

## 介绍
本库是参考[[C++高级教程]从零开始开发服务器框架(sylar)](https://github.com/sylar-yin/sylar)，我引入了C++11标准的内容，对原库代码进行重新编写，对各个功能进行阶段性测试。
并且基于该协程服务器开发了一个集群聊天服务器。实现新用户注册、用户登录、添加好友、添加群组、好友通信、群组聊天、保持离线消息等功能。

## 使用说明

* mkdir build && cd build && cmake ..
* make 
* ./chatserver [port]
* ./client [ip] [port]

## 文件介绍

###
* ./zy: 程序主体，包括日志、线程、协程、协程调度、反应堆、定时器、hook等模块的核心代码
* ./tests: 测试各模块相关代码
* ./chatroom: 集群聊天室程序，基于协程服务器与mysql、redis、nginx实现的集群聊天服务器和客户端程序
* CMakeList.txt：CMake用于项目构建

##

## 核心代码实现细节
### 1.协程调度器结构
如果使用(use_caller)
  主线程:主协程+调度协程+idle协程+任务协程
  子线程:主协程(调度协程)+idle协程+任务协程
  每一个task封装成一个任务协程，调用完之后析构释放资源。
### 2.定时器

### 3.反应堆流程

### 4.hook
