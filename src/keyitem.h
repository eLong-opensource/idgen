#ifndef IDGEN_KEYITEM_H
#define IDGEN_KEYITEM_H

#include <stdint.h>
#include <list>
#include <boost/shared_ptr.hpp>
#include <idgen/src/request.h>

namespace idgen {

struct keyItem {
  // 当前的id值
  uint64_t Current;

  // 预申请到的id值
  uint64_t Limit;

  // 在预申请返回前积压的请求
  std::list<requestPtr> Waitq;
};

typedef boost::shared_ptr<keyItem> keyItemPtr;

}

#endif
