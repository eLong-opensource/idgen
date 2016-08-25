#!/bin/bash

set -e

cd `dirname $0`

mkdir -pv raftlog snapshot
./idgen --myid=1 --prof=0 --log_dir=log --conf=./id.conf &>./nohup.log
