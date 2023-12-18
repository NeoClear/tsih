cc_binary(
    name = "master",
    srcs = ["master.cc"],
    deps = [
        "//application:RaftApplication",
        "//proto:api_cc_grpc",
        "//stub:PingStub",
        "//utility:Logger",
        "//utility:PingHistory",
        "//utility:periodic",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/strings:str_format",
    ],
)

cc_binary(
    name = "client",
    srcs = ["client.cc"],
    deps = [
        "//application:RaftApplication",
        "//proto:api_cc_grpc",
        "//stub:PingStub",
        "//utility:Logger",
        "//utility:PingHistory",
        "//utility:periodic",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/strings:str_format",
    ],
)
