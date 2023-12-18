package(default_visibility = ["//visibility:public"])

cc_library(
    name = "PingStub",
    srcs = ["PingStub.cc"],
    hdrs = ["PingStub.h"],
    deps = [
        "//:config",
        "//proto:api_cc_grpc",
        "//utility:Logger",
        "@com_google_absl//absl/container:flat_hash_map",
    ],
)

cc_library(
    name = "ElectionStub",
    srcs = ["ElectionStub.cc"],
    hdrs = ["ElectionStub.h"],
    deps = [
        "//:config",
        "//proto:api_cc_grpc",
        "//utility:Logger",
        "@com_google_absl//absl/container:flat_hash_map",
    ],
)
