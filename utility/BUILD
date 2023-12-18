package(default_visibility = ["//visibility:public"])

cc_library(
    name = "periodic",
    hdrs = ["periodic.h"],
)

cc_library(
    name = "PingHistory",
    hdrs = ["PingHistory.h"],
    deps = [
        "//proto:api_cc_grpc",
        "@com_google_absl//absl/container:flat_hash_set",
    ],
)

cc_library(
    name = "Deadliner",
    srcs = ["Deadliner.cc"],
    hdrs = ["Deadliner.h"],
)

cc_library(
    name = "Logger",
    srcs = ["Logger.cc"],
    hdrs = ["Logger.h"],
    deps = [
        "@com_google_absl//absl/strings:str_format",
    ],
)
