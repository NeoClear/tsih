cc_library(
    name = "config",
    hdrs = ["config.h"],
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "driver",
    srcs = ["driver.cc"],
    deps = [
        "//application:RaftApplication",
        "//proto:api_cc_grpc",
        "//utility:Deadliner",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
    ],
)
