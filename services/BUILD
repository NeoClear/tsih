load("@com_github_grpc_grpc//bazel:cc_grpc_library.bzl", "cc_grpc_library")
load("@rules_proto//proto:defs.bzl", "proto_library")

package(default_visibility = ["//visibility:public"])

cc_grpc_library(
    name = "ping_cc_grpc",
    srcs = [":ping_proto"],
    grpc_only = True,
    deps = [":ping_cc_proto"],
)

cc_proto_library(
    name = "ping_cc_proto",
    deps = [":ping_proto"],
)

proto_library(
    name = "ping_proto",
    srcs = [
        "identity.proto",
        "ping.proto",
    ],
    deps = [
        "@com_google_protobuf//:empty_proto",  # Well-known
    ],
)

cc_grpc_library(
    name = "identity_cc_grpc",
    srcs = [":identity_proto"],
    grpc_only = True,
    visibility = ["//visibility:public"],
    deps = [":identity_cc_proto"],
)

cc_proto_library(
    name = "identity_cc_proto",
    deps = [":identity_proto"],
)

proto_library(
    name = "identity_proto",
    srcs = ["identity.proto"],
)

cc_library(
    name = "pinger",
    srcs = ["pinger.cc"],
    hdrs = ["pinger.h"],
    deps = [
        ":ping_cc_grpc",
        "@com_google_absl//absl/container:flat_hash_map",
        "//:config"
    ],
)
