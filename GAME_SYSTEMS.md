# VEK 1.1 Gameplay Systems SDK

VEK 1.1 adds a renderer-agnostic gameplay SDK alongside the scripting language.

## Gravity
`vek::GravitySystem` supplies frame-rate-safe vertical gravity/jump state. Scripts can also call:
`gravity_step(velocity, gravity, terminal, dt, grounded)` and `gravity_jump(can_jump, jump_speed)`.

## Health
`vek::HealthSystem` provides clamped damage/healing/alive/hurt state. Script helpers:
`health_damage`, `health_heal`, `health_ratio`, `health_alive`.

## Ragdoll
`vek::RagdollSystem` provides activation, impact-driven duration, angular collapse, damping and recovery state. Script helpers:
`ragdoll_should_trigger` and `ragdoll_duration`.

This is a portable procedural ragdoll foundation, not a replacement for a future rigid-body physics backend.

## Advanced GUI command system
`vek::GuiSystem` is deliberately renderer-agnostic. VEK scripts can emit windows, panels, labels, buttons, checkboxes, sliders, progress bars, text fields, separators and spacers. A host application renders the returned `GuiCommand` buffer and feeds input events back.

The vehicle game does **not use the VEK GUI system yet**. It is included so future VEK-authored menus/tools do not require a language redesign.
