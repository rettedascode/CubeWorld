# Headless unit tests for CubeShared (GL-free simulation/worldgen).
find_package(Catch2 CONFIG REQUIRED)

add_executable(CubeSharedTests
    worldgen_tests.cpp
)

target_link_libraries(CubeSharedTests PRIVATE CubeShared Catch2::Catch2WithMain)
target_compile_features(CubeSharedTests PRIVATE cxx_std_17)
