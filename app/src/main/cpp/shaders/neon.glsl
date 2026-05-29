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

// ========== Fragment Shader ==========
#version 310 es
precision highp float;

in vec2 v_texcoord;
out vec4 fragColor;

uniform vec3 neon_color;      // لون النيون
uniform float glow_intensity; // شدة التوهج
uniform float corner_radius;  // الزوايا الدائرية
uniform vec2 resolution;      // دقة الشاشة
uniform float time;           // وقت للحركة

void main() {
    vec2 uv = v_texcoord;
    vec2 center = vec2(0.5);
    
    // مسافة من الحافة (للإطار)
    float dist = length(max(abs(uv - center) * resolution - (resolution * 0.5 - corner_radius), 0.0)) - corner_radius;
    
    // عرض الإطار
    float border_width = 2.0;
    float border = smoothstep(border_width, 0.0, abs(dist));
    
    // توهج خارجي
    float glow = exp(-abs(dist) * 0.05) * glow_intensity;
    
    // وميض نيون متحرك
    float pulse = sin(time * 2.0) * 0.3 + 0.7;
    
    // تجميع اللون النهائي
    vec3 final_color = neon_color * (border + glow * 0.5) * pulse;
    float alpha = border * 0.9 + glow * 0.3;
    
    fragColor = vec4(final_color, alpha);
}
