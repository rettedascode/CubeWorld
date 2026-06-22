# CubeServer: headless dedicated server. A thin wrapper around the shared Server
# class (which holds the authoritative sim + tick loop). Links shared + net, NOT
# the engine — it must build and run with no OpenGL.
add_executable(CubeServer
    src/main.cpp
)

target_link_libraries(CubeServer PRIVATE CubeShared CubeNet)
target_compile_features(CubeServer PRIVATE cxx_std_17)
