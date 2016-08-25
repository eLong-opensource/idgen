/*
 * idgen.cc
 *
 *  Created on: Jan 21, 2014
 *      Author: fan
 */

#include <stdio.h>
#include <sys/time.h>
#include <fstream>
#include <muduo/net/EventLoop.h>
#include <muduo/base/Singleton.h>
#include <boost/bind.hpp>
#include "idserver.h"
#include "config.h"

namespace idgen {

using namespace muduo;
using namespace muduo::net;

const std::string kLogFlushKey = "idgenFlushKey";
const std::string kIdgenLeaderKey = "idgenLeader";

IdServer::IdServer(EventLoop* loop, raft::Raft* raft, const muduo::net::InetAddress& addr, const std::string& snapshotDir)
    : loop_(loop),
      server_(loop_, addr, "idserver"),
      raft_(raft),
      sessions_(),
      nextSessionId_(0),
      snapshotdir_(snapshotDir),
      ready_(false),
      synctag_(),
      xid_(0),
      pendingReqs_(),
      accesslog_(NULL),
      lastFlush_(muduo::Timestamp::now()),
      keyitems_(),
      advanceStep_(muduo::Singleton<Config>::instance().AdvanceStep)
{
    initSessionId();
    initXid();
    logFilePath_ = muduo::Singleton<Config>::instance().AccessLog;
    reloadlog();

    raft_->SetSyncedCallback(boost::bind(&IdServer::onSynced, this, _1, _2));
    raft_->SetStateChangedCallback(boost::bind(&IdServer::onStateChanged, this, _1));
    raft_->SetTakeSnapshotCallback(boost::bind(&IdServer::onTakeSnapshot, this, _1, _2));
    raft_->SetLoadSnapshotCallback(boost::bind(&IdServer::onLoadSnapshot, this, _1));
    server_.setConnectionCallback(boost::bind(&IdServer::onConnection, this, _1));
}

void IdServer::Start()
{
    server_.start();
}

void IdServer::Request(const SessionPtr& s, proto::Request* data)
{
  requestPtr req(new request);
  req->Session = s;
  req->Start = muduo::Timestamp::now();
  req->Req.CopyFrom(*data);
  req->Req.set_xid(xid_++);

  if (data->type() == proto::INCR) {
    std::map<std::string, keyItemPtr>::iterator it;
    it = keyitems_.find(data->key());

    // not found, error
    if (it == keyitems_.end()) {
      s->ReplyError("not exists");
      VLOG(3) << "not exists";
      logaccess(req, 0, "not exists");
      return;
    }

    keyItemPtr k = it->second;
    uint64_t res = k->Current + data->value();
    VLOG(10) << "current " << k->Current << " limit " << k->Limit;
    // 没有到limit，直接返回
    if (res <= k->Limit) {
      k->Current = res;
      s->ReplyInt(res);
      logaccess(req, res, "i");
      return;
    }

    // 需要预申请key, 如果队列长度为空，表明是第一个申请的
    bool needAdvance = k->Waitq.size() == 0;
    k->Waitq.push_back(req);
    // 防止队列长度大于步长
    if (k->Waitq.size() >= size_t(advanceStep_)) {
      s->ReplyError("server busy");
      VLOG(2) << data->key() << " waitq full";
      logaccess(req, 0, "wait queue full");
      return;
    }

    if (needAdvance) {
      advanceKey(s, data->key(), k->Current);
    }
    return;
  }
  // 其他请求
  replicate(req);
}

// advanceKey发送ADVANCE请求
void IdServer::advanceKey(const SessionPtr& s, const std::string& key,
                          uint64_t value)
{
  requestPtr req(new request);
  req->Session = s;
  req->Start = muduo::Timestamp::now();
  req->Req.set_type(proto::ADVANCE);
  req->Req.set_key(key);
  req->Req.set_value(value);
  req->Req.set_xid(xid_++);
  replicate(req);
}

void IdServer::replicate(const requestPtr& r)
{
  uint64_t xid = r->Req.xid();
  pendingReqs_[xid] = r;
  raft_->Broadcast(r->Req.SerializeAsString());
}


bool IdServer::IsReady()
{
  // leader
  if (IsLeader() && ready_) {
    return true;
  }

  // follower
  if (raft_->LeaderId() !=0 && !IsLeader()) {
    return true;
  }

  return false;
}

bool IdServer::IsLeader()
{
  return raft_->LeaderId() ==  muduo::Singleton<Config>::instance().Myid;
}

InetAddress IdServer::LeaderAddress()
{
  int leaderid = raft_->LeaderId();
  if (leaderid == 0) {
    return InetAddress();
  } else {
    return muduo::Singleton<Config>::instance().AppAddress[leaderid];
  }
}

void IdServer::onConnection(const muduo::net::TcpConnectionPtr& conn)
{
  if (conn->connected()) {
    VLOG(1) << conn->name() << " on, sid:" << nextSessionId_;
    conn->setContext(nextSessionId_);
    SessionPtr session(new Session(this, conn, nextSessionId_));
    session->Setup();
    sessions_[nextSessionId_] = session;
    nextSessionId_++;
  } else {
    uint64_t sid = boost::any_cast<uint64_t>(conn->getContext());
    VLOG(1) << conn->name() << " off, sid:" << sid;
    std::map<uint64_t, SessionPtr>::iterator it = sessions_.find(sid);
    if (it == sessions_.end()) {
      return;
    }
    SessionPtr& ss = it->second;
    ss->Teardown();
    sessions_.erase(sid);
  }
}

void IdServer::onSynced(uint64_t index, const std::string& data)
{
    loop_->runInLoop(boost::bind(&IdServer::handleSynced, this, data));
}

void IdServer::onStateChanged(raft::ServerState state)
{
    loop_->runInLoop(boost::bind(&IdServer::handleStateChanged, this, state));
}

void IdServer::onTakeSnapshot(uint64_t index,
        const raft::TakeSnapshotDoneCallback& cb)
{
    loop_->runInLoop(boost::bind(&IdServer::handleTakeSnapshot, this, index, cb));
}

uint64_t IdServer::onLoadSnapshot(uint64_t index)
{
    if (index == 0) {
      return 0;
    }
    std::string name = snapshotFile(index);
    std::ifstream f(name.c_str());
    PCHECK(f) << "Load snapshot " << name;
    std::string key;
    uint64_t value;
    while (f >> key >> value) {
        keyItemPtr k(new keyItem);
        k->Current = value;
        k->Limit = value;
        keyitems_[key] = k;
    }
    return 0;
}

// handleSynced处理raft的日志回调
//
// leader广播的请求经过raft进行集群广播，如果得到大多数的同意则会调用这个
// 回调函数来处理请求。
void IdServer::handleSynced(const std::string& data)
{
    LOG_EVERY_N(INFO, 10000) << "on command " << google::COUNTER;
    proto::Request req;
    req.ParseFromString(data);

    const std::string& name = req.key();
    std::string reply;

    uint64_t xid = req.xid();

    requestPtr r;
    SessionPtr s;
    std::map<uint64_t, requestPtr>::iterator it;
    it = pendingReqs_.find(xid);
    if (it != pendingReqs_.end()) {
      r = it->second;
      pendingReqs_.erase(it);
      s = r->Session.lock();
      logaccess(r, req.value(), "l");
    }

    // idgen启动后，raft就开始回放日志，然而在把所有日志回放完之前是不能提供服务的
    // 因此leader在成为leader之后发送一个type为NIL的请求，同时附带一个唯一的tag
    // 如果这个请求被大家广播接受，就表明之前的日志都已经处理完毕
    if (req.type() == proto::NIL) {
        LOG(INFO) << "receive nil type, key: " << req.key();
        if (synctag_ == req.key()) {
          LOG(INFO) << "sync tag matched. server is ready";
          ready_ = true;
          // leader的incr的请求在没有到达limit之前是不会进行广播，因此新的leader
          // 需要重新设置current值以防止id回退
          resetKeyItems();
        }
        return;
    }

    if (req.type() == proto::GET) {
        if (name == kIdgenLeaderKey) {
          if (s) { s->ReplyString(LeaderAddress().toIpPort().c_str()); }
          return;
        }
        std::map<std::string, keyItemPtr>::iterator it;
        it = keyitems_.find(name);
        if (it == keyitems_.end()) {
          if (s) { s->ReplyError("not found");}
          return;
        }
        if (s) { s->ReplyInt(it->second->Current); }
        return;
    }

    if (req.type() == proto::SET) {
        if (name == kLogFlushKey) {
            reloadlog();
            if (s) { s->ReplyString("OK"); }
            return;
        }
        keyItemPtr k(new keyItem);
        k->Current = req.value();
        k->Limit = k->Current;
        keyitems_[name] = k;
        if (s) { s->ReplyString("OK"); }
        return;
    }

    if (req.type() == proto::DEL) {
        if (keyitems_.find(name) == keyitems_.end()) {
            if (s) { s->ReplyString("OK"); }
            return;
        }
        keyitems_.erase(name);
        if (s) { s->ReplyString("OK"); }
        return;
    }

    // 废弃的指令，兼容老的日志格式
    if (req.type() == proto::INCR) {
        std::map<std::string, keyItemPtr>::iterator it;
        it = keyitems_.find(name);
        if (it == keyitems_.end()) {
            if (s) { s->ReplyError("not exists"); }
            return;
        }
        keyItemPtr k = it->second;
        k->Current = req.value();
        k->Limit = k->Current;
        if (s) { s->ReplyString("OK"); }
        return;
    }

    // ADVANCE请求重置current和limit的值，同时会回复阻塞在调用incr的请求
    if (req.type() == proto::ADVANCE) {
        std::map<std::string, keyItemPtr>::iterator it0;
        it0 = keyitems_.find(name);
        if (it0 == keyitems_.end()) {
            if (s) { s->ReplyError("not exists"); }
            return;
        }
        keyItemPtr k = it0->second;
        k->Current = req.value();
        k->Limit = k->Current + advanceStep_;
        VLOG(10) << "ADVANCE " << name << " current "
                 << k->Current << " limit " << k->Limit;

        // 回复之前阻塞的请求
        std::list<requestPtr>::iterator it;
        for (it=k->Waitq.begin(); it!=k->Waitq.end(); it++) {
          requestPtr rr = *it;
          SessionPtr ss = rr->Session.lock();
          if (ss) {
            k->Current += rr->Req.value();
            ss->ReplyInt(k->Current);
            logaccess(rr, k->Current, "a");
          }
        }
        k->Waitq.clear();
        return;
    }
}

// resetKeyItems设置所有key的current到limit
// 同时清空等待队列
void IdServer::resetKeyItems()
{
  std::map<std::string, keyItemPtr>::iterator it;
  for (it=keyitems_.begin(); it!=keyitems_.end(); it++) {
    keyItemPtr k = it->second;
    // 考虑到新的Leader可能是之前的follower，而新的leader不知道之前的
    // leader的current值，但知道一定不会超过limit，因此设置自己的current
    // 为limit来避免id回退
    // 这个时候current == limit，在下次客户端请求的时候会强制进行一次advance
    k->Current = k->Limit;
    k->Waitq.clear();
  }
}

void IdServer::handleStateChanged(raft::ServerState state)
{
  LOG(INFO) << "state changed, clear pending requests and sessions";
  pendingReqs_.clear();
  std::map<uint64_t, SessionPtr>::iterator it = sessions_.begin();
  for(; it != sessions_.end(); it++) {
    SessionPtr& ss = it->second;
    ss->Teardown();
  }
  sessions_.clear();

  ready_ = false;

  if (state == raft::LEADER) {
    // 发送一个NIL包 这个NIL包的key字段使用当前时间戳 然后等待这个包的广播
    // 当这个包被成功广播 证明之前的日志都被提交了 可以正常提供服务
    synctag_ = muduo::Timestamp::now().toString().c_str();
    LOG(INFO) << "send sync tag " << synctag_;
    proto::Request req;
    req.set_type(proto::NIL);
    req.set_key(synctag_);
    req.set_value(0);
    raft_->Broadcast(req.SerializeAsString());
  }
}

void IdServer::handleTakeSnapshot(uint64_t index,
        const raft::TakeSnapshotDoneCallback& cb)
{
    std::string name = snapshotFile(index);
    FILE* fp = fopen(name.c_str(), "wb");
    PCHECK(fp != NULL) << "Take snapshot " << name;

    std::map<std::string, keyItemPtr>::iterator it;
    for (it=keyitems_.begin(); it!=keyitems_.end(); it++) {
      fprintf(fp, "%s %"PRIu64"\n", it->first.c_str(), it->second->Current);
    }
    fflush(fp);
    cb();
}

// myid(1) + timestamp_ms(5) + zero(2)
void IdServer::initSessionId()
{
    uint64_t myid = muduo::Singleton<Config>::instance().Myid;
    struct timeval tv;
    ::gettimeofday(&tv, NULL);
    nextSessionId_ = static_cast<uint64_t>(tv.tv_sec) * 1000 + tv.tv_usec / 1000;
    nextSessionId_ = (nextSessionId_ << 24 >> 8) | myid << 56;
    LOG(INFO) << "Init session id " << nextSessionId_;
}

void IdServer::initXid()
{
    uint64_t myid = muduo::Singleton<Config>::instance().Myid;
    struct timeval tv;
    ::gettimeofday(&tv, NULL);
    xid_ = static_cast<uint64_t>(tv.tv_sec) * 1000 + tv.tv_usec / 1000;
    xid_ = (nextSessionId_ << 24 >> 8) | myid << 56;
    LOG(INFO) << "Init xid " << nextSessionId_;
}

void IdServer::logaccess(const requestPtr& r, uint64_t ret, const std::string& err)
{
  // 必须和id.proto同步
  static const char* methodTable[] = {"SET", "DEL", "GET", "INCR", "NIL", "ADVANCE"};
  const char* addr = NULL;
  SessionPtr ss = r->Session.lock();
  if (ss) {
    addr = ss->Conn()->peerAddress().toIp().c_str();
  }
  muduo::Timestamp now = muduo::Timestamp::now();
  double used = muduo::timeDifference(now, r->Start);

  const char* methodName = "???";
  if (size_t(r->Req.type()) < sizeof(methodTable) / sizeof(methodTable[0])) {
    methodName = methodTable[r->Req.type()];
  }

  fprintf(accesslog_, "%s %s %s %s %ld %ld \"%s\" %f\n",
          now.toFormattedString().c_str(), addr, methodName,
          r->Req.key().c_str(), r->Req.value(), ret, err.c_str(), used);
  if (muduo::timeDifference(now, lastFlush_) > 5) {
    fflush(accesslog_);
    lastFlush_ = now;
  }
}

void IdServer::reloadlog()
{
  if (accesslog_ != NULL) {
    fclose(accesslog_);
  }
  accesslog_ = fopen(logFilePath_.c_str(), "ae");
  PCHECK(accesslog_ != NULL);
}

std::string IdServer::snapshotFile(uint64_t idx)
{
    char name[64];
    sprintf(name, "%s/snapshot.%020"PRIu64, snapshotdir_.c_str(), idx);
    return name;
}

} /* namespace idgen */
