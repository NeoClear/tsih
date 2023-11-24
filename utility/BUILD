package(default_visibility = ["//visibility:public"])

cc_library(
    name = "periodic",
    hdrs = ["periodic.h"],
)

cc_library(
    name = "ping_history",
    hdrs = ["ping_history.h"],
    deps = [
        "//proto:api_cc_grpc",
        "@com_google_absl//absl/container:flat_hash_set",
    ],
)

cc_library(
    name = "deadliner",
    srcs = ["deadliner.cc"],
    hdrs = ["deadliner.h"],
)
