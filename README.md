# VEK

**VEK (Vehicle Engineering Kernel)** is an original, embeddable programming language and gameplay SDK written in C++20.

VEK started as the secure gameplay scripting language for the Custom Vehicle Game, but the runtime is designed to be usable by other applications as well. It keeps one canonical language/runtime implementation and exposes safe host APIs instead of requiring every host language to reimplement VEK.

Current version: **1.5.0**

## What VEK is designed for

VEK is intended for configurable application and game logic where a native host should remain in control of performance-critical and security-sensitive operations.

A host can use VEK for things such as:

- gameplay rules and formulas
- vehicle-part definitions
- editor/build rules
- game modes
- health, gravity and ragdoll parameters
- GUI descriptions
- animations and interaction metadata
- proximity prompts
- garage doors and access-control rules
- jobs, missions, economy and progression logic
- safe entity/component scripting

The native application remains responsible for rendering, low-level physics, raw memory, operating-system integration, networking, threading, cryptography and other privileged functionality.

## Language features

VEK supports:

- `.vek` source files
- numbers, strings, booleans and `nil`
- variables and assignment
- functions and parameters
- `if / else if / else`
- `while`
- `break` and `continue`
- `return`
- arithmetic, comparison and boolean operators
- comments using `#` or `//`
- arrays
- maps/dictionaries
- nested indexing and member access
- structs/data types
- events
- safe modules/imports with configured module roots
- host-registered native functions
- configurable execution/sandbox limits
- a sealable native-function registry
- CLI, evaluator, syntax checker and REPL

## SDK systems

VEK currently includes reusable native-side SDK systems for:

- Gravity
- Health
- Procedural ragdoll foundations
- Renderer-neutral GUI commands
- Survival/Sandbox game-mode policy
- Vehicle-editor/build helpers
- Dynamic VEK part registration
- Script entity/component handles
- Animation definitions and playback state
- Proximity prompts
- Garage doors
- Passlocks/access control

## GUI system

VEK can describe interfaces without receiving direct GPU or renderer access. The host application consumes the generated GUI commands and renders them using its own renderer.

The command system includes concepts such as windows, modal windows, panels, layouts, labels, buttons, sliders, checkboxes, progress bars, text/password fields, status badges and keypads.

## One runtime, multiple host languages

VEK keeps one canonical C++ runtime.

A stable C ABI is provided through:

```text
include/vek/vek_c.h
include/vek_c.h
```

The repository also contains binding foundations for:

```text
bindings/
├── csharp/
├── node/
├── python/
└── rust/
```

These bindings are intended to wrap the same VEK runtime rather than create separate language implementations.

## Simple VEK example

```vek
fn factorial(n) {
    let result = 1;
    let i = 2;

    while i <= n {
        result = result * i;
        i = i + 1;
    }

    return result;
}

fn main() {
    println("6! =", factorial(6));
    return 0;
}
```

Arrays and maps:

```vek
let wheels = [
    "road",
    "offroad",
    "racing"
];

let engine = {
    power: 32000,
    mass: 140,
    fuel: "petrol"
};

println(wheels[1]);
println(engine.power);
```

## Build

VEK uses CMake.

```bash
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

On Windows you can also use:

```text
BUILD_WINDOWS.bat
```

## CLI

Examples:

```bash
vek run examples/hello.vek
vek check examples/hello.vek
vek eval "(10 + 5) * 2"
vek repl
vek version
```

## Embed in C++

```cpp
#include <VekScriptEngine.h>

VekScriptEngine vek;

vek.RegisterNative("speed", [](const std::vector<VekValue>&) {
    return VekValue(42.0);
});

vek.LoadFile("rules.vek");
VekValue result = vek.Call("main");
```

CMake projects can link the runtime with:

```cmake
target_link_libraries(MyProgram PRIVATE VEK::Runtime)
```

## Stable C ABI

VEK also exposes a C-compatible runtime interface for bindings and non-C++ hosts.

Conceptually:

```c
vek_runtime* runtime = vek_create();
vek_load_file(runtime, "rules.vek");
vek_value result = vek_call(runtime, "main", NULL, 0);
vek_destroy(runtime);
```

## Security model

VEK is designed around explicit host capabilities.

Scripts do not automatically receive access to:

- raw process memory
- native pointers
- arbitrary filesystem access
- shell/process execution
- operating-system APIs
- raw network sockets
- GPU memory
- DLL injection/loading
- cryptographic keys

A host application explicitly chooses which native functions and systems are exposed.

VEK also supports configurable limits for script size, execution, calls, loops and related runtime work. Applications can layer additional security on top, such as signed scripts and secure saves.

## Repository structure

```text
VEK/
├── include/
├── src/
├── cli/
├── bindings/
├── examples/
├── tests/
├── .github/
├── CMakeLists.txt
├── VERSION
├── README.md
└── Versions.md
```

## Version history

All release notes and feature updates are kept in **`Versions.md`**.

## License

VEK is released under the MIT License. See `LICENSE`.
