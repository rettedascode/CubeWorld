# CubeEngine: reusable, game-agnostic static library.
# It hides GLFW/GLAD/OpenGL behind its own classes. GLM is exposed publicly
# because both the engine and the game use it for math.
find_package(glfw3 CONFIG REQUIRED)
find_package(glad  CONFIG REQUIRED)
find_package(glm   CONFIG REQUIRED)

add_library(CubeEngine STATIC
    src/core/Window.cpp
    src/core/Application.cpp
    src/input/Input.cpp
    src/render/Buffer.cpp
    src/render/VertexArray.cpp
    src/render/Shader.cpp
    src/render/Texture.cpp
    src/render/Camera.cpp
    src/render/Mesh.cpp
    src/render/Renderer.cpp
)

# include/ is PUBLIC so the game can do  #include <ce/...>
target_include_directories(CubeEngine PUBLIC include)

# Vendored single-header libs (stb_image) are an engine-private implementation detail.
target_include_directories(CubeEngine PRIVATE ${CMAKE_SOURCE_DIR}/external)

target_link_libraries(CubeEngine
    PUBLIC  glm::glm          # math types appear in engine public headers later
    PRIVATE glfw glad::glad   # windowing + GL loader stay hidden inside the engine
)

target_compile_features(CubeEngine PUBLIC cxx_std_17)
