#version 410 core

// Full-screen quad vertex shader.
// Passes texture coordinates to the fragment shader.
layout(location = 0) in vec3 in_Position;
layout(location = 2) in vec2 in_TexCoord;

out vec2 texCoord;

void main(void) {
    gl_Position = vec4(in_Position, 1.0);
    texCoord    = in_TexCoord;
}


