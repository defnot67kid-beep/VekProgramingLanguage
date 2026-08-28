# VEK 1.5.0

VEK 1.5 expands the interaction SDK with secure garage/access-control foundations.

- GarageDoorRegistry and GarageDoorSystem
- Segmented garage metadata: size, panels, open/close durations, auto-close and lock policy
- Garage animation IDs and playback progress state
- PasslockRegistry and PasslockSystem
- Numeric code validation, attempt limits and timed lockout
- Safe passlock-to-garage linking by string ID
- GUI modal, password-input, status-badge and keypad commands
- New natives: garage_register, garage_exists, garage_open_duration, passlock_register, passlock_exists, passlock_max_digits, gui_begin_modal, gui_end_modal, gui_password_input, gui_status_badge and gui_keypad
- Interaction tests now cover garage locking/opening, PIN validation and the new GUI command types
- Existing VEK Guard boundary remains unchanged: native hosts own rendering, input devices, memory, OS access, cryptography and signature verification.
