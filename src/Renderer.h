//
// Renderer.h
// WaterColorSimulation
//
// OpenGL renderer for the watercolor simulation output.
// Manages a float RGB texture that is updated each frame and displayed
// via a full-screen quad shader.
//
// Originally derived from TexView.hpp (Hyun Joon Shin, 2021) and the
// Tex helper struct, adapted for standalone GLFW/GLEW usage.
//
#pragma once

#include <string>
#include <GL/glew.h>

// Wraps a single OpenGL 2D texture that can be efficiently updated
// from a CPU-side float RGB buffer.
class SimulationTexture {
public:
    SimulationTexture() = default;
    ~SimulationTexture();

    // Creates or resizes the texture, then uploads pixel data.
    // format: e.g. GL_RGB   type: e.g. GL_FLOAT
    void upload(int width, int height, GLuint format, GLuint type, const void* data);

    // Binds the texture to the given texture unit and sets the sampler uniform.
    void bind(GLuint program, const std::string& uniformName, int unit = 0) const;

    GLuint handle() const { return m_texHandle; }

private:
    GLuint m_texHandle = 0;
    int    m_width     = 0;
    int    m_height    = 0;
    GLuint m_format    = GL_RGB;
    GLuint m_type      = GL_UNSIGNED_BYTE;
};

// Loads the watercolor output shaders and renders the simulation texture
// as a full-screen quad each frame.
class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    // Loads shaders from the given file paths. Must be called once after
    // an OpenGL context exists.
    void init(const std::string& vertShaderPath, const std::string& fragShaderPath);

    // Uploads new pixel data and redraws the full-screen quad.
    // data: pointer to width*height*3 floats (RGB, row-major)
    void render(const float* data, int width, int height);

private:
    GLuint           m_program  = 0;
    GLuint           m_vertShader = 0;
    GLuint           m_fragShader = 0;
    SimulationTexture m_texture;
};
