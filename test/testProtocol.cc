/*
 * testProtocol.cc
 *
 *  Created on: Jan 22, 2014
 *      Author: fan
 */

#include <stdio.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/Buffer.h>
#include <boost/bind.hpp>
#include <idgen/src/command.h>

using namespace idgen;
using namespace muduo;
using namespace muduo::net;

class RedisMock
{
public:
    RedisMock(EventLoop* loop)
        : server_(loop, InetAddress(6011), "redis")
    {
        server_.setConnectionCallback(boost::bind(&RedisMock::onConnection, this, _1));
        server_.setMessageCallback(boost::bind(&RedisMock::onMessage, this, _1, _2, _3));
    }

    void reply(const muduo::net::TcpConnectionPtr& conn, std::string msg)
    {
        conn->send(msg + "\r\n");
    }

    void onMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, muduo::Timestamp)
    {
        bool cont = true;
        while (cont) {
            std::string error;
            if (!cmd_.Parse(buf, &error)) {
                reply(conn, "-ERR " + error);
                conn->shutdown();
                cont = false;
            }

            if (!cmd_.IsDone()) {
                cont = false;
            }

            if (cmd_.IsDone()) {
                printf("New command: %s\n", cmd_.DebugString().c_str());
                const std::string& name = cmd_.Argv(0);
                if (name == "GET") {
                    if (cmd_.Argc() != 2) {
                        reply(conn, "-ERR Protocol error:wrong number for get.");
                    } else {
                        const std::string& key = cmd_.Argv(1);
                        if (key == "error") {
                            reply(conn, "-ERR error");
                        } else if (key == "nil"){
                            reply(conn, "$-1");
                        } else {
                            char buff[32];
                            sprintf(buff, "$%zu\r\n%s", key.size(), key.c_str());
                            reply(conn, buff);
                        }
                    }
                } else if (name == "SET"){
                    if (cmd_.Argc() != 3) {
                        reply(conn, "-ERR Protocol error:wrong number for set.");
                    } else {
                        const std::string& key = cmd_.Argv(1);
                        if (key == "error") {
                            reply(conn, "-ERR error");
                        } else {
                            reply(conn, "+OK");
                        }
                    }

                } else if (name == "INCR") {
                    if (cmd_.Argc() != 2) {
                        reply(conn, "-ERR Protocol error:wrong number for incr.");
                    } else {
                        const std::string& key = cmd_.Argv(1);
                        if (key == "error") {
                            reply(conn, "-ERR error");
                        } else {
                            reply(conn, ":100");
                        }
                    }
                } else if (name == "INCRBY"){
                    if (cmd_.Argc() != 3) {
                        reply(conn, "-ERR Protocol error:wrong number for incrby.");
                    } else {
                        const std::string& key = cmd_.Argv(1);
                        const std::string& value = cmd_.Argv(2);
                        if (key == "error") {
                            reply(conn, "-ERR error");
                        } else {
                            reply(conn, ":"+value);
                        }
                    }
                } else if (name == "DEL") {
                    if (cmd_.Argc() != 2) {
                        reply(conn, "-ERR Protocol error:wrong number for del.");
                    } else {
                        const std::string& key = cmd_.Argv(1);
                        if (key == "error") {
                            reply(conn, "-ERR error");
                        } else if (key == "nonexists"){
                            reply(conn, ":0");
                        } else {
                            reply(conn, ":1");
                        }
                    }
                } else {
                    reply(conn, "-ERR Protocol error: unknow command.");
                }
                cmd_.Reset();
            }
        }
    }

    void onConnection(const TcpConnectionPtr& conn)
    {
        if (!conn->connected()) {
            cmd_.Reset();
        }
    }

    void Start()
    {
        server_.start();
    }

private:
    TcpServer server_;
    Command cmd_;
};

int main()
{
    EventLoop loop;
    RedisMock redis(&loop);
    redis.Start();
    printf("start on port 6011\n");
    loop.loop();
}
