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
        "//utility:deadliner",
    ],
)
