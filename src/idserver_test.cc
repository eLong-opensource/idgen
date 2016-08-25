#include <muduo/net/EventLoop.h>
#include <muduo/base/Singleton.h>
#include <muduo/base/Logging.h>

#include <gtest/gtest.h>

#include <stdlib.h>

#include <idgen/src/idserver.h>
#include <idgen/src/fake_raft.h>
#include <idgen/src/config.h>

using namespace muduo;
using namespace muduo::net;
using namespace idgen;

class TestServer {
public:
  TestServer(EventLoop* loop, uint16_t port)
    : loop_(loop),
      raft_(),
      server_(loop, &raft_, InetAddress(port), "")
  {}

  void Start()
  {
    server_.Start();
    raft_.Start();
  }

private:
  EventLoop* loop_;
  FakeRaft raft_;
  IdServer server_;

};

class TestClient {
public:
  TestClient(EventLoop* loop, uint16_t port)
    : loop_(loop),
      port_(port)
  {}

  void Start()
  {
    Thread thread(boost::bind(&TestClient::workThread, this), "idgenClient");
    thread.start();
  }

private:
  void workThread()
  {
    ::usleep(1000 * 1000);
    char cmd[128];

    // 准备相应的key
    snprintf(cmd, 128, "redis-cli -p %d set counter:rand:000000000000 0", port_);
    system(cmd);

    snprintf(cmd, 128, "redis-cli -p %d set counter:__rand_int__ 0", port_);
    system(cmd);

    snprintf(cmd, 128, "redis-benchmark -n 100000 -t incr,incrby -p %d", port_);
    system(cmd);

    snprintf(cmd, 128, "redis-benchmark -n 10000 -t set,get,del -p %d", port_);
    system(cmd);

    ::usleep(1000 * 1000);

    loop_->quit();
  }

  EventLoop* loop_;
  uint16_t port_;
};

void dummyOutput(const char* msg, int len)
{
}

TEST(idgen, all)
{
  Config& config = muduo::Singleton<Config>::instance();
  config.AccessLog = "access.log";
  config.AdvanceStep = 10000;

  Logger::setOutput(dummyOutput);

  EventLoop loop;
  TestServer server(&loop, 20010);
  TestClient client(&loop, 20010);
  server.Start();
  client.Start();
  loop.loop();
}
