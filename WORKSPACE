load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "rules_proto",
    sha256 = "c1783dda34d634434852f8a33be48402c45c083fd5b181ee3db01d8689add8c1",
    strip_prefix = "rules_proto-c911daaf3d260023d526ef603a7cc5d0d445561e",
    urls = [
        "https://github.com/bazelbuild/rules_proto/archive/c911daaf3d260023d526ef603a7cc5d0d445561e.tar.gz",
    ],
)

load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies", "rules_proto_toolchains")

rules_proto_dependencies()

rules_proto_toolchains()

http_archive(
    name = "com_github_grpc_grpc",
    strip_prefix = "grpc-35df344f5e17a9cb290ebf0f5b0f03ddb1ff0a97",
    urls = [
        "https://github.com/grpc/grpc/archive/35df344f5e17a9cb290ebf0f5b0f03ddb1ff0a97.tar.gz",
    ],
)

load("@com_github_grpc_grpc//bazel:grpc_deps.bzl", "grpc_deps")

grpc_deps()

load("@com_github_grpc_grpc//bazel:grpc_extra_deps.bzl", "grpc_extra_deps")

grpc_extra_deps()

# Below are dependencies for standard library and testing
# Might not be an requirement for this project, but possible

http_archive(
    name = "com_google_googletest",
    strip_prefix = "googletest-f8d7d77c06936315286eb55f8de22cd23c188571",
    urls = ["https://github.com/google/googletest/archive/f8d7d77c06936315286eb55f8de22cd23c188571.tar.gz"],
)

http_archive(
    name = "bazel_skylib",
    strip_prefix = "bazel-skylib-9c9beee7411744869300f67a98d42f5081e62ab3",
    urls = [
        "https://github.com/bazelbuild/bazel-skylib/archive/9c9beee7411744869300f67a98d42f5081e62ab3.tar.gz",
    ],
)

load("@bazel_skylib//:workspace.bzl", "bazel_skylib_workspace")

bazel_skylib_workspace()

http_archive(
    name = "com_google_absl",
    strip_prefix = "abseil-cpp-fb3621f4f897824c0dbe0615fa94543df6192f30",
    urls = ["https://github.com/abseil/abseil-cpp/archive/fb3621f4f897824c0dbe0615fa94543df6192f30.tar.gz"],
)
