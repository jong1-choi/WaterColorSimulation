//
// ShaderUtils.h
// WaterColorSimulation
//
// OpenGL shader loading and uniform-setting utilities.
// Originally part of GLTools (SpringMass project, Hyun Joon Shin, 2021).
// Adapted to use GLEW/GLFW directly (no JGL dependency).
//
#pragma once

#include <string>
#include <tuple>

#include <GL/glew.h>
#include <glm/glm.hpp>

// Prints the info log of a compiled shader object to stderr (if non-empty).
void printShaderLog(GLuint shaderObj);

// Prints the info log of a linked program object to stderr (if non-empty).
void printProgramLog(GLuint programObj);

// Reads the entire text content of a file. Returns an empty string on failure.
std::string readTextFile(const std::string& filePath);

// Compiles a GLSL source file and returns the shader object handle.
// shaderType is GL_VERTEX_SHADER or GL_FRAGMENT_SHADER.
GLuint loadShader(const std::string& filePath, GLuint shaderType);

// Links a vertex and fragment shader into a program. Does not delete the shaders.
GLuint buildProgram(GLuint vertShader, GLuint fragShader);

// Convenience: compile and link both shaders in one call.
// Returns {program, vertShader, fragShader}.
std::tuple<GLuint, GLuint, GLuint> loadProgram(const std::string& vertPath,
                                                const std::string& fragPath);

// --- Uniform setters (overloaded by value type) --------------------------------
void setUniform(GLuint prog, const std::string& name, int value);
void setUniform(GLuint prog, const std::string& name, float value);
void setUniform(GLuint prog, const std::string& name, const glm::ivec2& value);
void setUniform(GLuint prog, const std::string& name, const glm::ivec3& value);
void setUniform(GLuint prog, const std::string& name, const glm::vec2& value);
void setUniform(GLuint prog, const std::string& name, const glm::vec3& value);
void setUniform(GLuint prog, const std::string& name, const glm::vec4& value);
void setUniform(GLuint prog, const std::string& name, const glm::mat3& value);
void setUniform(GLuint prog, const std::string& name, const glm::mat4& value);

// Renders a full-screen quad covering NDC [-1,1] x [-1,1].
// Uses a lazily-created static VAO (safe to call every frame).
void drawFullscreenQuad();
