# idgen介绍

idgen是一个可以生成全局唯一自增id的分布式的高可用服务。

# 特性

- 分布式强一致
- redis兼容协议
- 高性能

# idgen的编译安装

## 依赖

- centos6.4
- zlib
- boost1.53.0

## 安装

1. 执行idgen包内的build.sh
2. 编译过程需要花费一些时间，编译完成后执行 alt 即能切换至编译的结果存放目录，看到最终的二进制文件。如果alt命令找不到，二进制的目录为blade-bin/idgen/

## 本地快速搭建集群

1.源代码编译(如果下载release包，直接从第二步开始)

  ``` bash

mkdir /tmp/blade_root

export BLADE_ROOT=/tmp/blade_root

git clone https://github.com/eLong-INF/idgen.git

cd idgen && ./build.sh

mkdir /tmp/idgen

cd /tmp/idgen

cp -r $BLADE_ROOT/idgen/support-files node1
cp -r $BLADE_ROOT/build64_release/idgen/idgen  node1/idgen

  ```

2.服务启动(直接拷贝release包为node1,node2,node3)

  ```
cp -r node1 node2
cp -r node1 node3

node1/start.sh 1
node2/start.sh 2
node3/start.sh 3

  ```

## id生成器的使用

idgen使用redis通信协议，运行后,可以使用redis客户端连接测试。

```

//连接任意一个实例,发送ping,回复Pong,即该实例服务启动正常。
idgen]# redis-cli -h 127.0.0.1 -p 6001 ping
PONG 

//设置id初始值,回复OK，设置成功
idgen]# redis-cli -h 127.0.0.1 -p 6001 set id 23
OK

//通过incr命令从任一实例中取出全局自增的id
idgen]# redis-cli -h 127.0.0.1 -p 6001  incr id
(integer) 24
idgen]# redis-cli -h 127.0.0.1 -p 6002  incr id
(integer) 25
idgen]# redis-cli -h 127.0.0.1 -p 6003  incr id
(integer) 26

//通过incyby命令设置每次自增的增加量
idgen]# redis-cli -h 127.0.0.1 -p 6001  incrby  id 5
(integer) 31
idgen]# redis-cli -h 127.0.0.1 -p 6002  incrby  id 5
(integer) 36
idgen]# redis-cli -h 127.0.0.1 -p 6003  incrby  id 5
(integer) 41

```

## 运行参数

idgen运行需要设置运行参数，通过--help查看参数说明。其中需要配置id.conf,如下:

```
# server{$id}=rpc address & idmachine address
server1=127.0.0.1:8001 & 127.0.0.1:6001
server2=127.0.0.1:8002 & 127.0.0.1:6002
server3=127.0.0.1:8003 & 127.0.0.1:6003

# Raft message log dir
raftlog=raftlog

# snapshot dir
snapshot=snapshot

# Election timeout in ms
electionTimeout=1000

# Heart beat timeout in ms
heartBeatTimeout=80

# If false raft logs will be replicated in batch mode
forceFlush=true

# 单次最大提交日志的数量，太多会影响Leader和Follower之间的稳定性
maxCommitSize=10000

# 提交多少个日志之后做快照, 设置为0之后按照时间来做快照
snapshotLogSize = 0

# 隔多久做快照，默认一天(3600*24)
snapshotInterval = 86400

# acccess log file
accesslog = log/access.log

# 一次批量申请的步长
advanceStep = 10000

```


