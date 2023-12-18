package(default_visibility = ["//visibility:public"])

cc_library(
    name = "RaftApplication",
    srcs = ["RaftApplication.cc"],
    hdrs = ["RaftApplication.h"],
    deps = [
        "//application:RaftState",
        "//proto:api_cc_grpc",
        "//stub:ElectionStub",
        "//stub:PingStub",
        "//utility:Deadliner",
        "//utility:Logger",
        "//utility:PingHistory",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/strings:str_format",
    ],
)

cc_library(
    name = "RaftState",
    srcs = ["RaftState.cc"],
    hdrs = ["RaftState.h"],
    deps = [
        "//proto:api_cc_grpc",
        "//stub:AppendStub",
        "//stub:ElectionStub",
        "//stub:PingStub",
        "//utility:Deadliner",
        "//utility:Logger",
        "//utility:PingHistory",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/strings:str_format",
    ],
)
