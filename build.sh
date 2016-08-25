#!/bin/bash 

set -e 

##blade install path
export PATH=$PATH:$HOME/bin

this=`pwd`/..

cd $this

## pyxis ROOTPATH
root=`pwd`

echo "download dependent libraries"

if [ ! -d "$root/typhoon-blade" ];then
echo "--- Blade ---"
git clone https://github.com/eLong-INF/typhoon-blade.git
fi

if [ ! -d "$root/toft" ];then 
echo "--- toft ---"
git clone https://github.com/eLong-INF/toft.git
fi

if [ ! -d "$root/raft" ];then
echo "--- raft ---"
git clone https://github.com/eLong-INF/raft.git
fi

if [ ! -d "$root/thirdparty" ];then
echo "--- thirdparty ---"
git clone https://github.com/eLong-INF/thirdparty.git
fi

cd "$root/thirdparty"

git submodule init
git submodule update

cd $root

touch  BLADE_ROOT 

pwd

./typhoon-blade/install

cd $root/idgen/

blade build

