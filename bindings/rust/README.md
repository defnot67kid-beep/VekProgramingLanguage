# VEK Rust binding
This crate wraps the single canonical VEK runtime through `vek_c.h`. Link the VEK shared library (`vek.dll`, `libvek.so`, or `libvek.dylib`) with your application. The first wrapper exposes creation, file loading, and numeric calls; extend it without reimplementing the interpreter.
