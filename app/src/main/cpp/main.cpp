#include <android_native_app_glue.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <android/input.h>
#include <cmath>
#include <jni.h>
#include "wayland_client.h"

#define TAG "V-Viewer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

extern "C" {
    extern float get_bg_r(), get_bg_g(), get_bg_b();
    extern float get_accent_r(), get_accent_g(), get_accent_b();
}

static EGLDisplay display;
static EGLSurface surface;
static EGLContext context;
static GLuint program = 0;
static GLint uColorLoc = -1;

static int screen_w = 1920, screen_h = 1080;

// شريط علوي
static float bar_h = 0.07f;

// زر الإعدادات (ترس)
static float gear_cx, gear_cy, gear_r;
static bool gear_pressed = false;

// زر الطرفية
static float term_cx, term_cy;

// حالة الاتصال
static bool connected = false;

static void update_layout() {
    gear_r = bar_h * 0.5f; // ترس أكبر
    gear_cx = 0.94f;
    gear_cy = 1.0f - bar_h / 2.0f;

    term_cx = gear_cx - gear_r * 3.5f;
    term_cy = gear_cy;
}

static float to_ndc_x(float px) { return (px / screen_w) * 2.0f - 1.0f; }
static float to_ndc_y(float py) { return 1.0f - (py / screen_h) * 2.0f; }

const char* vs_src = R"(#version 310 es
precision highp float;
layout(location=0) in vec2 pos;
void main() { gl_Position = vec4(pos,0,1); })";

const char* fs_src = R"(#version 310 es
precision mediump float;
uniform vec4 uColor;
out vec4 fragColor;
void main() { fragColor = uColor; })";

static GLuint load_shader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    return s;
}

static void init_shaders() {
    GLuint vs = load_shader(GL_VERTEX_SHADER, vs_src);
    GLuint fs = load_shader(GL_FRAGMENT_SHADER, fs_src);
    program = glCreateProgram();
    glAttachShader(program, vs); glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs); glDeleteShader(fs);
    uColorLoc = glGetUniformLocation(program, "uColor");
}

// رسم خط
static void draw_line(float x1, float y1, float x2, float y2, float t, float r, float g, float b, float a) {
    float dx = x2 - x1, dy = y2 - y1;
    float len = sqrtf(dx*dx + dy*dy);
    if (len < 0.0001f) return;
    float nx = -dy / len * t * 0.5f;
    float ny =  dx / len * t * 0.5f;
    float verts[] = { x1-nx, y1-ny, x2-nx, y2-ny, x1+nx, y1+ny, x2-nx, y2-ny, x2+nx, y2+ny, x1+nx, y1+ny };
    glUseProgram(program);
    glUniform4f(uColorLoc, r,g,b,a);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,verts);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_TRIANGLES,0,6);
    glDisableVertexAttribArray(0);
}

// رسم مستطيل
static void draw_rect(float x, float y, float w, float h, float r, float g, float b, float a) {
    float verts[] = { x,y, x+w,y, x,y-h, x+w,y-h };
    glUseProgram(program);
    glUniform4f(uColorLoc, r,g,b,a);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,verts);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_TRIANGLE_STRIP,0,4);
    glDisableVertexAttribArray(0);
}

// رسم دائرة ممتلئة
static void draw_circle(float cx, float cy, float r, float red, float green, float blue, float a) {
    const int seg=32;
    float verts[(seg+2)*2];
    verts[0]=cx; verts[1]=cy;
    for(int i=0;i<=seg;i++) {
        float ang = 2*M_PI*i/seg;
        verts[2+i*2]=cx+cosf(ang)*r;
        verts[2+i*2+1]=cy+sinf(ang)*r;
    }
    glUseProgram(program);
    glUniform4f(uColorLoc, red,green,blue,a);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,verts);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_TRIANGLE_FAN,0,seg+2);
    glDisableVertexAttribArray(0);
}

// أيقونة ترس (دائرة بداخلها خطوط) - محسّنة
static void draw_gear(float cx, float cy, float r, float r_color, float g_color, float b_color) {
    // مربع خلفي للترس (خلفية شفافة داكنة)
    float sq = r * 1.6f;
    draw_rect(cx - sq, cy + sq, sq*2, sq*2, 0.15f, 0.15f, 0.15f, 0.8f);
    // إطار المربع
    draw_line(cx - sq, cy + sq, cx + sq, cy + sq, 0.005f, r_color, g_color, b_color, 0.6f);
    draw_line(cx + sq, cy + sq, cx + sq, cy - sq, 0.005f, r_color, g_color, b_color, 0.6f);
    draw_line(cx + sq, cy - sq, cx - sq, cy - sq, 0.005f, r_color, g_color, b_color, 0.6f);
    draw_line(cx - sq, cy - sq, cx - sq, cy + sq, 0.005f, r_color, g_color, b_color, 0.6f);
    // الترس نفسه
    draw_circle(cx, cy, r, r_color, g_color, b_color, 1.0f);
    float t = r*0.25f;
    float len = r*0.6f;
    draw_line(cx, cy+len, cx, cy+r*0.9f, t, 0.1f, 0.1f, 0.1f, 1.0f);
    draw_line(cx, cy-len, cx, cy-r*0.9f, t, 0.1f, 0.1f, 0.1f, 1.0f);
    draw_line(cx+len, cy, cx+r*0.9f, cy, t, 0.1f, 0.1f, 0.1f, 1.0f);
    draw_line(cx-len, cy, cx-r*0.9f, cy, t, 0.1f, 0.1f, 0.1f, 1.0f);
}

// أيقونة الطرفية (مستطيل مع >_ )
static void draw_terminal_icon(float cx, float cy, float r) {
    float sq = r * 1.6f;
    draw_rect(cx - sq, cy + sq, sq*2, sq*2, 0.15f, 0.15f, 0.15f, 0.8f);
    draw_line(cx - sq, cy + sq, cx + sq, cy + sq, 0.005f, 1.0f, 1.0f, 1.0f, 0.6f);
    draw_line(cx + sq, cy + sq, cx + sq, cy - sq, 0.005f, 1.0f, 1.0f, 1.0f, 0.6f);
    draw_line(cx + sq, cy - sq, cx - sq, cy - sq, 0.005f, 1.0f, 1.0f, 1.0f, 0.6f);
    draw_line(cx - sq, cy - sq, cx - sq, cy + sq, 0.005f, 1.0f, 1.0f, 1.0f, 0.6f);
    draw_line(cx - r*0.5f, cy + r*0.5f, cx + r*0.7f, cy + r*0.5f, 0.02f, 0.0f, 1.0f, 1.0f, 1.0f);
    draw_line(cx - r*0.3f, cy, cx + r*0.3f, cy, 0.02f, 1.0f, 1.0f, 1.0f, 1.0f);
    draw_line(cx, cy - r*0.5f, cx + r*0.7f, cy - r*0.5f, 0.02f, 0.0f, 1.0f, 1.0f, 1.0f);
}

// فتح شاشة الإعدادات
static void openSettings(ANativeActivity* activity) {
    JNIEnv* env;
    activity->vm->AttachCurrentThread(&env, NULL);
    jclass clazz = env->GetObjectClass(activity->clazz);
    jmethodID method = env->GetMethodID(clazz, "openSettings", "()V");
    env->CallVoidMethod(activity->clazz, method);
    activity->vm->DetachCurrentThread();
}

// فتح الطرفية
static void openTerminal(ANativeActivity* activity) {
    JNIEnv* env;
    activity->vm->AttachCurrentThread(&env, NULL);
    jclass clazz = env->GetObjectClass(activity->clazz);
    jmethodID method = env->GetMethodID(clazz, "openTerminal", "()V");
    env->CallVoidMethod(activity->clazz, method);
    activity->vm->DetachCurrentThread();
}

// محاولة الاتصال بـ Wayland
static void try_connect() {
    if (!connected) {
        connected = wayland_auto_connect();
        LOGI("Wayland connection attempt: %s", connected ? "SUCCESS" : "FAILED");
    }
}

// فحص لمس الزر
static bool in_rect(float px, float py, float cx, float cy, float size) {
    return (px >= cx - size && px <= cx + size && py >= cy - size && py <= cy + size);
}

// معالج اللمس
static int32_t handle_input(struct android_app* app, AInputEvent* event) {
    if (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION) return 0;
    float px = to_ndc_x(AMotionEvent_getX(event,0));
    float py = to_ndc_y(AMotionEvent_getY(event,0));
    int action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;

    if (action == AMOTION_EVENT_ACTION_DOWN) {
        // زر الإعدادات (مربع كامل)
        float sq = gear_r * 1.6f;
        if (in_rect(px, py, gear_cx, gear_cy, sq)) {
            gear_pressed = true;
            try_connect();
            openSettings(app->activity);
            return 1;
        }
        // زر الطرفية
        if (in_rect(px, py, term_cx, term_cy, sq)) {
            openTerminal(app->activity);
            return 1;
        }
    }
    else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) {
        gear_pressed = false;
        return 1;
    }
    return 0;
}

static void init_egl(ANativeWindow* window) {
    screen_w = ANativeWindow_getWidth(window);
    screen_h = ANativeWindow_getHeight(window);
    LOGI("Screen: %dx%d", screen_w, screen_h);
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display,0,0);
    const EGLint attribs[]={ EGL_SURFACE_TYPE,EGL_WINDOW_BIT, EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,
                              EGL_BLUE_SIZE,8, EGL_GREEN_SIZE,8, EGL_RED_SIZE,8, EGL_DEPTH_SIZE,16, EGL_NONE };
    EGLConfig cfg; EGLint n;
    eglChooseConfig(display,attribs,&cfg,1,&n);
    surface = eglCreateWindowSurface(display,cfg,window,0);
    const EGLint ctxAtt[]={EGL_CONTEXT_CLIENT_VERSION,3,EGL_NONE};
    context = eglCreateContext(display,cfg,NULL,ctxAtt);
    eglMakeCurrent(display,surface,surface,context);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    init_shaders();
    update_layout();
    try_connect();
}

static void term_egl() {
    if(program) glDeleteProgram(program);
    eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
    if(surface) eglDestroySurface(display,surface);
    if(context) eglDestroyContext(display,context);
    eglTerminate(display);
    display=EGL_NO_DISPLAY;
}

static void draw_frame() {
    glClearColor(get_bg_r(), get_bg_g(), get_bg_b(), 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // شريط علوي
    draw_rect(-1.0f, 1.0f, 2.0f, bar_h, 0.05f, 0.05f, 0.07f, 0.95f);

    // أيقونة الطرفية
    draw_terminal_icon(term_cx, term_cy, gear_r);

    // زر الترس
    float gcol_r = get_accent_r(), gcol_g = get_accent_g(), gcol_b = get_accent_b();
    if (gear_pressed) { gcol_r*=0.7f; gcol_g*=0.7f; gcol_b*=0.7f; }
    draw_gear(gear_cx, gear_cy, gear_r, gcol_r, gcol_g, gcol_b);

    // مؤشر حالة الاتصال (دائرة بجوار الترس)
    float status_x = gear_cx - gear_r*7.0f;
    float status_y = gear_cy;
    float status_r = gear_r*0.4f;
    if (connected) {
        draw_circle(status_x, status_y, status_r, 0.2f, 0.9f, 0.2f, 1.0f);
    } else {
        draw_circle(status_x, status_y, status_r, 0.9f, 0.2f, 0.2f, 1.0f);
    }

    eglSwapBuffers(display, surface);
}

static void handle_cmd(struct android_app* app, int32_t cmd) {
    switch(cmd) {
        case APP_CMD_INIT_WINDOW:
            if(app->window) { init_egl(app->window); app->onInputEvent = handle_input; }
            break;
        case APP_CMD_TERM_WINDOW: term_egl(); break;
    }
}

void android_main(struct android_app* app) {
    app_dummy();
    app->onAppCmd = handle_cmd;
    while(1) {
        int ident, events;
        android_poll_source* src;
        while((ident=ALooper_pollAll(0,NULL,&events,(void**)&src))>=0) {
            if(src) src->process(app,src);
        }
        if(display && program) draw_frame();
    }
}
