// swift-tools-version: 5.9
import PackageDescription

let package = Package(
    name: "stockfish",
    platforms: [
        .iOS("12.0")
    ],
    products: [
        .library(name: "stockfish", targets: ["stockfish"])
    ],
    dependencies: [
        .package(name: "FlutterFramework", path: "../FlutterFramework")
    ],
    targets: [
        .target(
            name: "stockfish",
            dependencies: [
                .product(name: "FlutterFramework", package: "FlutterFramework")
            ],
            exclude: [
                "Engine/src/main.cpp",
                "Engine/src/incbin/UNLICENCE"
            ],
            publicHeadersPath: "include",
            cxxSettings: [
                .headerSearchPath("Engine/src"),
                .define("NNUE_EMBEDDING_OFF"),
                .define("USE_PTHREADS"),
                .define("IS_64BIT"),
                .define("USE_POPCNT"),
                .unsafeFlags(["-std=c++17"])
            ],
            linkerSettings: [
                .linkedLibrary("c++"),
                .linkedLibrary("pthread")
            ]
        )
    ]
)
