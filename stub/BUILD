package(default_visibility = ["//visibility:public"])

cc_library(
    name = "pinger",
    srcs = ["pinger.cc"],
    hdrs = ["pinger.h"],
    deps = [
        "//:config",
        "//proto:api_cc_grpc",
        "@com_google_absl//absl/container:flat_hash_map",
    ],
)

cc_library(
    name = "election",
    srcs = ["election.cc"],
    hdrs = ["election.h"],
    deps = [
        "//:config",
        "//proto:api_cc_grpc",
        "//utility:logger",
        "@com_google_absl//absl/container:flat_hash_map",
    ],
)
