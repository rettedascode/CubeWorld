# CubeShared: the HEADLESS game simulation. Must compile and run with NO OpenGL,
# NO GLFW, NO window so it can run on a dedicated server. It may use glm and (later)
# the net library, but must NEVER depend on the engine or any GL headers.
find_package(glm CONFIG REQUIRED)

add_library(CubeShared STATIC
    src/world/Chunk.cpp
    src/world/World.cpp
    src/world/gen/WorldGenerator.cpp
    src/world/gen/BaseTerrainStage.cpp
    src/world/gen/BiomeStage.cpp
    src/world/gen/CaveStage.cpp
    src/world/gen/OreStage.cpp
    src/world/gen/FloraStage.cpp
    src/world/gen/Biome.cpp
    src/proto/Protocol.cpp
    src/item/Inventory.cpp
    src/craft/Crafting.cpp
    src/sim/Simulation.cpp
    src/sim/PlayerMovement.cpp
    src/server/Server.cpp
    src/server/Persistence.cpp
)

# Public headers are included as <cw/...>. external/ is PUBLIC because the public
# header gen/WorldGenerator.h includes the vendored single-header FastNoiseLite, so
# consumers (the server) must be able to find it too.
target_include_directories(CubeShared
    PUBLIC include ${CMAKE_SOURCE_DIR}/external)

# net is PUBLIC: the protocol headers expose net::ByteBuffer to consumers.
target_link_libraries(CubeShared PUBLIC glm::glm CubeNet)
target_compile_features(CubeShared PUBLIC cxx_std_17)
