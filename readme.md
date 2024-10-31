# 输出目录设计

OutputFolder
|--- ByDevice(GetByDeviceRootFolder)
	|--- AmazonEcho
		|---192.168.1.23.pcapng
|--- ByTime(GetByTimeRootPath)
	|--- 【Interval=1800s】
		|---AmaazonEcho
			|---{timestamp}.pcapng
			|
		 AmazonEcho.burstbin (全部集合)
|--- ByBurst
	|--- 【Interval=1s】【MaxUniqueSig=48】【MaxDuration=15s】(GetByBurstRootPath)
		|--- AmazonEcho.bin(GetByBurstDatasetBinPath)
		|--- AmazonEcho.txt
		|---【groupSize=20】.test
		|---【samplePercentage=50】【similarDistance=0.05】.train
		|--- Score (GetByBurstScoreFolder)
			|--- 【Prune=1】【samplePercentage=50】【groupMemberNum=20】【maxUniquePackets=50】【maxPacketsIndex=60】【similarDistance=0.05】【rangeOfUnique=1】【rangeOfPacketNum=30】.clsException


|---SliceFreeDataset
	|---{BurstParameter}
		|---{sample rate .etc}
			|---{hashval}.train
			|---{hashval}.test

|---SlicedDataset
	|---{BurstParameter}
		|---{DivisionParameter}
			|---{RandomValue}.train
			|---{RandomValue}.test


# 生产消费业务模型

一堆PCAP文件可以当做是在队列中已经有的很多产品。

现在有很多 一级消费者 他们将原始产品拿到， 加工为[知道设备类型的报文产品]

放入队列中， 此时一级消费者化身为 一级生产者。

会有很多 二级消费者 他们从 [特性类型报文产品的生产线] 拿到产品进行保存操作。

我们只需要一个队列即可。

## 一级消费者停止消费的条件

应当有个，等待读取的文件句柄列表，遍历这个列表

！等等！ 有文件句柄列表说明是有多个队列了！
如果是文件的话，那么就不可以使用queue，而是一个文件句柄！
我可以认为这里的queue

某个文件完全读完->某个文件句柄->inFile.eof()->文件句柄列表
->文件句柄列表是空的->退出

传递一个判断停止条件的函数指针
函数名称为: isEndofConsumption()
函数的参数是：文件句柄列表
函数的主题是：遍历文件句柄列表，判断 文件句柄列表是不是空的，如果都是空的，那么就返回布尔类型的 true

# 主线程有一个处理 Reorg的线程池，用于解析报文

# 每个接收的设备有一个线程池

这样就可以做到，在 Reorg的线程池 任务解析之后，将解析报文分配到不同的设备文件中。

# debug

现在经常出现空指针的问题。
是多线程带来的。

首先需要再context中开辟一块全局的buffer, 用来存储接受到的报文。

task中的处理函数也需要是静态全局的，不然就会有释放的问题。

为了方便，我把reorganizer作为iotBurst类的一个属性。
这样，只要iotBurst不消失，那么reorganizer实体也不会消失。

# Lambda 表达式 捕获变量的声明周期

永远不要在捕获局部变量时使用引用捕获



# 时间间隔

1.0
3.9 = 2.9

3.0 - 0.9 

1.9 = 1  9 smaller  bigger

3.3 = 2  3 bigger   smaller

间隔2秒以上纳入

first case high - low  >= setsec + 1 is OK
second case high - low == setsec and high.2 > low.2 is also OK

# 输出文件夹

