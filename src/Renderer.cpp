//
// Renderer.cpp
// WaterColorSimulation
//
// OpenGL texture management and full-screen quad rendering.
//
#include "Renderer.h"
#include "ShaderUtils.h"

#include <iostream>
#include <glm/glm.hpp>

// --- SimulationTexture --------------------------------------------------------

SimulationTexture::~SimulationTexture() {
    if (m_texHandle > 0)
        glDeleteTextures(1, &m_texHandle);
}

void SimulationTexture::upload(int width, int height,
                                GLuint format, GLuint type,
                                const void* data) {
    // Recreate the texture object if dimensions or format changed
    if (width != m_width || height != m_height || format != m_format || type != m_type) {
        if (m_texHandle > 0) glDeleteTextures(1, &m_texHandle);
        m_texHandle = 0;
    }

    m_width  = width;
    m_height = height;
    m_format = format;
    m_type   = type;

    if (m_texHandle == 0) {
        // First upload: create and configure a new texture object
        glGenTextures(1, &m_texHandle);
        glBindTexture(GL_TEXTURE_2D, m_texHandle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, format, type, data);
    } else {
        // Subsequent uploads: just replace the pixel data (faster path)
        glBindTexture(GL_TEXTURE_2D, m_texHandle);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, format, type, data);
    }
}

void SimulationTexture::bind(GLuint program,
                              const std::string& uniformName,
                              int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_texHandle);
    setUniform(program, uniformName, unit);
}

// --- Renderer -----------------------------------------------------------------

Renderer::~Renderer() {
    if (m_program)    glDeleteProgram(m_program);
    if (m_vertShader) glDeleteShader(m_vertShader);
    if (m_fragShader) glDeleteShader(m_fragShader);
}

void Renderer::init(const std::string& vertShaderPath,
                    const std::string& fragShaderPath) {
    auto [prog, vert, frag] = loadProgram(vertShaderPath, fragShaderPath);
    m_program    = prog;
    m_vertShader = vert;
    m_fragShader = frag;
}

void Renderer::render(const float* data, int width, int height) {
    if (m_program == 0) {
        std::cerr << "[Renderer] Shaders not loaded. Call init() first.\n";
        return;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(m_program);

    // Upload the current simulation frame to the GPU texture
    m_texture.upload(width, height, GL_RGB, GL_FLOAT, data);
    m_texture.bind(m_program, "tex", 0);

    drawFullscreenQuad();
}
