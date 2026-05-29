// ========== Vertex Shader ==========
#version 310 es
precision highp float;

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texcoord;

out vec2 v_texcoord;

void main() {
    gl_Position = vec4(position, 0.0, 1.0);
    v_texcoord = texcoord;
}

---

// ========== Fragment Shader (Gaussian Blur 9-tap) ==========
#version 310 es
precision highp float;

in vec2 v_texcoord;
out vec4 fragColor;

uniform sampler2D u_texture;
uniform vec2 u_texel_size;   // 1.0 / resolution
uniform float u_blur_radius; // نصف قطر الضبابية
uniform bool u_horizontal;   // أفقي أم عمودي

// أوزان Gaussian (انحراف معياري 2.0)
const float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    vec2 offset = u_horizontal ? vec2(u_texel_size.x * u_blur_radius, 0.0)
                               : vec2(0.0, u_texel_size.y * u_blur_radius);
    
    vec3 result = texture(u_texture, v_texcoord).rgb * weights[0];
    
    for (int i = 1; i < 5; i++) {
        result += texture(u_texture, v_texcoord + offset * float(i)).rgb * weights[i];
        result += texture(u_texture, v_texcoord - offset * float(i)).rgb * weights[i];
    }
    
    fragColor = vec4(result, 1.0);
}
