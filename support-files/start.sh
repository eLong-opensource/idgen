#!/bin/bash

set -e

cd `dirname $0`

chmod +x idgen_mon run.sh
./idgen_mon -l mon.log -d ./run.sh
