# VEK 2.1 Portable Release Architecture

VEK 2.1 adds a relocatable Windows distribution. The official release workflow builds a package whose root contains `vek.exe`, so users can add the package root itself to PATH.

Release layout:

```text
vek/
├── vek.exe
├── vek.dll
├── VERSION
├── LICENSE
├── README.md
├── INSTALL_PATH.cmd
├── UNINSTALL_PATH.cmd
├── manifest.sha256
├── include/
├── lib/
├── examples/
└── docs/
```

The CLI discovers its home from its executable path and only uses `VEK_HOME` as a fallback when that variable points at a structurally valid VEK installation.

`vek verify` checks package files against `manifest.sha256`. This detects local changes/corruption; it is not a substitute for verifying that the release itself came from a trusted GitHub release.
