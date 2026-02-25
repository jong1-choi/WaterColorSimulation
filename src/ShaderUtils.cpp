//
// ShaderUtils.cpp
// WaterColorSimulation
//
// OpenGL shader loading and uniform-setting implementations.
//
#include "ShaderUtils.h"

#include <fstream>
#include <iostream>
#include <vector>

#include <glm/gtc/type_ptr.hpp>

// --- Logging helpers -----------------------------------------------------------

void printShaderLog(GLuint shaderObj) {
    GLint logLength = 0;
    glGetShaderiv(shaderObj, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength <= 0) return;

    std::vector<char> log(logLength);
    GLint written = 0;
    glGetShaderInfoLog(shaderObj, logLength, &written, log.data());
    std::cerr << "[Shader] " << log.data() << "\n";
}

void printProgramLog(GLuint programObj) {
    GLint logLength = 0;
    glGetProgramiv(programObj, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength <= 0) return;

    std::vector<char> log(logLength);
    GLint written = 0;
    glGetProgramInfoLog(programObj, logLength, &written, log.data());
    std::cerr << "[Program] " << log.data() << "\n";
}

// --- File reading --------------------------------------------------------------

std::string readTextFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[ERROR] Cannot open file: " << filePath << "\n";
        return "";
    }
    return std::string(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());
}

// --- Shader compilation -------------------------------------------------------

GLuint loadShader(const std::string& filePath, GLuint shaderType) {
    std::string source = readTextFile(filePath);
    GLuint shader = glCreateShader(shaderType);

    const GLchar* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    printShaderLog(shader);

    return shader;
}

GLuint buildProgram(GLuint vertShader, GLuint fragShader) {
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vertShader);
    glAttachShader(prog, fragShader);
    glLinkProgram(prog);
    glUseProgram(prog);
    printProgramLog(prog);
    return prog;
}

std::tuple<GLuint, GLuint, GLuint> loadProgram(const std::string& vertPath,
                                                const std::string& fragPath) {
    GLuint vert = loadShader(vertPath, GL_VERTEX_SHADER);
    GLuint frag = loadShader(fragPath, GL_FRAGMENT_SHADER);
    GLuint prog = buildProgram(vert, frag);
    return { prog, vert, frag };
}

// --- Uniform setters -----------------------------------------------------------

void setUniform(GLuint prog, const std::string& name, int value) {
    glUniform1i(glGetUniformLocation(prog, name.c_str()), value);
}
void setUniform(GLuint prog, const std::string& name, float value) {
    glUniform1f(glGetUniformLocation(prog, name.c_str()), value);
}
void setUniform(GLuint prog, const std::string& name, const glm::ivec2& value) {
    glUniform2iv(glGetUniformLocation(prog, name.c_str()), 1, glm::value_ptr(value));
}
void setUniform(GLuint prog, const std::string& name, const glm::ivec3& value) {
    glUniform3iv(glGetUniformLocation(prog, name.c_str()), 1, glm::value_ptr(value));
}
void setUniform(GLuint prog, const std::string& name, const glm::vec2& value) {
    glUniform2fv(glGetUniformLocation(prog, name.c_str()), 1, glm::value_ptr(value));
}
void setUniform(GLuint prog, const std::string& name, const glm::vec3& value) {
    glUniform3fv(glGetUniformLocation(prog, name.c_str()), 1, glm::value_ptr(value));
}
void setUniform(GLuint prog, const std::string& name, const glm::vec4& value) {
    glUniform4fv(glGetUniformLocation(prog, name.c_str()), 1, glm::value_ptr(value));
}
void setUniform(GLuint prog, const std::string& name, const glm::mat3& value) {
    glUniformMatrix3fv(glGetUniformLocation(prog, name.c_str()), 1, GL_FALSE,
                       glm::value_ptr(value));
}
void setUniform(GLuint prog, const std::string& name, const glm::mat4& value) {
    glUniformMatrix4fv(glGetUniformLocation(prog, name.c_str()), 1, GL_FALSE,
                       glm::value_ptr(value));
}

// --- Fullscreen quad -----------------------------------------------------------

// Renders a NDC-space quad covering the entire viewport.
// The VAO is created once and reused across frames.
void drawFullscreenQuad() {
    struct QuadMesh {
        GLuint vao  = 0;
        GLuint vbo  = 0;
        GLuint ebo  = 0;

        void create() {
            // Positions (attr 0) and UV coords (attr 2) for two triangles
            const float vertices[] = {
                // x      y     z     u     v
                -1.0f,  1.0f, 0.0f, 0.0f, 0.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
                 1.0f,  1.0f, 0.0f, 1.0f, 0.0f,
                 1.0f, -1.0f, 0.0f, 1.0f, 1.0f,
            };
            const unsigned int indices[] = { 0, 1, 2,  2, 1, 3 };

            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);

            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

            // Position (location 0)
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                                  reinterpret_cast<void*>(0));
            // Tex coord (location 2, matching shader layout)
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                                  reinterpret_cast<void*>(3 * sizeof(float)));

            glGenBuffers(1, &ebo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

            glBindVertexArray(0);
        }
    };

    static QuadMesh quad;
    if (!quad.vao) quad.create();

    glBindVertexArray(quad.vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}
