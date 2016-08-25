/*
 * command.cc
 *
 *  Created on: Jan 22, 2014
 *      Author: fan
 */

#include <stdio.h>
#include <ctype.h>
#include <sstream>
#include <algorithm>
#include <glog/logging.h>
#include <muduo/net/Buffer.h>
#include "command.h"

namespace idgen {

Command::Command()
    : state_(kNewCommand),
      ncmd_(0),
      args_()
{
}

bool Command::Parse(muduo::net::Buffer* buff, std::string* error)
{
    if (state_ == kNewCommand) {
        const char* crlf = buff->findCRLF();
        if (crlf == NULL) {
            return true;
        }
        std::string line(buff->peek(), crlf);
        size_t n = 0;
        if (sscanf(line.c_str(), "*%zu", &n) != 1) {
            error->assign("Protocol error: get command size");
            return false;
        }
        ncmd_ = n;
        buff->retrieveUntil(crlf + 2);
        if (n != 0) {
            state_ = kParseArgs;
        }
    }

    while (state_ == kParseArgs) {
        const char* crlf1 = buff->findCRLF();
        const char* crlf2 = NULL;
        if (crlf1 == NULL) {
            return true;
        } else {
            crlf2 = buff->findCRLF(crlf1 + 1);
            if (crlf2 == NULL) {
                return true;
            }
        }
        std::string line(buff->peek(), crlf1);
        std::string arg(crlf1 + 2, crlf2);
        size_t n;
        if (sscanf(line.c_str(), "$%zu", &n) != 1) {
            error->assign("Protocol error: get arg size");
            return false;
        }
        if (arg.size() != n) {
            error->assign("Protocol error: arg size not matched");
            return false;
        }
        buff->retrieveUntil(crlf2 + 2);
        args_.push_back(arg);
        if (args_.size() == ncmd_) {
            state_ = kDone;
        }
    }
    CHECK(state_ == kDone);
    std::string& name = args_[0];
    std::transform(name.begin(), name.end(), name.begin(), ::toupper);
    return true;
}

const std::string& Command::Name() const
{
    CHECK(IsDone());
    return args_[0];
}

const std::string& Command::Argv(size_t i) const
{
    CHECK(IsDone());
    CHECK(i < ncmd_);
    return args_[i];
}

size_t Command::Argc() const
{
    return ncmd_;
}

void Command::Reset()
{
    state_ = kNewCommand;
    ncmd_ = 0;
    args_.clear();
}

bool Command::IsDone() const
{
    return state_ == kDone;
}

bool Command::IsSupportCommand()
{
    CHECK(IsDone());
    const std::string& name = Name();
    const char* cmds[] = {"GET", "SET", "INCR", "INCRBY", "DEL", "PING", NULL};
    const char** p = cmds;
    while (*p) {
        if (name == *p) {
            break;
        }
        p++;
    }
    return *p != NULL;
}

std::string Command::DebugString() const
{
    CHECK(IsDone());
    std::stringstream ss;
    ss << args_[0];
    for (size_t i=1; i<args_.size(); i++) {
        ss << " " << args_[i];
    }
    return ss.str();
}

} /* namespace idgen */
