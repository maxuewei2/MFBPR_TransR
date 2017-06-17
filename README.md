# 程序使用说明

## 1. 程序运行环境说明
- 有支持C++11的编译环境
- 有Python2.7环境
- 已安装Python的Numpy科学计算库

## 2. 文件说明
#### 以下两文件夹内为数据文件，作为程序的输入数据
- bpr-transr/data
- bpr/data
#### 请使用支持C++11的编译环境编译以下两文件
- bpr-transr/bpr-transr.cpp:	BPR-TransR程序源文件
- bpr/bpr.cpp:             MFBPR程序源文件
#### 以下文件为程序输出数据文件
- outputs_and_plots/bp*.txt
- outputs_and_plots/bt*.txt
- outputs_and_plots/final.txt    该文件为以上两类文件经手动整理得到
#### 请使用Python2.7运行以下两文件，这两个文件以以上的程序输出数据文件为输入
- outputs_and_plots/plot_dim.py:         绘制不同维度下MFBPR和BPR-TransR的比较图
- outputs_and_plots/plot_40.py:        绘制随迭代次数MFBPR和BPR-TransR的比较图
#### 以下两文件为以上Python程序的输出图像
- outputs_and_plots/figure_1.png
- outputs_and_plots/figure_2.png
## 3. 编译
```shell
g++ -o bpr.exe -std=c++11 bpr.cpp
g++ -o bpr-transr.exe -std=c++11 bpr-transr.cpp
```
## 4. 运行
```shell
bpr.exe
bpr-transr.exe
```
```shell
Python plot_dim.py
Python plot_40.py
```
## 5.其他
程序的具体问题请见源代码中的注释
