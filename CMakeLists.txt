# Top-level build: builds the reusable engine (static lib) first,
# then the game executable that links against it.
cmake_minimum_required(VERSION 3.20)
project(CubeWorld LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# All executables land in one bin/ dir so the game can find assets via
# relative paths and so the .exe is easy to locate after a build.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Targets (client/server split — see Phase 2 master prompt):
add_subdirectory(engine)   # CubeEngine  (static lib) rendering/window/input - CLIENT only
add_subdirectory(net)      # CubeNet     (static lib) ENet transport + ByteBuffer - NO OpenGL
add_subdirectory(shared)   # CubeShared  (static lib) headless simulation + protocol - NO OpenGL
add_subdirectory(server)   # CubeServer  (executable) headless authoritative sim
add_subdirectory(client)   # CubeWorld   (executable) renders the sim; links engine + shared

enable_testing()
add_subdirectory(tests)    # CubeSharedTests (Catch2) headless worldgen/sim tests
add_test(NAME CubeSharedTests COMMAND CubeSharedTests)
