#version 330 core

layout(location = 0) in vec3 a_position; // unit quad (z = 0)
layout(location = 1) in vec2 a_texCoord; // 0..1 across the quad

uniform mat4 u_viewProjection; // orthographic, in pixels
uniform mat4 u_transform;      // per-slot translate * scale
uniform vec2 u_uvMin;
uniform vec2 u_uvMax;

out vec2 v_uv;

void main() {
    v_uv        = mix(u_uvMin, u_uvMax, a_texCoord);
    gl_Position = u_viewProjection * u_transform * vec4(a_position, 1.0);
}
