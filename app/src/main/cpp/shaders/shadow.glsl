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

// ========== Fragment Shader (Soft Shadow) ==========
#version 310 es
precision highp float;

in vec2 v_texcoord;
out vec4 fragColor;

uniform vec2 u_resolution;
uniform float u_shadow_radius;  // نصف قطر الظل
uniform float u_shadow_alpha;   // شفافية الظل
uniform float u_corner_radius;  // زوايا النافذة

void main() {
    vec2 uv = v_texcoord;
    vec2 center = vec2(0.5);
    
    // حساب مسافة من الحواف للزوايا الدائرية
    vec2 dist_vec = max(abs(uv - center) * u_resolution - (u_resolution * 0.5 - u_corner_radius), 0.0);
    float dist = length(dist_vec) - u_corner_radius;
    
    // تدرج الظل (ناعم من الداخل للخارج)
    float shadow = 1.0 - smoothstep(0.0, u_shadow_radius, dist);
    
    // لون الظل (أسود شفاف)
    vec3 shadow_color = vec3(0.0, 0.0, 0.0);
    float alpha = shadow * u_shadow_alpha;
    
    fragColor = vec4(shadow_color, alpha);
}
