package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "hydra",
    data = ["//content"],
    linkopts = [
        "-lSDL2",
        "-lSDL2_image",
        "-lSDL2_mixer",
    ],
    srcs = ["main.cc"],
    deps = [
        "@libgam//:game",
        ":screens",
    ],
)

cc_library(
    name = "screens",
    srcs = [
        "title_screen.cc",
    ],
    hdrs = [
        "title_screen.h",
    ],
    deps = [
        "@libgam//:screen",
        "@libgam//:text",
        "@entt//:entt",
    ],
)
