#version 330 core

in vec2 v_uv;

uniform sampler2D u_texture;
uniform vec4 u_tint;
uniform int  u_useTexture;

out vec4 FragColor;

void main() {
    FragColor = (u_useTexture == 1) ? texture(u_texture, v_uv) * u_tint : u_tint;
}
