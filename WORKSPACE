load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "rules_proto",
    sha256 = "c6d6f9bfd39b6417724fd4a504767aa1e8dbfe828d9d41ab4ccd1976aba53fb4",
    strip_prefix = "rules_proto-7188888362a203892dec354f52623f9970bff48c",
    urls = [
        "https://github.com/bazelbuild/rules_proto/archive/7188888362a203892dec354f52623f9970bff48c.tar.gz",
    ],
)

load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies", "rules_proto_toolchains")

rules_proto_dependencies()

rules_proto_toolchains()

http_archive(
    name = "com_github_grpc_grpc",
    sha256 = "2dc24cfd9ecdc970ce0320e257c7121eec1bdf92966301019eb2b2e8f717ecf7",
    strip_prefix = "grpc-42284424acd2ba4ba649599984592f5d2eade919",
    urls = [
        "https://github.com/grpc/grpc/archive/42284424acd2ba4ba649599984592f5d2eade919.tar.gz",
    ],
)

load("@com_github_grpc_grpc//bazel:grpc_deps.bzl", "grpc_deps")

grpc_deps()

load("@com_github_grpc_grpc//bazel:grpc_extra_deps.bzl", "grpc_extra_deps")

grpc_extra_deps()

new_local_repository(
    name = "usr_local",
    build_file = "mysql.BUILD",
    path = "/usr/local",
)

# rules_cc defines rules for generating C++ code from Protocol Buffers.
http_archive(
    name = "rules_cc",
    sha256 = "35f2fb4ea0b3e61ad64a369de284e4fbbdcdba71836a5555abb5e194cf119509",
    strip_prefix = "rules_cc-624b5d59dfb45672d4239422fa1e3de1822ee110",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/rules_cc/archive/624b5d59dfb45672d4239422fa1e3de1822ee110.tar.gz",
        "https://github.com/bazelbuild/rules_cc/archive/624b5d59dfb45672d4239422fa1e3de1822ee110.tar.gz",
    ],
)

load("@rules_cc//cc:repositories.bzl", "rules_cc_dependencies")

rules_cc_dependencies()

http_archive(
    name = "com_google_googletest",
    sha256 = "983a7f2f4cc2a4d75d94ee06300c46a657291fba965e355d11ab3b6965a7b0e5",
    strip_prefix = "googletest-b796f7d44681514f58a683a3a71ff17c94edb0c1",
    urls = ["https://github.com/google/googletest/archive/b796f7d44681514f58a683a3a71ff17c94edb0c1.zip"],
)

http_archive(
    name = "bazel_skylib",
    sha256 = "b8a1527901774180afc798aeb28c4634bdccf19c4d98e7bdd1ce79d1fe9aaad7",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/bazel-skylib/releases/download/1.4.1/bazel-skylib-1.4.1.tar.gz",
        "https://github.com/bazelbuild/bazel-skylib/releases/download/1.4.1/bazel-skylib-1.4.1.tar.gz",
    ],
)

load("@bazel_skylib//:workspace.bzl", "bazel_skylib_workspace")

bazel_skylib_workspace()

http_archive(
    name = "com_google_absl",
    sha256 = "d091c4da2c1d51f52e7d37fb4a6c6e8be3cc4f5ddf9f1de50754be2f6c992818",
    strip_prefix = "abseil-cpp-a69b0ae5cdba53a45617afc408618a3e1ac244de",
    urls = ["https://github.com/abseil/abseil-cpp/archive/a69b0ae5cdba53a45617afc408618a3e1ac244de.zip"],
)

benchmark_version = "1.7.1"

http_archive(
    name = "com_github_google_benchmark",
    sha256 = "6430e4092653380d9dc4ccb45a1e2dc9259d581f4866dc0759713126056bc1d7",
    strip_prefix = "benchmark-%s" % benchmark_version,
    urls = ["https://github.com/google/benchmark/archive/refs/tags/v%s.tar.gz" % benchmark_version],
)
