#version 330 core

layout(location = 0) in vec3  a_position;
layout(location = 1) in vec2  a_texCoord;
layout(location = 2) in float a_brightness;
layout(location = 3) in vec3  a_color; // biome foliage tint (white = none)

uniform mat4 u_viewProjection;
uniform mat4 u_transform;

out vec2  v_texCoord;
out float v_brightness;
out vec3  v_color;

void main() {
    v_texCoord   = a_texCoord;
    v_brightness = a_brightness;
    v_color      = a_color;
    gl_Position  = u_viewProjection * u_transform * vec4(a_position, 1.0);
}
