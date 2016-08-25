# idgen介绍

idgen是一个可以生成全局唯一自增id的分布式的高可用服务。

# idgen的编译安装

## 依赖

- centos6.4
- zlib
- boost1.53.0

## 安装

1. 执行idgen包内的build.sh
2. 编译过程需要花费一些时间，编译完成后执行 alt 即能切换至编译的结果存放目录，看到最终的二进制文件。如果alt命令找不到，二进制的目录为blade-bin/idgen/

# 运行参数

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

# id生成器的使用

idgen使用redis通信协议，运行后,可以使用redis客户端连接测试。
