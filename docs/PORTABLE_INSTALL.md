# VEK Portable Installation

VEK 2.1 is designed so the Windows release can be moved anywhere without reinstalling it.

## Fast setup

1. Download `VEK-v2.1.0-windows-x64.zip` from the GitHub Release.
2. Extract it.
3. Optionally rename the extracted folder to `vek`.
4. Move it anywhere, for example `C:\vek` or `D:\Dev\vek`.
5. Run `INSTALL_PATH.cmd` once.
6. Close Command Prompt and open a new one.
7. Run:

```bat
vek --version
vek info
vek doctor
```

No Git clone, CMake, Visual Studio or installer is required for users of the portable release.

## Manual PATH setup

If you do not want to run `INSTALL_PATH.cmd`, add the VEK folder itself to your User PATH. If VEK is in `C:\vek`, add exactly:

```text
C:\vek
```

`VEK_HOME` is optional and is not required. VEK normally discovers its installation from the location of `vek.exe`.

## Move VEK later

The installation is relocatable. If you move `C:\vek` to `D:\Languages\vek`, remove the old PATH entry and add the new folder. The VEK binaries themselves do not contain a hard-coded install path.

## Git clone remains supported

Developers and contributors can still use:

```bat
git clone https://github.com/defnot67kid-beep/VekProgramingLanguage.git
cd VekProgramingLanguage
BUILD_WINDOWS.bat
```

The ZIP release is for normal use; Git clone is for source development.
