# VEK

**VEK (Vehicle Engineering Kernel)** is an original C++20 embeddable scripting language created for the Custom Vehicle Game. VEK is independent of raylib and can be used by other C++ programs.

Current version: **1.2.0**

## Language features

- `.vek` source files
- numbers, booleans, strings and `nil`
- `let` local variables
- assignment
- functions and parameters
- `if / else if / else`
- `while`
- `break` and `continue`
- `return`
- arithmetic: `+ - * / %`
- comparison: `== != < <= > >=`
- boolean operators: `&& || !`
- string concatenation with `+`
- comments using `#` or `//`
- native C++ function embedding
- configurable sandbox limits
- sealed native-function registry
- CLI, REPL and syntax checker
- gameplay SDK: gravity, health and procedural ragdoll foundations
- advanced renderer-agnostic GUI command system (windows, panels, buttons, sliders, checkboxes, progress bars and text input)

VEK is a complete small scripting-language core, designed to grow with the game. Future versions can add collections, modules, bytecode and tooling without changing the embedding concept.

## Build

```bash
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Windows executable:

```text
build\\Release\\vek.exe
```

## CLI

```bash
vek run examples/hello.vek
vek check examples/hello.vek
vek eval "(10 + 5) * 2"
vek repl
vek version
```

## Example

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

## Embed in C++

```cpp
#include <VekScriptEngine.h>

VekScriptEngine vek;
vek.RegisterNative("speed", [](const std::vector<VekValue>&) {
    return VekValue(42.0);
});
vek.LoadFile("rules.vek");
auto result = vek.Call("main");
```

CMake consumers can link:

```cmake
target_link_libraries(MyGame PRIVATE VEK::Runtime)
```

## VEK 1.2 game systems

Include `<VekGameSystems.h>` for the reusable gravity, health, ragdoll and GUI APIs. `VekRegisterGameplayLibrary(engine)` exposes safe gravity/health/ragdoll helpers directly to `.vek` scripts. `vek::GuiSystem` can expose GUI commands to scripts when a host renderer is ready. See `GAME_SYSTEMS.md` and `examples/gui_blueprint.vek`.

## Security model

The runtime provides execution, call-depth, native-call, loop and source limits. The vehicle game adds its own signed-script and secure-save layer above VEK. VEK itself deliberately does not expose filesystem, OS, networking or process APIs unless the host application explicitly registers them.

## Repository structure

- `include/` public embedding API
- `src/` lexer/parser/interpreter
- `cli/` `vek` executable
- `examples/` sample `.vek` programs
- `tests/` language tests
- `.github/workflows/` CI

## License

MIT. See `LICENSE`.


## VEK 1.2 Vehicle Editor SDK

VEK now includes Survival/Sandbox game-mode policies, a reusable vehicle-part catalog, build validation/snapping/unlock/cost helpers, and the renderer-neutral GUI command system for host editors and menus. See `EDITOR_SYSTEMS.md`.
