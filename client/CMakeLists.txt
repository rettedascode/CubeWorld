# CubeWorld client: renders the replicated simulation. Links the engine
# (rendering/window/input) and shared (headless sim). Meshing lives here because
# it produces GPU vertex data.
add_executable(CubeWorld
    src/main.cpp
    src/CubeWorldLayer.cpp
    src/render/ChunkMesher.cpp
    src/render/EntityRenderer.cpp
    src/render/HudRenderer.cpp
    src/player/PlayerController.cpp
    src/player/VoxelRaycast.cpp
    src/net/NetClient.cpp
    src/net/IntegratedServer.cpp
)

# CubeNet comes transitively via CubeShared (PUBLIC), but list it explicitly
# since the client uses net::Host / net::initialize directly.
target_link_libraries(CubeWorld PRIVATE CubeEngine CubeShared CubeNet)
target_compile_features(CubeWorld PRIVATE cxx_std_17)

# Root client includes at src/ so headers use "render/...", "player/..." paths.
# Shared's <cw/...> headers (and FastNoiseLite) come transitively from CubeShared.
target_include_directories(CubeWorld PRIVATE src)

# During development, load assets straight from the source tree by absolute path.
target_compile_definitions(CubeWorld PRIVATE
    CW_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets")
