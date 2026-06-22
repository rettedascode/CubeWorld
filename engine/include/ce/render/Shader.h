#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

namespace ce {

// RAII wrapper over a compiled+linked OpenGL shader program.
class Shader {
public:
    Shader(const std::string& vertexSrc, const std::string& fragmentSrc);
    ~Shader();

    Shader(const Shader&)            = delete;
    Shader& operator=(const Shader&) = delete;

    // Builds a shader from two GLSL source files.
    static std::unique_ptr<Shader> fromFiles(const std::string& vertexPath,
                                             const std::string& fragmentPath);

    void bind() const;
    void unbind() const;

    void setInt(const std::string& name, int value);
    void setFloat(const std::string& name, float value);
    void setFloat2(const std::string& name, const glm::vec2& v);
    void setFloat3(const std::string& name, const glm::vec3& v);
    void setFloat4(const std::string& name, const glm::vec4& v);
    void setMat4(const std::string& name, const glm::mat4& m);

private:
    int uniformLocation(const std::string& name);

    uint32_t                             m_id = 0;
    std::unordered_map<std::string, int> m_uniformCache;
};

} // namespace ce
