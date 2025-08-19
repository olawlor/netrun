// swift-tools-version: 6.0
import PackageDescription

let package = Package(
  name: "UI",
  platforms: [.macOS(.v10_15)],
    dependencies: [
    ],
  targets: [
    .executableTarget(
      name: "foo"
    ),
    .target(
        name: "SelectedBackend",
    ),
  ]
)


