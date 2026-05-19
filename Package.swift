// swift-tools-version: 5.9
import PackageDescription

let package = Package(
    name: "PC110EMU",
    platforms: [
        .macOS(.v13)
    ],
    products: [
        .executable(name: "PC110EMU", targets: ["PC110EMUApp"])
    ],
    targets: [
        .target(
            name: "PC110Core",
            publicHeadersPath: "include",
            cSettings: [
                .unsafeFlags(["-std=c99"])
            ]
        ),
        .executableTarget(
            name: "PC110EMUApp",
            dependencies: ["PC110Core"],
            path: "Sources/PC110SimApp"
        )
    ]
)
