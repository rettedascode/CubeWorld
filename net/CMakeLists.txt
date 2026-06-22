# CubeNet: reliable-UDP transport (ENet) + a little-endian ByteBuffer. Game-
# agnostic, NO OpenGL. Linked by both the server and the client.
find_package(unofficial-enet CONFIG REQUIRED)

add_library(CubeNet STATIC
    src/ByteBuffer.cpp
    src/Host.cpp
)

target_include_directories(CubeNet PUBLIC include)

# ENet is PRIVATE: consumers use net's wrapper, never <enet/enet.h>. (Symbols are
# still pulled into the final executables since CubeNet is a static lib.)
target_link_libraries(CubeNet PRIVATE unofficial::enet::enet)

# ENet needs the Windows sockets + multimedia-timer libs; the vcpkg target does
# not propagate them, so link them here (PUBLIC so the final executables get them).
if(WIN32)
    target_link_libraries(CubeNet PUBLIC ws2_32 winmm)
endif()

target_compile_features(CubeNet PUBLIC cxx_std_17)
