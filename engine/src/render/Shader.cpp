#include "ce/render/Shader.h"
#include "ce/util/Log.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <sstream>
#include <vector>

namespace ce {

namespace {

uint32_t compile(GLenum stage, const std::string& src, const char* label) {
    uint32_t shader = glCreateShader(stage);
    const char* cstr = src.c_str();
    glShaderSource(shader, 1, &cstr, nullptr);
    glCompileShader(shader);

    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(static_cast<size_t>(len));
        glGetShaderInfoLog(shader, len, nullptr, log.data());
        CE_LOG_ERROR("Failed to compile ", label, " shader: ", log.data());
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

std::string readFile(const std::string& path) {
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in) {
        CE_LOG_ERROR("Could not open shader file: ", path);
        return {};
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

} // namespace

Shader::Shader(const std::string& vertexSrc, const std::string& fragmentSrc) {
    uint32_t vs = compile(GL_VERTEX_SHADER, vertexSrc, "vertex");
    uint32_t fs = compile(GL_FRAGMENT_SHADER, fragmentSrc, "fragment");
    if (!vs || !fs) return;

    m_id = glCreateProgram();
    glAttachShader(m_id, vs);
    glAttachShader(m_id, fs);
    glLinkProgram(m_id);

    GLint ok = GL_FALSE;
    glGetProgramiv(m_id, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(static_cast<size_t>(len));
        glGetProgramInfoLog(m_id, len, nullptr, log.data());
        CE_LOG_ERROR("Failed to link shader program: ", log.data());
        glDeleteProgram(m_id);
        m_id = 0;
    }

    // Shader objects are no longer needed once linked into the program.
    glDeleteShader(vs);
    glDeleteShader(fs);
}

Shader::~Shader() { glDeleteProgram(m_id); }

std::unique_ptr<Shader> Shader::fromFiles(const std::string& vertexPath,
                                          const std::string& fragmentPath) {
    return std::make_unique<Shader>(readFile(vertexPath), readFile(fragmentPath));
}

void Shader::bind() const { glUseProgram(m_id); }
void Shader::unbind() const { glUseProgram(0); }

int Shader::uniformLocation(const std::string& name) {
    auto it = m_uniformCache.find(name);
    if (it != m_uniformCache.end()) return it->second;

    int location = glGetUniformLocation(m_id, name.c_str());
    if (location == -1) CE_LOG_WARN("Uniform '", name, "' not found in shader");
    m_uniformCache[name] = location;
    return location;
}

void Shader::setInt(const std::string& name, int value) {
    glUniform1i(uniformLocation(name), value);
}
void Shader::setFloat(const std::string& name, float value) {
    glUniform1f(uniformLocation(name), value);
}
void Shader::setFloat2(const std::string& name, const glm::vec2& v) {
    glUniform2f(uniformLocation(name), v.x, v.y);
}
void Shader::setFloat3(const std::string& name, const glm::vec3& v) {
    glUniform3f(uniformLocation(name), v.x, v.y, v.z);
}
void Shader::setFloat4(const std::string& name, const glm::vec4& v) {
    glUniform4f(uniformLocation(name), v.x, v.y, v.z, v.w);
}
void Shader::setMat4(const std::string& name, const glm::mat4& m) {
    glUniformMatrix4fv(uniformLocation(name), 1, GL_FALSE, glm::value_ptr(m));
}

} // namespace ce
