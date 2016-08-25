/*
 * idserver.h
 *
 *  Created on: Jan 21, 2014
 *      Author: fan
 */

#ifndef IDGEN_IDSERVER_H_
#define IDGEN_IDSERVER_H_

#include <stdint.h>
#include <string>
#include <map>
#include <deque>

#include <boost/weak_ptr.hpp>

#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/EventLoopThread.h>
#include <raft/core/server.h>

#include <idgen/src/id.pb.h>
#include <idgen/src/request.h>
#include <idgen/src/keyitem.h>
#include <idgen/src/session.h>


namespace muduo {
namespace net {
class EventLoop;
}
}

namespace idgen {

class IdServer {
public:
    IdServer(muduo::net::EventLoop* loop, raft::Raft* raft, const muduo::net::InetAddress& addr, const std::string& snapshotDir);
    void Start();
    void Request(const SessionPtr& s, proto::Request* data);
    bool IsReady();
    bool IsLeader();
    muduo::net::InetAddress LeaderAddress();

private:
    void onConnection(const muduo::net::TcpConnectionPtr& conn);
    void onMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, muduo::Timestamp);
    void onBroadCast(int leaderid);
    void onSynced(uint64_t index, const std::string& data);
    void onStateChanged(raft::ServerState state);
    void onTakeSnapshot(uint64_t index, const raft::TakeSnapshotDoneCallback& cb);
    uint64_t onLoadSnapshot(uint64_t index);

    void handleMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf);
    void handleSynced(const std::string& data);
    void handleStateChanged(raft::ServerState state);
    void handleTakeSnapshot(uint64_t index, const raft::TakeSnapshotDoneCallback& cb);

    void initSessionId();

    void initXid();

    void logaccess(const requestPtr& r, uint64_t ret, const std::string& err);

    // 重新打开accesslog文件
    void reloadlog();

    void replicate(const requestPtr& r);

    void resetKeyItems();

    std::string snapshotFile(uint64_t idx);

    void advanceKey(const SessionPtr& s, const std::string& key,
                    uint64_t value);


    muduo::net::EventLoop* loop_;
    muduo::net::TcpServer server_;
    raft::Raft* raft_;
    std::map<uint64_t, SessionPtr> sessions_;
    uint64_t nextSessionId_;
    std::string snapshotdir_;

    // 同步相关的变量，用于刚启动的时候指示日志是否同步完毕
    bool ready_;
    std::string synctag_;

    // 用于跟踪请求的id
    uint64_t xid_;

    // 处于等待状态的请求
    std::map<uint64_t, requestPtr> pendingReqs_;

    // access log name
    std::string logFilePath_;

    // access log
    FILE* accesslog_;

    // access log last flush time
    muduo::Timestamp lastFlush_;

    // id keys
    std::map<std::string, keyItemPtr> keyitems_;

    // advance step
    int advanceStep_;
};

} /* namespace idgen */
#endif /* IDGEN_IDSERVER_H_ */
