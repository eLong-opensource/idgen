/*
 * server.cc
 *
 *  Created on: Jan 21, 2014
 *      Author: fan
 */

#include <glog/logging.h>
#include <gflags/gflags.h>

#include <muduo/base/Singleton.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/net/inspect/Inspector.h>
#include <raft/core/transporter/sofarpc.h>
#include <raft/core/server.h>

#include <boost/scoped_ptr.hpp>

#include <toft/base/binary_version.h>

#include "config.h"
#include "idserver.h"

using namespace idgen;
using namespace raft;
using namespace muduo;
using namespace muduo::net;

int main(int argc, char* argv[])
{
    toft::SetupBinaryVersion();

    ::google::InitGoogleLogging(argv[0]);
    ::google::ParseCommandLineFlags(&argc, &argv, true);

    Config& config = muduo::Singleton<Config>::instance();
    config.Init();

    EventLoop loop;

    raft::sofarpc::Transporter trans;
    raft::Options raft_options;
    raft_options.MyID = config.Myid;
    raft_options.RaftLogDir = config.RaftLogDir;
    raft_options.ForceFlush = config.ForceFlush;
    raft_options.HeartbeatTimeout = config.HeartBeatTimeout;
    raft_options.ElectionTimeout = config.ElectionTimeout;
    raft_options.MaxCommitSize = config.MaxCommitSize;
    raft_options.SnapshotLogSize = config.SnapshotLogSize;
    raft_options.SnapshotInterval = config.SnapshotInterval;

    raft::Server raftServer(raft_options, &trans);

    raft::sofarpc::RpcServer raftRpcServer(config.RpcAddress[config.Myid].toIpPort().c_str(), raftServer.RpcEventChannel());

    IdServer idserver(&loop, &raftServer, config.AppAddress[config.Myid], config.SnapshotDir);

    for (size_t i=1; i<config.RpcAddress.size(); i++) {
        if (i == static_cast<size_t>(config.Myid)) {
            continue;
        }
        std::string peerAddr = config.RpcAddress[i].toIpPort().c_str();
        trans.AddPeer(peerAddr);
        raftServer.AddPeer(peerAddr);
    }

    EventLoopThread inspectThread;
    boost::scoped_ptr<Inspector> inspector;
    if (config.ProfPort != 0) {
        inspector.reset(new Inspector(inspectThread.startLoop(), InetAddress(config.ProfPort), "inspect"));
    }

    raftRpcServer.Start();
    raftServer.Start();

    idserver.Start();

    loop.loop();
}
