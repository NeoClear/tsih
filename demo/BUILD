load("@rules_proto//proto:defs.bzl", "proto_library")
load("@com_github_grpc_grpc//bazel:grpc_build_system.bzl", "grpc_proto_library")
load("@com_github_grpc_grpc//bazel:cc_grpc_library.bzl", "cc_grpc_library")

cc_grpc_library(
    name = "demo_cc_grpc",
    srcs = [":demo_proto"],
    grpc_only = True,
    deps = [":demo_cc_proto"],
)

cc_proto_library(
    name = "demo_cc_proto",
    deps = [":demo_proto"],
)

proto_library(
    name = "demo_proto",
    srcs = ["demo.proto"],
    deps = [
        "@com_google_protobuf//:any_proto",  # Well-known
    ],
)

cc_binary(
    name = "client",
    srcs = ["client.cc"],
    deps = [
        ":demo_cc_grpc",
        "@com_github_grpc_grpc//:grpc++",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
    ],
)

cc_binary(
    name = "server",
    srcs = ["server.cc"],
    deps = [
        ":demo_cc_grpc",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/strings:str_format",
    ],
)
