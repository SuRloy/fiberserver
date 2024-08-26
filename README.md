# fiberserver

## 介绍
本库是参考[[C++高级教程]从零开始开发服务器框架(sylar)](https://github.com/sylar-yin/sylar)，我引入了C++11标准的内容，对原库代码进行重新编写，对各个功能进行阶段性测试。

## 使用说明

* cd build && cmake ..
* make 
* ./main

## 文件介绍

###
* ./zy: 程序主体，包括日志、线程、协程、协程调度、反应堆、定时器、hook等模块的核心代码
* ./tests: 测试各模块相关代码
* CMakeList.txt：CMake用于项目构建

##

## 核心代码实现细节
### 1.协程调度器


1


### 2.定时器



### 3.反应堆流程


### 4.hook
