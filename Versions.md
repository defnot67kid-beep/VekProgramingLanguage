# VEK Versions

This file is the single version history for VEK. New release notes and feature additions should be added here instead of creating separate changelog Markdown files.

---

## VEK 1.5.0

### Garage doors, passlocks and access-control UI

VEK 1.5 expands the interaction SDK with reusable garage and access-control systems.

New features:

- `GarageDoorRegistry`
- `GarageDoorSystem`
- segmented garage metadata
- configurable garage width, height and panel count
- open/close durations
- auto-close timing
- locked/unlocked policy
- garage animation IDs
- garage playback/progress state
- `PasslockRegistry`
- `PasslockSystem`
- numeric passcode validation
- maximum-attempt rules
- timed lockouts
- passlock-to-garage linking using safe string IDs
- GUI modal commands
- password-input commands
- status-badge commands
- keypad commands

New safe VEK/native integration points include garage registration/querying, passlock registration/querying and renderer-neutral access UI descriptions.

The security boundary remains unchanged: the native host owns rendering, actual input devices, raw memory, OS access, cryptography and signature verification.

---

## VEK 1.4.0

### Animations and proximity prompts

VEK 1.4 adds renderer-independent interaction primitives.

New features:

- `AnimationLibrary`
- `AnimationSystem`
- animation duration
- playback speed
- looping
- blend metadata
- animation tags
- timed animation markers
- `ProximityPromptRegistry`
- `ProximityPromptSystem`
- action/object prompt text
- input-key metadata
- activation distance
- hold duration
- prompt priority
- line-of-sight policy
- safe animation registration/query natives
- safe proximity-prompt registration/query natives

VEK scripts describe animation and interaction data while the host remains responsible for character animation execution, input and rendering.

---

## VEK 1.3.0

### Language expansion and multi-language SDK foundations

VEK 1.3 significantly expands the language and embedding architecture.

Language additions:

- arrays
- maps/dictionaries
- nested maps
- array indexing
- member access such as `engine.power`
- map indexing such as `engine["power"]`
- structs/data definitions
- events
- native host event emission
- modules/imports
- configurable module roots
- module path validation
- protection against `../` path traversal outside configured roots

SDK/embedding additions:

- stable C ABI
- `vek_create`
- `vek_destroy`
- file loading through the C API
- function calls through the C API
- event emission through the C API
- C ABI error/version helpers
- C++ runtime remains canonical
- Rust binding foundation
- C# binding foundation
- Python binding foundation
- Node/N-API binding foundation
- script entity registry using safe numeric IDs instead of native pointers
- safe component storage on script entity handles

Vehicle/editor additions:

- dynamic `PartRegistry`
- VEK-created vehicle-part definitions
- string part IDs
- component-based part metadata
- attachment metadata
- safe part registration natives

This release establishes the architecture of one VEK runtime with other host languages binding to it rather than reimplementing the interpreter.

---

## VEK 1.2.0

### Vehicle Editor and game-mode SDK

VEK 1.2 adds reusable systems for vehicle-building/editor applications.

New features:

- `GameModeSystem`
- Survival policy support
- Sandbox policy support
- `BuildPartCatalog`
- structural parts
- movement parts
- mechanical parts
- functional parts
- experimental parts
- `VehicleEditorSystem`
- grid snapping
- placement bounds
- unlock rules
- build costs
- part limits
- build validation
- `VehicleBuildCounts`
- validation/warning architecture
- safe editor helper natives
- power-to-weight helpers
- mode-aware part-cost helpers
- mode-aware unlock helpers
- renderer-neutral GUI system usable by host menus and editors

---

## VEK 1.1.0

### Gameplay systems

VEK 1.1 introduces reusable gameplay systems alongside the language runtime.

New features:

- `GravitySystem`
- frame-rate-safe gravity helpers
- jump helpers
- `HealthSystem`
- clamped damage
- healing
- alive/dead state
- health ratio helpers
- procedural `RagdollSystem`
- impact activation
- ragdoll duration
- angular collapse
- damping
- recovery state
- renderer-independent `GuiSystem`
- GUI command buffer
- windows
- labels
- buttons
- checkboxes
- sliders
- progress bars
- text input
- separators
- spacing
- gameplay helper natives
- gameplay-system tests
- GUI command-buffer tests

---

## VEK 1.0.0

### Initial VEK programming language

The first standalone VEK release established the embeddable scripting-language runtime.

Initial features:

- `.vek` source files
- numbers
- strings
- booleans
- `nil`
- local variables with `let`
- assignment
- functions
- parameters
- `if`
- `else if`
- `else`
- `while`
- `break`
- `continue`
- `return`
- arithmetic operators
- comparison operators
- boolean operators
- string concatenation
- `#` and `//` comments
- native C++ function registration
- configurable sandbox limits
- sealable native-function registry
- command-line executable
- `run`
- `check`
- `eval`
- REPL
- version command
- CMake embedding target `VEK::Runtime`

VEK began as the Vehicle Engineering Kernel for the Custom Vehicle Game while remaining independent from raylib and suitable for embedding in other C++ programs.

---

## How to update this file

For each future VEK release, add a new section at the top using:

```text
## VEK X.Y.Z

### Short release title

New features:
- feature
- feature
- feature
```

Do not create separate `CHANGELOG_*.md`, `UPDATE_GITHUB_*.md` or feature-specific Markdown files. Keep VEK documentation tidy by maintaining only:

- `README.md`
- `Versions.md`
