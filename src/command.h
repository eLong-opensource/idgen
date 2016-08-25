/*
 * command.h
 *
 *  Created on: Jan 22, 2014
 *      Author: fan
 */

#ifndef IDGEN_COMMAND_H_
#define IDGEN_COMMAND_H_

namespace muduo {
namespace net {
class Buffer;
}
}

namespace idgen {

class Command {
public:
    Command();
    bool Parse(muduo::net::Buffer* buff, std::string* error);
    const std::string& Name() const;
    const std::string& Argv(size_t i) const;
    size_t Argc() const;
    bool IsDone() const ;
    bool IsSupportCommand();
    void Reset();
    std::string DebugString() const;

private:
    enum CommandState {
        kNewCommand,
        kParseArgs,
        kDone
    };
    CommandState state_;
    size_t ncmd_;
    std::vector<std::string> args_;
};

} /* namespace idgen */
#endif /* IDGEN_COMMAND_H_ */
