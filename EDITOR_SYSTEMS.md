# VEK Vehicle Editor SDK

VEK 1.2 keeps low-level rendering/physics in the C++ host while moving secure gameplay rules into VEK and its SDK.

## Main systems

- `vek::GameModeSystem`: Survival/Sandbox policy data.
- `vek::BuildPartCatalog`: reusable part metadata and unlock information.
- `vek::VehicleEditorSystem`: grid snapping, placement bounds, part limits, mode costs/unlocks and build validation.
- `vek::GuiSystem`: renderer-neutral command buffer for menus and editors.

## Script natives

`VekRegisterVehicleEditorLibrary(engine)` registers safe deterministic helpers:

- `mode_is_sandbox(mode)`
- `mode_part_cost(mode, baseCost)`
- `editor_snap(value, grid)`
- `editor_inside_radius(x, z, radius)`
- `editor_power_to_weight(mass, power)`
- `editor_basic_valid(structural, engines, seats, wheels, movement)`
- `editor_part_unlocked(mode, level, requiredLevel, sandboxOnly)`
- `editor_max_parts(mode, survivalMax, sandboxMax)`

None of these functions expose filesystem, process, network, DLL loading or raw memory.
