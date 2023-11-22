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
