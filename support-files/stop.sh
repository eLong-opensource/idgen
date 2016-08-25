#!/bin/bash

cd `dirname $0`

# 根据pid杀掉进程
pid=`cat log/idgen_mon.pid`
if  [[ -e log/idgen_mon.pid ]] && \
    ps -p "$pid" | grep idgen_mon && \
    kill "$pid"
then
    rm -v log/idgen_mon.pid
    echo kill $pid
    exit 0
fi

# 根据进程名，全路径匹配
pids=`ps ax | grep '[i]dgen' | awk '{print $1}'`
for pid in $pids; do
    p=`readlink /proc/$pid/exe`
    if [[ "$p" == "`pwd`/idgen_mon" ]]; then
        echo kill $pid
        kill $pid
        exit 0
    fi
done

exit 0
