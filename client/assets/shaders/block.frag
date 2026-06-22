#version 330 core

in vec2  v_texCoord;
in float v_brightness;
in vec3  v_color;

uniform sampler2D u_texture;

out vec4 FragColor;

void main() {
    vec4 tex = texture(u_texture, v_texCoord);
    FragColor = vec4(tex.rgb * v_brightness * v_color, tex.a);
}
