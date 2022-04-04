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
        ":config",
        ":screens",
    ],
)

cc_library(
    name = "config",
    srcs = ["config.cc"],
    hdrs = ["config.h"],
    deps = ["@libgam//:game"],
)

cc_library(
    name = "screens",
    srcs = [
        "game_screen.cc",
        "title_screen.cc",
    ],
    hdrs = [
        "game_screen.h",
        "title_screen.h",
    ],
    deps = [
        "@libgam//:screen",
        "@libgam//:spritemap",
        "@libgam//:text",
        "@entt//:entt",
        ":components",
        ":config",
        ":dialog",
        ":geometry",
        ":space",
    ],
)

cc_library(
    name = "components",
    hdrs = ["components.h"],
    deps = [":geometry"],
)

cc_library(
    name = "geometry",
    hdrs = ["geometry.h"],
)

cc_library(
    name = "space",
    srcs = ["space.cc"],
    hdrs = ["space.h"],
    deps = [
        "@libgam//:graphics",
        ":config",
        ":geometry",
    ],
)

cc_library(
    name = "dialog",
    srcs = ["dialog.cc"],
    hdrs = ["dialog.h"],
    deps = [
        "@libgam//:text",
        ":config",
    ],
)
