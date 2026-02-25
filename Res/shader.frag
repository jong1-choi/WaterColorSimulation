#version 410 core

// Full-screen quad fragment shader.
// Samples the watercolor simulation output texture and outputs it directly.
in vec2 texCoord;

uniform sampler2D tex;  // Watercolor RGB output (bound from Renderer::render)

out vec4 out_Color;

void main(void) {
    out_Color = vec4(texture(tex, texCoord).rgb, 1.0);
}
