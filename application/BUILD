package(default_visibility = ["//visibility:public"])

cc_library(
    name = "master_application",
    srcs = ["master_application.cc"],
    hdrs = ["master_application.h"],
    deps = [
        "//proto:api_cc_grpc",
        "//stub:pinger",
        "//utility:periodic",
        "//utility:ping_history",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/strings:str_format",
    ],
)
