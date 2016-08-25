/*
 * session.h
 *
 *  Created on: Jan 22, 2014
 *      Author: fan
 */

#ifndef IDGEN_SESSION_H_
#define IDGEN_SESSION_H_

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/Buffer.h>
#include "command.h"

namespace idgen {
class IdServer;
class Tunnel;
typedef boost::shared_ptr<Tunnel> TunnelPtr;

class Session : public boost::enable_shared_from_this<Session>{
public:
    Session(IdServer* server, const muduo::net::TcpConnectionPtr& conn, uint64_t sessionid);
    ~Session();
    void Setup();
    void Teardown();
    void Reply(const std::string& data);

    // redis RESP protocol, see http://redis.io/topics/protocol
    void ReplyError(const std::string& err);
    void ReplyInt(uint64_t n);
    void ReplyString(const std::string& s);

    muduo::net::TcpConnectionPtr Conn() { return conn_; }

private:
    void onTunnelMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, muduo::Timestamp);
    void onMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, muduo::Timestamp);
    IdServer* server_;
    uint64_t sessionid_;
    muduo::net::TcpConnectionPtr conn_;
    Command cmd_;
    TunnelPtr tunnel_;
    bool closed_;
};

typedef boost::shared_ptr<Session> SessionPtr;
typedef boost::weak_ptr<Session> SessionWeakPtr;

} /* namespace idgen */
#endif /* IDGEN_SESSION_H_ */
