/*
 * testCommand.cc
 *
 *  Created on: Jan 22, 2014
 *      Author: fan
 */

#include <muduo/net/Buffer.h>
#include <gtest/gtest.h>
#include <idgen/src/command.h>

TEST(command, cmd_size)
{
    muduo::net::Buffer buff;
    buff.append("*3$2\r\naa\r\n");
    idgen::Command cmd;
    std::string error;
    ASSERT_TRUE(cmd.Parse(&buff, &error));
    ASSERT_FALSE(cmd.IsDone());

    buff.retrieveAll();
    buff.append("*1\r\n$3\r\ncmd\r\n");
    cmd.Reset();
    ASSERT_TRUE(cmd.Parse(&buff, &error)) << error;
    ASSERT_TRUE(cmd.IsDone()) << cmd.Argc();
    ASSERT_TRUE(cmd.Argc() == 1) << cmd.Argc();
}

TEST(command, arg_size)
{
    muduo::net::Buffer buff;
    idgen::Command cmd;
    std::string error;
    buff.append("*1\r\n$1\r\naa\r\n");
    ASSERT_FALSE(cmd.Parse(&buff, &error)) << error;
}

TEST(command, cmd_argv)
{
    muduo::net::Buffer buff;
    idgen::Command cmd;
    buff.append("*3\r\n$1\r\na\r\n$2\r\naa\r\n$3\r\naaa\r\n");
    std::string error;
    ASSERT_TRUE(cmd.Parse(&buff, &error)) << error;
    ASSERT_TRUE(cmd.IsDone());
    ASSERT_TRUE(cmd.Argc() == 3);
    ASSERT_TRUE(cmd.Name() == "A");
    ASSERT_TRUE(cmd.Argv(0) == "A");
    ASSERT_TRUE(cmd.Argv(1) == "aa");
    ASSERT_TRUE(cmd.Argv(2) == "aaa");
}
