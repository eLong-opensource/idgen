/*
 * config.h
 *
 *  Created on: Jan 21, 2014
 *      Author: fan
 */

#ifndef IDGEN_CONFIG_H_
#define IDGEN_CONFIG_H_

#include <vector>
#include <string>
#include <muduo/net/InetAddress.h>

namespace idgen {

class Config {
public:
    Config();
    void Init();
    int Myid;
    int ElectionTimeout;
    int HeartBeatTimeout;
    int MaxCommitSize;
    int SnapshotLogSize;
    int SnapshotInterval;
    bool ForceFlush;
    int AdvanceStep;
    std::string RaftLogDir;
    std::string SnapshotDir;
    int ProfPort;
    std::string AccessLog;
    std::vector<muduo::net::InetAddress> RpcAddress;
    std::vector<muduo::net::InetAddress> AppAddress;
};

} /* namespace idgen */
#endif /* IDGEN_CONFIG_H_ */
