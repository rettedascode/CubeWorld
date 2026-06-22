# CubeWorld

A voxel game built on **CubeEngine**, a from-scratch, game-agnostic C++17
engine (GLFW + OpenGL 3.3 core + GLM).

- `engine/` — **CubeEngine**, reusable static library (`namespace ce`).
- `game/`   — **CubeWorld**, the voxel game executable (`namespace cw`).
- `external/` — vendored single-header libraries.

## Prerequisites

- Visual Studio 2022 with the **Desktop development with C++** workload
  (includes CMake and Ninja).
- [vcpkg](https://github.com/microsoft/vcpkg) cloned and bootstrapped, with the
  `VCPKG_ROOT` environment variable pointing at it.

Dependencies (`glfw3`, `glad`, `glm`) are declared in `vcpkg.json` and pulled
automatically in manifest mode during the CMake configure step.

## Build & run

See the build steps below — both the *Open Folder* and *generate a .sln*
workflows are supported via `CMakePresets.json`.
