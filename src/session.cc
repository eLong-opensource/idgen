/*
 * session.cc
 *
 *  Created on: Jan 22, 2014
 *      Author: fan
 */

#include <string>
#include <sstream>
#include <boost/bind.hpp>
#include <stdio.h>

#include <idgen/src/id.pb.h>
#include <idgen/src/idserver.h>
#include <idgen/src/session.h>
#include <idgen/src/tunnel.h>

using namespace muduo;
using namespace muduo::net;

namespace idgen {

Session::Session(IdServer* server, const TcpConnectionPtr& conn, uint64_t sessionid)
    : server_(server),
      sessionid_(sessionid),
      conn_(conn),
      cmd_(),
      closed_(false)
{
}

Session::~Session()
{
  VLOG(1) << "~Session " << sessionid_;
}

void Session::Setup()
{
  if (!server_->IsReady()) {
    LOG(INFO) << "server not ready";
    Reply("-ERR not ready\r\n");
    conn_->shutdown();
    return;
  }

  if (!server_->IsLeader()) {
    InetAddress addr = server_->LeaderAddress();
    conn_->setMessageCallback(boost::bind(&Session::onTunnelMessage, shared_from_this(), _1, _2, _3));
    tunnel_.reset(new Tunnel(conn_->getLoop(), addr, conn_));
    tunnel_->setup();
    tunnel_->connect();
  } else {
    conn_->setMessageCallback(boost::bind(&Session::onMessage, shared_from_this(), _1, _2, _3));
  }
}

void Session::Teardown()
{
  VLOG(1) << "Session::Teardown " << sessionid_;
  if (closed_) {
    return;
  }
  closed_ = true;

  if (tunnel_) {
    tunnel_->disconnect();
  }
  CHECK(conn_);
  // 把conn_->this的引用消除
  conn_->setMessageCallback(muduo::net::defaultMessageCallback);
  conn_->shutdown();
}

void Session::Reply(const std::string& data)
{
    conn_->send(data);
}

void Session::ReplyError(const std::string& err)
{
  std::string out;
  out.append("-ERR ");
  out.append(err);
  out.append("\r\n");
  conn_->send(out);
}

void Session::ReplyInt(uint64_t n)
{
  char buf[32];
  snprintf(buf, 32, "%ld", n);
  std::string out;
  out.append(":");
  out.append(buf);
  out.append("\r\n");
  conn_->send(out);
}

void Session::ReplyString(const std::string& s)
{
  std::string out;
  out.append("+");
  out.append(s);
  out.append("\r\n");
  conn_->send(out);
}

void Session::onTunnelMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, muduo::Timestamp)
{
  const TcpConnectionPtr& remote = tunnel_->remoteConn();
  if (remote) {
    remote->send(buf);
  }
}

void Session::onMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, muduo::Timestamp)
{
    bool cont = true;
    while (cont) {
        cont = false;
        std::string error;
        if (!cmd_.Parse(buf, &error)) {
            Reply("-ERR " + error + "\r\n");
            conn->shutdown();
            return;
        }

        if (cmd_.IsDone()) {
            cont = true;
            if (!cmd_.IsSupportCommand()) {
                Reply("-ERR command not supported.\r\n");
                cmd_.Reset();
                continue;
            }

            if (cmd_.Name() == "PING") {
                Reply("+PONG\r\n");
                cmd_.Reset();
                continue;
            }

            if (cmd_.Argc() < 2) {
                Reply("-ERR Protocol error:arg number error.\r\n");
                cmd_.Reset();
                continue;
            }

            const std::string& name = cmd_.Name();
            const std::string& key = cmd_.Argv(1);
            proto::Request req;
            req.set_type(proto::NIL);
            req.set_key(key);
            req.set_value(0);

            if (name == "GET") {
                if (cmd_.Argc() != 2) {
                    Reply("-ERR Protocol error:wrong number for get.\r\n");
                } else {
                    req.set_type(proto::GET);
                }
            } else if (name == "SET") {
                if (cmd_.Argc() != 3) {
                    Reply("-ERR Protocol error:wrong number for set.\r\n");
                } else {
                    req.set_type(proto::SET);
                    std::stringstream ss(cmd_.Argv(2));
                    uint64_t v;
                    ss >> v;
                    req.set_value(v);
                }
            } else if (name == "DEL") {
                if (cmd_.Argc() != 2) {
                    Reply("-ERR Protocol error:wrong number for del.\r\n");
                } else {
                    req.set_type(proto::DEL);
                }
            } else if (name == "INCR") {
                if (cmd_.Argc() != 2) {
                    Reply("-ERR Protocol error:wrong number for incr.\r\n");
                } else {
                    req.set_type(proto::INCR);
                    req.set_value(1);
                }
            } else if (name == "INCRBY") {
                if (cmd_.Argc() != 3) {
                    Reply("-ERR Protocol error:wrong number for incrby.\r\n");
                } else {
                    req.set_type(proto::INCR);
                    std::stringstream ss(cmd_.Argv(2));
                    uint64_t v;
                    ss >> v;
                    req.set_value(v);
                }
            }
            cmd_.Reset();

            if (req.type() != proto::NIL) {
                server_->Request(shared_from_this(), &req);
            }
        }
    }
}

} /* namespace idgen */
