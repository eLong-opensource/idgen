#ifndef IDGEN_REQUEST_H
#define IDGEN_REQUEST_H

#include <muduo/base/Timestamp.h>

#include <idgen/src/session.h>
#include <idgen/src/id.pb.h>
#include <boost/shared_ptr.hpp>

namespace idgen {
  struct request {
    // 这次请求的session
    SessionWeakPtr Session;

    // 请求的pb
    proto::Request Req;

    // 请求的开始时间
    muduo::Timestamp Start;
  };

  typedef boost::shared_ptr<request> requestPtr;
}

#endif
