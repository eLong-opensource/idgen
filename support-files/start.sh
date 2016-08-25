#!/bin/bash

set -e

cd `dirname $0`

[ $# -lt 1 ] && echo 'usage ./start.sh $MYID' && exit 1

chmod +x idgen_mon run.sh

mkdir -p log

export MYID=$1
./idgen_mon -l log/mon.log -d -m log/idgen_mon.pid ./run.sh
