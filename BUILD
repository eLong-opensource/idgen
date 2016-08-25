cc_library(
    name = 'idgenlib',
    srcs = [
        'src/command.cc',
        'src/config.cc',
        'src/session.cc',
        'src/idserver.cc',
    ],
    deps = [
        ':idproto',
        '//thirdparty/muduo:muduo_net',
        '//raft:raft',
        '//thirdparty/gflags:gflags',
        '//thirdparty/glog:glog',
        '//thirdparty/perftools:tcmalloc'
    ]
)

cc_binary(
    name = 'idgen',
    srcs = [
        'src/main.cc'
    ],
    deps = [
        ':idgenlib',
        '//thirdparty/muduo:muduo_inspect',
        '//raft:sofa_transporter',
        "//toft/base:binary_version"
    ]
)

cc_binary(
    name = 'testproto',
    srcs = [
        'test/testProtocol.cc'
    ],
    deps = [
        ':idgenlib'
    ]
)
cc_binary(
    name = 'idplugin',
    srcs = [
        'tools/idplugin.cc'
    ],
    deps = [
        ':idproto'
    ]
)

cc_test(
    name = 'testIdgen',
    srcs = [
        'test/testCommand.cc',
        'src/idserver_test.cc'
    ],
    deps = [
        ':idgenlib'
    ]
)

proto_library(
    name = 'idproto',
    srcs = 'src/id.proto'
)
