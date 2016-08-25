/*
 * config.cc
 *
 *  Created on: Jan 21, 2014
 *      Author: fan
 */

#include <stdio.h>
#include <sstream>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <raft/base/kvconfig.h>
#include <raft/base/address.h>

#include "config.h"

DEFINE_string(conf, "id.conf", "config file");
DEFINE_int32(myid, 1, "id of mine");
DEFINE_int32(prof, 0, "port of prof");

namespace idgen {

Config::Config()
    : Myid(0),
      ElectionTimeout(0),
      HeartBeatTimeout(0),
      MaxCommitSize(10000),
      SnapshotLogSize(1000000),
      SnapshotInterval(3600 * 24),
      ForceFlush(true),
      AdvanceStep(10000),
      RaftLogDir(""),
      SnapshotDir(""),
      ProfPort(0),
      AccessLog("log/access.log"),
      RpcAddress(),
      AppAddress()
{
}

void Config::Init()
{
    Myid = FLAGS_myid;
    ProfPort = FLAGS_prof;

    raft::KvConfig config;
    if (config.Parse(FLAGS_conf) < 0) {
        LOG(FATAL) << "parse config file error.";
    }
    RaftLogDir = config.String("raftlog");
    SnapshotDir = config.String("snapshot");
    ElectionTimeout = config.Int("electionTimeout");
    HeartBeatTimeout = config.Int("heartBeatTimeout");
    MaxCommitSize = config.Int("maxCommitSize");
    SnapshotLogSize = config.Int("snapshotLogSize");
    SnapshotInterval = config.Int("snapshotInterval");
    ForceFlush = config.Bool("forceFlush");
    AccessLog = config.String("accesslog");
    AdvanceStep = config.Int("advanceStep");

    RpcAddress.push_back(muduo::net::InetAddress("0.0.0.0", 0));
    AppAddress.push_back(muduo::net::InetAddress("0.0.0.0", 0));

    char buf[64];
    for (int i=1; ;i++) {
        sprintf(buf, "server%d", i);
        if (config.Exists(buf)) {
            std::stringstream ss(config.String(buf));
            std::string rpcAddr, appAddr;
            std::getline(ss, rpcAddr, '&');
            std::getline(ss, appAddr, '&');

            std::string ip;
            uint16_t port;
            raft::Address(rpcAddr, &ip, &port);
            RpcAddress.push_back(muduo::net::InetAddress(ip, port));

            raft::Address(appAddr, &ip, &port);
            AppAddress.push_back(muduo::net::InetAddress(ip, port));

            LOG(INFO) << "add rpc address:" << rpcAddr << " app address:" << appAddr;
        } else {
            break;
        }
    }
}


} /* namespace idgen */
