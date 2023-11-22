load("@com_github_grpc_grpc//bazel:cc_grpc_library.bzl", "cc_grpc_library")
load("@rules_proto//proto:defs.bzl", "proto_library")

package(default_visibility = ["//visibility:public"])

cc_grpc_library(
    name = "api_cc_grpc",
    srcs = [":api_proto"],
    grpc_only = True,
    deps = [":api_cc_proto"],
)

cc_proto_library(
    name = "api_cc_proto",
    deps = [
        ":api_proto",
    ],
)

proto_library(
    name = "api_proto",
    srcs = [
        "api.proto",
        "token.proto",
    ],
    deps = [
        "@com_google_protobuf//:empty_proto",
    ],
)
