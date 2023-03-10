#version 410 core

in vec2 texCoord;

uniform sampler2D tex;
uniform sampler2D T;
uniform sampler2D R;

out vec4 out_Color;

void main(void) {
//	out_Color = vec4(pow(texture(tex,texCoord).r,1/2.2),pow(texture(tex,texCoord).g,1/2.2),pow(texture(tex,texCoord).b,1/2.2),1);
//	out_Color = vec4(vec3(texture(tex,texCoord).rgb),1);
//    out_Color = vec4(vec3(texture(tex,texCoord).r),1);
    out_Color = vec4(vec3(texture(R,texCoord).rgb),1);
    
//	out_Color = vec4(texCoord,0,1);
}
