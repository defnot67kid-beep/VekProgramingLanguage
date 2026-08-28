# VEK 1.4.0

VEK 1.4 adds renderer-independent gameplay interaction primitives.

- AnimationLibrary and AnimationSystem
- Animation clips with duration, speed, loop, blends, tags and timed markers
- ProximityPromptRegistry and ProximityPromptSystem
- Prompt action/object text, input key, range, hold duration, priority and line-of-sight policy
- New safe VEK natives: animation_register, animation_exists, animation_duration, prompt_register, prompt_exists, prompt_max_distance
- New interaction-system tests and example
- Existing VEK Guard/native sandbox model remains unchanged: scripts create safe data/commands; hosts own rendering, input, memory and OS access.
