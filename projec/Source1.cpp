#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <vector>
#include <iostream>
#include <string>
#include <map>
#include <random>
#include <algorithm>
#include <execution>

const int WIDTH = 1280;
const int HEIGHT = 720;
const int FONT_WIDTH = 8;
const int FONT_HEIGHT = 8;

glm::vec3 cameraPos = glm::vec3(0.0f, 5.0f, 30.0f);
glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
float cameraYaw = -90.0f, cameraPitch = 15.0f;
double cameraDistance = 30.0f;
bool mouseButtonPressed = false;
double lastMouseX = WIDTH / 2.0, lastMouseY = HEIGHT / 2.0;
bool autoRotate = true;


std::map<char, std::vector<uint8_t>> fontData = {
    {'R', {0x7C, 0x42, 0x42, 0x7C, 0x48, 0x44, 0x42, 0x00}},
    {'o', {0x00, 0x00, 0x3C, 0x42, 0x42, 0x42, 0x3C, 0x00}},
    {'s', {0x00, 0x00, 0x3E, 0x40, 0x3C, 0x02, 0x7C, 0x00}},
    {'l', {0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x3E, 0x00}},
    {'e', {0x00, 0x00, 0x3C, 0x42, 0x7E, 0x40, 0x3C, 0x00}},
    {'r', {0x00, 0x00, 0x5E, 0x60, 0x40, 0x40, 0x40, 0x00}},
    {' ', {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {'A', {0x18, 0x24, 0x42, 0x42, 0x7E, 0x42, 0x42, 0x00}},
    {'t', {0x10, 0x10, 0x7C, 0x10, 0x10, 0x10, 0x0E, 0x00}},
    {'c', {0x00, 0x00, 0x3C, 0x42, 0x40, 0x42, 0x3C, 0x00}},
    {'d', {0x02, 0x02, 0x3E, 0x42, 0x42, 0x42, 0x3E, 0x00}},
    {'x', {0x00, 0x00, 0x42, 0x24, 0x18, 0x24, 0x42, 0x00}},
    {'y', {0x00, 0x00, 0x42, 0x42, 0x3E, 0x02, 0x3C, 0x00}},
    {'z', {0x00, 0x00, 0x7E, 0x04, 0x18, 0x20, 0x7E, 0x00}},
    {'=', {0x00, 0x00, 0x7E, 0x00, 0x7E, 0x00, 0x00, 0x00}},
    {'-', {0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00}},
    {'+', {0x00, 0x10, 0x10, 0x7E, 0x10, 0x10, 0x00, 0x00}},
    {'/', {0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x00}},
    {'a', {0x00, 0x00, 0x3C, 0x02, 0x3E, 0x42, 0x3E, 0x00}},
    {'b', {0x40, 0x40, 0x7C, 0x42, 0x42, 0x42, 0x7C, 0x00}},
    {'(', {0x0C, 0x10, 0x20, 0x20, 0x20, 0x10, 0x0C, 0x00}},
    {')', {0x30, 0x08, 0x04, 0x04, 0x04, 0x08, 0x30, 0x00}},
};

// ═══════════════════════════════════════════════════════════════
// SHADER UTILITIES
// ═══════════════════════════════════════════════════════════════
GLuint compileShader(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, nullptr);
    glCompileShader(vertexShader);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
    glCompileShader(fragmentShader);
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}

namespace RosslerSystem {
    GLuint glowShader, pointVAO, pointVBO;
    float a = 0.2f, b = 0.2f, c = 5.7f, dt = 0.007f;

    // Thomas's cyclically symmetric attractor
    float thomasB = 0.208186f;
    float thomasScale = 5.0f; 
    // oscillation state
    float morphTimer = 0.0f;
    float morphBlend = 0.0f; 
    float phaseDuration = 60.0f; 
    float cyclePeriod = 120.0f; 
    bool anomalyFlash = false;

    float shockwaveTimer = -1.0f; 
    float shockwaveRadius = 0.0f;
    float glitchIntensity = 0.0f;
    float chromaFlash = 0.0f;
    int anomalyDirection = 0; 
    std::vector<glm::vec3> glitchParticles;
    std::vector<glm::vec3> glitchVelocities;
    std::vector<float> glitchLife;
    
    struct Particle {
        glm::vec3 pos;        // Rossler position (always flowing on Rossler)
        glm::vec3 thomasPos;  // Thomas position (always flowing on Thomas)
        glm::vec3 vel;
        glm::vec3 target;     // kept for text particles only
        glm::vec3 color;
        float size;
        float alpha;
        float baseAlpha;
        bool isText;
        bool isStar;
        bool isForeground;
    };

    std::vector<Particle> particles;
    enum SimState {
        STATE_OSCILLATING,
        STATE_COLLAPSING,
        STATE_BLACKHOLE,
        STATE_EXPLODING
    };
    SimState currentState = STATE_OSCILLATING;


    float stateTimer = 0.0f;
    bool explosionTriggered = false;
    
    float flashTimer = 0.0f;
    glm::vec3 blackholePos = glm::vec3(0.0f, 0.0f, 0.0f);
    float screenShake = 0.0f;
    
    float blackholeSpinSpeed = 1.0f;
    float hawkingRadiance = 0.0f;
    float simTime = 0.0f;

    // Rossler derivative
    glm::vec3 rosslerDeriv(const glm::vec3& p) {
        return glm::vec3(-(p.y + p.z), p.x + a * p.y, b + p.z * (p.x - c));
    }

    // Thomas cyclically symmetric attractor derivative
    // dx/dt = sin(y) - bx, dy/dt = sin(z) - by, dz/dt = sin(x) - bz
    // Scaled so its visual extent matches Rossler (~12-15 units)
    glm::vec3 thomasDeriv(const glm::vec3& p) {
        float s = thomasScale;
        float px = p.x / s, py = p.y / s, pz = p.z / s;
        float dx = sin(py) - thomasB * px;
        float dy = sin(pz) - thomasB * py;
        float dz = sin(px) - thomasB * pz;
        return glm::vec3(dx, dy, dz) * s;
    }

    // RK4 for Rossler
    glm::vec3 rosslerRk4(const glm::vec3& p, float h) {
        glm::vec3 k1 = rosslerDeriv(p);
        glm::vec3 k2 = rosslerDeriv(p + h * 0.5f * k1);
        glm::vec3 k3 = rosslerDeriv(p + h * 0.5f * k2);
        glm::vec3 k4 = rosslerDeriv(p + h * k3);
        return p + (h / 6.0f) * (k1 + 2.0f * k2 + 2.0f * k3 + k4);
    }

    // RK4 for Thomas
    glm::vec3 thomasRk4(const glm::vec3& p, float h) {
        glm::vec3 k1 = thomasDeriv(p);
        glm::vec3 k2 = thomasDeriv(p + h * 0.5f * k1);
        glm::vec3 k3 = thomasDeriv(p + h * 0.5f * k2);
        glm::vec3 k4 = thomasDeriv(p + h * k3);
        return p + (h / 6.0f) * (k1 + 2.0f * k2 + 2.0f * k3 + k4);
    }

    glm::vec3 halvorsenDeriv(const glm::vec3& p) {
        float a = 1.89f;
        glm::vec3 safeP = p;
        float len = glm::length(safeP);
        if (len > 12.0f) safeP = glm::normalize(safeP) * (12.0f + (len - 12.0f) * 0.1f);
        
        float dx = -a * safeP.x - 4.0f * safeP.y - 4.0f * safeP.z - safeP.y * safeP.y;
        float dy = -a * safeP.y - 4.0f * safeP.z - 4.0f * safeP.x - safeP.z * safeP.z;
        float dz = -a * safeP.z - 4.0f * safeP.x - 4.0f * safeP.y - safeP.x * safeP.x;
        return glm::vec3(dx, dy, dz);
    }
    
    glm::vec3 halvorsenRk4(const glm::vec3& p, float h) {
        glm::vec3 k1 = halvorsenDeriv(p);
        glm::vec3 k2 = halvorsenDeriv(p + h * 0.5f * k1);
        glm::vec3 k3 = halvorsenDeriv(p + h * 0.5f * k2);
        glm::vec3 k4 = halvorsenDeriv(p + h * k3);
        return p + (h / 6.0f) * (k1 + 2.0f * k2 + 2.0f * k3 + k4);
    }

    // Legacy alias
    glm::vec3 rk4(const glm::vec3& p, float h) { return rosslerRk4(p, h); }

    glm::vec3 blendedDeriv(const glm::vec3& p, float morph) {
        glm::vec3 r = rosslerDeriv(p);
        glm::vec3 t = thomasDeriv(p) * 2.5f; // speed up thomas to match flow visually
        glm::vec3 deriv = glm::mix(r, t, morph);
        
        // Stabilization: gently pull particles back if they diverge during morph
        float transitionFactor = sin(morph * 3.14159265f);
        if (transitionFactor > 0.0f) {
            float dist = glm::length(p);
            if (dist > 25.0f) {
                deriv -= p * ((dist - 25.0f) * 0.2f * transitionFactor);
            }
        }
        return deriv;
    }
    
    glm::vec3 blendedRk4(const glm::vec3& p, float h, float morph) {
        glm::vec3 k1 = blendedDeriv(p, morph);
        glm::vec3 k2 = blendedDeriv(p + h * 0.5f * k1, morph);
        glm::vec3 k3 = blendedDeriv(p + h * 0.5f * k2, morph);
        glm::vec3 k4 = blendedDeriv(p + h * k3, morph);
        return p + (h / 6.0f) * (k1 + 2.0f * k2 + 2.0f * k3 + k4);
    }

    void addTextParticles(const std::string& text, float startX, float startY, float z, float scale, glm::vec3 color) {
        float x = startX;
        for (char ch : text) {
            if (ch == '\n') { startY -= 18.0f * scale; x = startX; continue; }
            if (fontData.count(ch)) {
                auto& bitmap = fontData[ch];
                for (int row = 0; row < 8; ++row) {
                    uint8_t bits = bitmap[row];
                    for (int col = 0; col < 8; ++col) {
                        if (bits & (1 << (7 - col))) {
                            Particle p;
                            p.target = glm::vec3(x + col * scale, startY - row * scale, z);
                            p.pos = p.target;
                            p.thomasPos = p.target;
                            p.vel = glm::vec3(0.0f);
                            p.color = color;
                            p.size = 2.5f;
                            p.alpha = 0.9f;
                            p.baseAlpha = 0.9f;
                            p.isText = true;
                            p.isStar = false;
                            p.isForeground = false;
                            particles.push_back(p);
                        }
                    }
                }
            }
            x += 10.0f * scale;
        }
    }

    void init() {
        glowShader = compileShader(R"(#version 330 core
layout (location = 0) in vec3 aPos; layout (location = 1) in vec4 aCol; layout (location = 2) in float aSize;
out vec4 vCol; uniform mat4 view, projection;
uniform float uTime;
void main() { 
    vec3 p = aPos;
    vec4 col = aCol;
    
    // Dynamic cinematic tinting based on spatial position
    if (aCol.a < 0.8) {
        vec3 spatialCol = vec3(0.5) + 0.5 * cos(uTime*0.5 + p.xyx * 0.05 + vec3(0, 2, 4));
        col.rgb = mix(col.rgb, spatialCol * 1.5, 0.5);
    }
    
    gl_Position = projection * view * vec4(p, 1.0); 
    gl_PointSize = aSize * (35.0 / gl_Position.w); 
    vCol = col; 
})",
R"(#version 330 core
in vec4 vCol; out vec4 FragColor;
void main() { 
    vec2 c = gl_PointCoord - vec2(0.5); 
    float d = length(c); 
    if (d > 0.5) discard;
    // Enhanced HDR Bloom lighting profile
    float core = exp(-d*d*45.0) * 2.5;
    float glow = exp(-d*d*10.0) * 1.0;
    FragColor = vec4(vCol.rgb * (core + glow), vCol.a * glow); 
})");

        glGenVertexArrays(1, &pointVAO); glGenBuffers(1, &pointVBO);
        glBindVertexArray(pointVAO); glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
        // pos(3) + col(4) + size(1) = 8 floats per particle
        glBufferData(GL_ARRAY_BUFFER, 800000 * 8 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(7 * sizeof(float))); glEnableVertexAttribArray(2);

        particles.clear();
        flashTimer = 0.0f; screenShake = 0.0f;
        morphTimer = 0.0f; morphBlend = 0.0f; anomalyFlash = false;
        shockwaveTimer = -1.0f; shockwaveRadius = 0.0f; glitchIntensity = 0.0f; chromaFlash = 0.0f;
        glitchParticles.clear(); glitchVelocities.clear(); glitchLife.clear();

        // Background star field (sparse, subtle)
        std::mt19937 rng(42); std::uniform_real_distribution<float> d(-0.5f, 0.5f);
        std::uniform_real_distribution<float> starDist(-100.0f, 100.0f);
        for (int i = 0; i < 400; i++) {
            Particle p; p.pos = glm::vec3(starDist(rng), starDist(rng), starDist(rng));
            p.thomasPos = p.pos; p.target = p.pos; p.vel = glm::vec3(0);
            float bright = 0.3f + 0.7f * ((float)(i % 10) / 10.0f);
            p.color = glm::vec3(0.85f, 0.85f, 0.9f);
            p.size = 0.3f + bright * 0.4f; p.alpha = 0.1f + bright * 0.15f; p.baseAlpha = p.alpha;
            p.isText = false; p.isStar = true; p.isForeground = false;
            particles.push_back(p);
        }

        // Seed both attractors with independent warmup
        glm::vec3 rSeed = glm::vec3(0.1f, 0.0f, 0.0f);
        glm::vec3 tSeed = glm::vec3(1.0f, 1.1f, -0.1f); // Thomas seed (in scaled space)
        for(int w = 0; w < 5000; w++) {
            rSeed = rosslerRk4(rSeed, dt);
            tSeed = thomasRk4(tSeed, dt * 3.0f); // Thomas is slower, need bigger steps
        }
        
        // 1. Background web: 80,000 particles for optimized dense ribbon look
        for (int i = 0; i < 80000; i++) {
            rSeed = rosslerRk4(rSeed, dt * 0.12f);
            tSeed = thomasRk4(tSeed, dt * 0.5f);
            Particle p;
            p.pos = rSeed;          // Rossler position
            p.thomasPos = tSeed;    // Thomas position
            p.target = rSeed;
            p.vel = glm::vec3(0.0f);
            p.color = glm::vec3(0.9f, 0.95f, 1.0f);
            p.size = 1.0f;
            p.alpha = 0.25f;
            p.baseAlpha = 0.25f;
            p.isStar = false; p.isText = false; p.isForeground = false;
            particles.push_back(p);
        }

        // 2. The 4 distinct colored foreground trails
        glm::vec3 tColors[4] = {
            glm::vec3(1.0f, 0.2f, 0.2f), // Red
            glm::vec3(0.2f, 1.0f, 0.2f), // Green
            glm::vec3(0.2f, 0.4f, 1.0f), // Blue
            glm::vec3(1.0f, 0.9f, 0.2f)  // Yellow
        };
        for (int path = 0; path < 4; path++) {
            for (int w = 0; w < 8000; w++) {
                rSeed = rosslerRk4(rSeed, dt);
                tSeed = thomasRk4(tSeed, dt * 3.0f);
            }
            for (int i = 0; i < 12000; i++) {
                rSeed = rosslerRk4(rSeed, dt * 0.12f);
                tSeed = thomasRk4(tSeed, dt * 0.5f);
                Particle p;
                p.pos = rSeed;
                p.thomasPos = tSeed;
                p.target = rSeed;
                p.vel = glm::vec3(0.0f);
                p.color = tColors[path];
                p.size = 2.0f;
                float fadeIn = 0.05f + 0.9f * ((float)i / 12000.0f);
                p.alpha = fadeIn;
                p.baseAlpha = fadeIn;
                p.isStar = false; p.isText = false; p.isForeground = true;
                particles.push_back(p);
            }
        }

    }

    void renderOverlay() {
        bool showThomas = morphBlend > 0.5f;
        const char* title = showThomas ? "THOMAS ATTRACTOR" : "ROSSLER ATTRACTOR";
        // Title color: warm orange (Rossler) vs cool blue (Thomas)
        float tr = showThomas ? 0.6f : 1.0f;
        float tg = showThomas ? 0.8f : 0.85f;
        float tb = showThomas ? 1.0f : 0.6f;

        ImGui::SetNextWindowPos(ImVec2(WIDTH/2 - 200, 20), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(400, 50));
        ImGui::Begin("##Title", nullptr, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoScrollbar);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(tr, tg, tb, 1.0f));
        float titleW = ImGui::CalcTextSize(title).x;
        ImGui::SetCursorPosX((400 - titleW) * 0.5f);
        ImGui::Text("%s", title);
        ImGui::PopStyleColor();
        ImGui::End();

        // Equations
        float eqR = showThomas ? 0.4f : 1.0f;
        float eqG = showThomas ? 0.7f : 0.55f;
        float eqB = showThomas ? 1.0f : 0.3f;
        ImGui::SetNextWindowPos(ImVec2(WIDTH - 280, HEIGHT - 140), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(270, 130));
        ImGui::Begin("##Equations", nullptr, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoScrollbar);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.4f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(eqR, eqG, eqB, 1.0f));
        if (showThomas) {
            ImGui::Text("  dx/dt = sin(y) - bx");
            ImGui::Text("  dy/dt = sin(z) - by");
            ImGui::Text("  dz/dt = sin(x) - bz");
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            ImGui::Text("  b=%.6f", thomasB);
        } else {
            ImGui::Text("  dx/dt = -y - z");
            ImGui::Text("  dy/dt = x + ay");
            ImGui::Text("  dz/dt = b + z(x - c)");
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            ImGui::Text("  a=%.2f  b=%.2f  c=%.2f", a, b, c);
        }
        ImGui::PopStyleColor(3);
        ImGui::End();
    }

    void spawnAnomalyBurst(int direction) {
        anomalyDirection = direction;
        shockwaveTimer = 0.0f;
        shockwaveRadius = 0.0f;
        glitchIntensity = 1.0f;
        chromaFlash = 1.0f;
        screenShake = 0.8f;
        anomalyFlash = true;
        // Spawn glitch debris particles
        std::mt19937 rng((unsigned)glfwGetTime() * 10000);
        std::normal_distribution<float> nd(0, 1);
        glitchParticles.clear(); glitchVelocities.clear(); glitchLife.clear();
        for (int i = 0; i < 300; i++) {
            glitchParticles.push_back(glm::vec3(nd(rng)*2.0f, nd(rng)*2.0f, nd(rng)*2.0f));
            glitchVelocities.push_back(glm::normalize(glm::vec3(nd(rng), nd(rng), nd(rng))) * (5.0f + abs(nd(rng))*8.0f));
            glitchLife.push_back(1.0f);
        }
    }

    void update(float dt_frame) {
        simTime += dt_frame * blackholeSpinSpeed;
        
        float safe_dt = glm::min(dt_frame * 1.5f, 0.015f);
        
        if (currentState == STATE_OSCILLATING) {
            morphTimer += dt_frame;
            float cyclePos = fmod(morphTimer, 20.0f); // 10s hold, 10s transition
            float morphBlend = 0.0f;
            if (cyclePos < 10.0f) morphBlend = 0.0f;
            else {
                float mt = (cyclePos - 10.0f) / 10.0f;
                morphBlend = mt * mt * mt * (mt * (mt * 6.0f - 15.0f) + 10.0f);
            }
            if (fmod(morphTimer, 40.0f) >= 20.0f) morphBlend = 1.0f - morphBlend;
            RosslerSystem::morphBlend = morphBlend; // Global for rendering colors
            
            std::for_each(std::execution::par_unseq, particles.begin(), particles.end(), [=](Particle& p) {
                if (p.isStar) return;
                
                glm::vec3 rPos = rosslerRk4(p.pos, safe_dt * 0.8f);
                glm::vec3 tPos = thomasRk4(p.thomasPos, safe_dt * 3.0f);
                
                p.pos = rPos;
                p.thomasPos = tPos;
                
                // Track mathematical seeds as well
                p.target = rosslerRk4(p.target, safe_dt * 0.8f);
            });
        } 
        else if (currentState == STATE_COLLAPSING || currentState == STATE_BLACKHOLE) {
            if (currentState == STATE_COLLAPSING) {
                stateTimer += dt_frame;
                if (stateTimer >= 40.0f) currentState = STATE_BLACKHOLE;
            }
            
            float t = stateTimer;
            
            // 0 - 5s: Smoothly transition visual rendering entirely to p.thomasPos (which is now Halvorsen)
            float blendSpeed = dt_frame * 1.0f;
            RosslerSystem::morphBlend += (1.0f - RosslerSystem::morphBlend) * blendSpeed;
            
            float spherePhase = glm::clamp((t - 10.0f) / 10.0f, 0.0f, 1.0f);
            float bhPhase = glm::clamp((t - 20.0f) / 20.0f, 0.0f, 1.0f);
            
            std::for_each(std::execution::par_unseq, particles.begin(), particles.end(), [=](Particle& p) {
                if (p.isStar) {
                    if (bhPhase > 0.7f) {
                        glm::vec3 toBH = blackholePos - p.pos;
                        p.vel += glm::normalize(toBH) * (bhPhase - 0.7f) * 1.0f * dt_frame;
                        p.pos += p.vel * dt_frame;
                        p.alpha *= (1.0f - dt_frame * 0.1f * bhPhase);
                    }
                    return;
                }
                
                // Track 1: Rossler stays active in background
                p.pos = rosslerRk4(p.pos, safe_dt * 0.8f);
                p.target = rosslerRk4(p.target, safe_dt * 0.8f);
                
                // Track 2: Repurpose thomasPos to trace Halvorsen!
                // To prevent shock, blend the physics engine from Thomas to Halvorsen over 2s
                float mathBlend = glm::clamp(t / 2.0f, 0.0f, 1.0f);
                glm::vec3 tDeriv = thomasRk4(p.thomasPos, safe_dt * 3.0f);
                glm::vec3 hDeriv = halvorsenRk4(p.thomasPos, safe_dt * 1.5f);
                glm::vec3 currentTrack = glm::mix(tDeriv, hDeriv, mathBlend);
                
                if (spherePhase > 0.0f) {
                    float rHash = fmod(p.baseAlpha * 143.234f, 1.0f);
                    float targetRadius = 15.0f * pow(rHash, 0.333f); // Uniform solid sphere
                    
                    // Shrink it down as spherePhase approaches 1
                    targetRadius *= (1.0f - spherePhase * 0.8f); // Shrinks from 15 to 3
                    
                    glm::vec3 sphereTarget = glm::normalize(currentTrack) * targetRadius;
                    currentTrack = glm::mix(currentTrack, sphereTarget, spherePhase * 0.05f); 
                    
                    float angSpd = spherePhase * 2.5f; // Spin faster as it compresses
                    float c = cos(angSpd * dt_frame);
                    float s = sin(angSpd * dt_frame);
                    float px = currentTrack.x * c - currentTrack.z * s;
                    float pz = currentTrack.x * s + currentTrack.z * c;
                    currentTrack.x = px;
                    currentTrack.z = pz;
                }
                
                if (bhPhase > 0.0f) {
                    float pull = bhPhase * bhPhase * 150.0f;
                    float swirl = bhPhase * 120.0f * blackholeSpinSpeed;
                    
                    glm::vec3 toBH = blackholePos - currentTrack;
                    float dist = glm::length(toBH);
                    
                    glm::vec3 up(0, 1, 0);
                    glm::vec3 tangent = dist > 0.1f ? glm::normalize(glm::cross(toBH, up)) : glm::vec3(1,0,0);
                    
                    glm::vec3 force = glm::normalize(toBH) * pull + tangent * swirl;
                    
                    p.vel += force * dt_frame;
                    p.vel *= (1.0f - dt_frame * 1.2f);
                    
                    currentTrack += p.vel * dt_frame;
                    
                    if (dist < 12.0f) {
                        p.alpha *= (1.0f - dt_frame * 2.5f);
                    } else if (bhPhase > 0.8f) {
                        p.alpha *= (1.0f - dt_frame * 0.5f);
                    }
                    if (p.alpha < 0.0f) p.alpha = 0.0f;
                }
                
                p.thomasPos = currentTrack;
            });
        }
        else if (currentState == STATE_EXPLODING) {
            stateTimer += dt_frame;
            float t = stateTimer;
            
            if (t < 3.0f) {
                blackholeSpinSpeed = 1.0f + (t / 3.0f) * 20.0f;
                
                std::for_each(std::execution::par_unseq, particles.begin(), particles.end(), [=](Particle& p) {
                    if (p.isStar) return;
                    p.target = rosslerRk4(p.target, safe_dt * 0.8f);
                    
                    // Black hole continues to swirl them really fast
                    float pull = 150.0f;
                    float swirl = 120.0f * blackholeSpinSpeed;
                    glm::vec3 toBH = blackholePos - p.pos;
                    float dist = glm::length(toBH);
                    glm::vec3 up(0, 1, 0);
                    glm::vec3 tangent = dist > 0.1f ? glm::normalize(glm::cross(toBH, up)) : glm::vec3(1,0,0);
                    glm::vec3 force = glm::normalize(toBH) * pull + tangent * swirl;
                    p.vel += force * dt_frame;
                    p.vel *= (1.0f - dt_frame * 1.2f);
                    p.pos += p.vel * dt_frame;
                });
            } 
            else if (t >= 3.0f && !explosionTriggered) {
                explosionTriggered = true;
                blackholeSpinSpeed = 1.0f;
                hawkingRadiance = 1.0f;
                RosslerSystem::morphBlend = 0.0f; // Switch visual back to p.pos for the explosion
                
                std::mt19937 rng(42);
                std::normal_distribution<float> nd(0, 1);
                
                std::for_each(std::execution::par_unseq, particles.begin(), particles.end(), [&](Particle& p) {
                    if (p.isStar) return;
                    glm::vec3 toP = p.pos - blackholePos;
                    if (glm::length(toP) < 0.1f) toP = glm::vec3(nd(rng), nd(rng), nd(rng));
                    toP = glm::normalize(toP);
                    p.vel = toP * (100.0f + abs(nd(rng)) * 200.0f);
                    p.pos = blackholePos + toP * 5.0f;
                    p.thomasPos = p.pos; // Sync immediately
                    p.alpha = p.baseAlpha; // Instantly restore visibility so they shoot out
                });
            } 
            else if (t >= 3.0f && t < 6.0f) {
                hawkingRadiance = glm::clamp(1.0f - (t - 3.0f) / 3.0f, 0.0f, 1.0f);
                std::for_each(std::execution::par_unseq, particles.begin(), particles.end(), [=](Particle& p) {
                    if (p.isStar) return;
                    p.target = rosslerRk4(p.target, safe_dt * 0.8f);
                    p.pos += p.vel * dt_frame;
                    p.vel *= (1.0f - dt_frame * 1.5f);
                });
            } 
            else if (t >= 6.0f) {
                hawkingRadiance = 0.0f;
                float reformPhase = glm::clamp((t - 6.0f) / 24.0f, 0.0f, 1.0f);
                
                std::for_each(std::execution::par_unseq, particles.begin(), particles.end(), [=](Particle& p) {
                    if (p.isStar) return;
                    
                    p.target = rosslerRk4(p.target, safe_dt * 0.8f);
                    
                    // We need a proper tPos to restore them to Oscillation
                    glm::vec3 tPos = thomasRk4(p.pos, safe_dt * 3.0f); // Generate new thomas positions organically from their current space!
                    
                    glm::vec3 mathPos = p.target; // Re-form into rossler
                    glm::vec3 toTarget = mathPos - p.pos;
                    p.vel += toTarget * dt_frame * (10.0f + reformPhase * 50.0f);
                    p.vel *= (1.0f - dt_frame * 3.0f);
                    p.pos += p.vel * dt_frame;
                    p.alpha = glm::min(p.alpha + dt_frame * 0.5f, p.baseAlpha);
                    
                    // Resync thomasPos so it's ready when we switch states
                    p.thomasPos = tPos;
                });
                
                if (t >= 30.0f) {
                    currentState = STATE_OSCILLATING;
                    morphTimer = 0.0f;
                }
            }
        }
        
        screenShake *= 0.93f;
        glitchIntensity *= (1.0f - dt_frame * 2.5f);
        chromaFlash *= (1.0f - dt_frame * 3.0f);
        if (glitchIntensity < 0.01f) glitchIntensity = 0.0f;
        if (chromaFlash < 0.01f) chromaFlash = 0.0f;
    }

    void render(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& camPos) {
        glEnable(GL_PROGRAM_POINT_SIZE); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glUseProgram(glowShader);
        glUniformMatrix4fv(glGetUniformLocation(glowShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(glowShader, "projection"), 1, GL_FALSE, glm::value_ptr(proj));
        glUniform1f(glGetUniformLocation(glowShader, "uTime"), simTime);

        std::vector<float> vData;
        vData.reserve(particles.size() * 8 + 30000);
        float time = simTime;

        // Draw all flowing particles

        for (const auto& p : particles) {
            if (p.alpha < 0.01f) continue;
            if (p.isStar || p.isText) {
                vData.push_back(p.pos.x); vData.push_back(p.pos.y); vData.push_back(p.pos.z);
                vData.push_back(p.color.r); vData.push_back(p.color.g); vData.push_back(p.color.b);
                vData.push_back(p.alpha);
                vData.push_back(p.size);
                continue;
            }

            // Smoothly interpolate between the two independently-flowing positions
            glm::vec3 morphPos = glm::mix(p.pos, p.thomasPos, morphBlend);

            // Calculate colors based on current morph blend
            glm::vec3 rColor;
            float rAlpha;
            if (!p.isForeground) { // Background Web
                glm::vec3 rPink = glm::vec3(1.0f, 0.4f, 0.7f); // Bright pink (Rossler)
                glm::vec3 tGray = glm::vec3(0.85f, 0.85f, 0.9f); // Silver-white (Thomas)
                rColor = glm::mix(rPink, tGray, morphBlend);
                rAlpha = glm::mix(0.35f, 0.25f, morphBlend);
            } else { // Foreground colored paths
                glm::vec3 rPink = glm::vec3(1.0f, 0.2f, 0.8f); // Hot pink in Rossler
                glm::vec3 tCol = p.color; // Red/Green/Blue/Yellow in Thomas
                rColor = glm::mix(rPink, tCol, morphBlend);
                rAlpha = p.alpha;
            }

            // Chromatic aberration flash - shift hue during anomaly
            if (chromaFlash > 0.01f) {
                rColor.r += chromaFlash * 0.3f * sin(time*30.0f);
                rColor.g += chromaFlash * 0.2f * cos(time*25.0f);
                rColor.b += chromaFlash * 0.4f;
            }

            float jx = 0, jy = 0, jz = 0;
            if (glitchIntensity > 0.01f) {
                float ji = glitchIntensity * 0.5f;
                jx = sin(time*200.0f + morphPos.x*50.0f) * ji;
                jy = cos(time*170.0f + morphPos.y*50.0f) * ji;
                jz = sin(time*230.0f + morphPos.z*50.0f) * ji;
            }
            
            // Halvorsen color transition
            if (currentState == STATE_COLLAPSING || currentState == STATE_BLACKHOLE) {
                float halvColBlend = glm::clamp(stateTimer / 5.0f, 0.0f, 1.0f);
                rColor = glm::mix(rColor, glm::vec3(1.0f, 0.6f, 0.1f), halvColBlend); // Orange/Red/Yellow
            }

            float finalAlpha = rAlpha;
            if (chromaFlash > 0.01f) finalAlpha += chromaFlash * 0.2f;

            if (hawkingRadiance > 0.01f) {
                rColor = glm::mix(rColor, glm::vec3(1.0f, 0.9f, 0.6f), hawkingRadiance);
                finalAlpha = glm::mix(finalAlpha, 1.0f, hawkingRadiance);
            }

            vData.push_back(morphPos.x + jx); vData.push_back(morphPos.y + jy); vData.push_back(morphPos.z + jz);
            vData.push_back(rColor.r); vData.push_back(rColor.g); vData.push_back(rColor.b);
            vData.push_back(finalAlpha);
            vData.push_back(p.size * (1.0f + glitchIntensity * 0.3f));
        }

        // Shockwave ring particles
        if (shockwaveTimer >= 0.0f) {
            float swAlpha = glm::clamp(1.0f - shockwaveTimer / 2.0f, 0.0f, 1.0f);
            float ringThickness = 0.3f + shockwaveTimer * 0.5f;
            int ringPts = 2000;
            for (int i = 0; i < ringPts; i++) {
                float ang = (float)i / ringPts * 6.2832f;
                float r = shockwaveRadius + sin(ang * 12.0f + time * 20.0f) * ringThickness;
                float px = cos(ang) * r, py = sin(ang * 0.3f) * ringThickness * 0.5f, pz = sin(ang) * r;
                vData.push_back(px); vData.push_back(py); vData.push_back(pz);
                // Color: white-cyan for to-Thomas, white-orange for to-Rossler
                if (anomalyDirection > 0) {
                    vData.push_back(0.5f); vData.push_back(0.8f); vData.push_back(1.0f);
                } else {
                    vData.push_back(1.0f); vData.push_back(0.6f); vData.push_back(0.2f);
                }
                vData.push_back(swAlpha * 0.3f * (0.5f + 0.5f*sin(ang*8.0f)));
                vData.push_back(4.0f + swAlpha * 6.0f);
            }
        }

        // Glitch debris particles
        for (size_t i = 0; i < glitchParticles.size(); i++) {
            if (glitchLife[i] <= 0.0f) continue;
            vData.push_back(glitchParticles[i].x); vData.push_back(glitchParticles[i].y); vData.push_back(glitchParticles[i].z);
            float flicker = 0.5f + 0.5f * sin(time * 50.0f + (float)i * 7.0f);
            if (anomalyDirection > 0) {
                vData.push_back(0.4f*flicker); vData.push_back(0.6f*flicker); vData.push_back(1.0f);
            } else {
                vData.push_back(1.0f); vData.push_back(0.4f*flicker); vData.push_back(0.1f*flicker);
            }
            vData.push_back(glitchLife[i] * 0.6f * flicker);
            vData.push_back(2.0f + glitchLife[i] * 3.0f);
        }

        if (!vData.empty()) {
            glBindVertexArray(pointVAO); glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, vData.size() * sizeof(float), vData.data());
            glDrawArrays(GL_POINTS, 0, (int)(vData.size() / 8));
        }

        bool drawBlackhole = (currentState == STATE_COLLAPSING && stateTimer >= 20.0f) || (currentState == STATE_BLACKHOLE) || (currentState == STATE_EXPLODING && stateTimer < 3.0f);
        if (drawBlackhole) {
            float progress = 1.0f;
            if (currentState == STATE_COLLAPSING) progress = glm::clamp((stateTimer - 20.0f) / 20.0f, 0.0f, 1.0f);
            std::mt19937 rng((int)(glfwGetTime()*2000));
            std::normal_distribution<float> nd(0, 1);
            std::uniform_real_distribution<float> ud(0.0f, 1.0f);
            float time = (float)glfwGetTime();
            float bhR = 5.0f * progress; // sphere grows as collapse progresses

            // ══ PASS 1: DARK SPHERE (standard blending to block light) ══
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_TRUE);
            {
                std::vector<float> sphereData;
                // Dense dark sphere surface
                int spherePts = (int)(12000 * progress);
                for (int i = 0; i < spherePts; i++) {
                    // Fibonacci sphere distribution for even coverage
                    float golden = 3.14159f * (3.0f - sqrt(5.0f));
                    float y_s = 1.0f - ((float)i / (float)(spherePts - 1)) * 2.0f;
                    float radiusAtY = sqrt(1.0f - y_s * y_s);
                    float theta = golden * i;
                    glm::vec3 pos = blackholePos + glm::vec3(
                        cos(theta) * radiusAtY * bhR,
                        y_s * bhR,
                        sin(theta) * radiusAtY * bhR
                    );
                    // Add slight noise for organic look
                    pos += glm::vec3(nd(rng)*0.05f, nd(rng)*0.05f, nd(rng)*0.05f);
                    sphereData.push_back(pos.x); sphereData.push_back(pos.y); sphereData.push_back(pos.z);
                    sphereData.push_back(0.0f); sphereData.push_back(0.0f); sphereData.push_back(0.01f);
                    sphereData.push_back(0.99f * progress);
                    sphereData.push_back(12.0f);
                }
                if (!sphereData.empty()) {
                    glBufferSubData(GL_ARRAY_BUFFER, 0, sphereData.size()*sizeof(float), sphereData.data());
                    glDrawArrays(GL_POINTS, 0, (int)(sphereData.size()/8));
                }
            }

            // ══ PASS 2: BRIGHT ELEMENTS (additive blending for glow) ══
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glDepthMask(GL_FALSE);
            {
                std::vector<float> glowData;
                glowData.reserve(200000);
                float PI = 3.14159265f;
                float ringR = bhR * 1.15f;

                glm::vec3 camDir = glm::normalize(camPos - blackholePos);
                glm::vec3 worldUp = (abs(camDir.y) > 0.99f) ? glm::vec3(0, 0, -1) : glm::vec3(0, 1, 0);
                glm::vec3 camRight = glm::normalize(glm::cross(worldUp, camDir));
                glm::vec3 camUp = glm::cross(camDir, camRight);
                float viewingAngleFactor = 1.0f - abs(camDir.y);

                // ── ACCRETION DISK (Dense Flowing Plasma Clouds) ──
                for (int ring = 0; ring < 40; ring++) {
                    float ringT = (float)ring / 40.0f;
                    float baseRadius = ringR * 1.0f + ringT * 12.0f;
                    float orbSpd = 1.0f / sqrt(0.1f + ringT * 2.5f);
                    int ptsInRing = (int)(250 * progress * (1.0f + ringT));
                    
                    // Precompute math for the entire ring
                    float innerGlow = exp(-ringT * 3.0f);
                    float fade = 1.0f - ringT * ringT;
                    float puffiness = 0.05f + ringT * 0.8f;
                    float timeSpd = time * orbSpd * 8.0f;
                    float base_g_c = 0.1f + 0.1f * (1.0f - ringT);
                    float n1_offset = ringT * 20.0f;
                    float n2_offset = -ringT * 15.0f + time * 1.2f;
                    float n3_offset = ringT * 35.0f;
                    float pSizeBase = 35.0f + (1.0f - ringT) * 15.0f;
                    
                    float invPts = 1.0f / (float)ptsInRing;
                    
                    for (int i = 0; i < ptsInRing; i++) {
                        float ang = i * invPts * 6.2832f;
                        float cosA = cos(ang);
                        float sinA = sin(ang);
                        
                        float flowAng = ang - timeSpd;
                        float n1 = sin(flowAng * 6.0f + n1_offset);
                        float n2 = sin(flowAng * 13.0f + n2_offset);
                        float n3 = cos(flowAng * 29.0f + n3_offset);
                        float noise = (n1 * 0.5f + n2 * 0.25f + n3 * 0.125f) + 0.5f; 
                        
                        float t_c = noise < 0.2f ? 0.0f : (noise > 0.8f ? 1.0f : (noise - 0.2f) * 1.66667f);
                        float cloud = t_c * t_c * (3.0f - 2.0f * t_c);
                        
                        float yOffset = (noise - 0.5f) * puffiness * 2.0f;
                        
                        float px = cosA * baseRadius;
                        float pz = sinA * baseRadius;
                        
                        // Ultra-fast universal physical Doppler beaming
                        float distXZ = sqrt(px * px + pz * pz) + 0.0001f;
                        float vx = pz / distXZ;
                        float vz = -px / distXZ;
                        float approach = vx * camDir.x + vz * camDir.z;
                        float dop = 0.5f + 0.5f * approach;
                        
                        float dopSq = dop * dop;
                        float bri = 0.15f + 1.2f * (dopSq * dop); // ~ dop^3
                        
                        float g_c = base_g_c + 0.8f * dop * innerGlow;
                        float b_c = 0.15f * innerGlow * dop;
                        
                        float gas = 0.2f + 1.2f * cloud; 
                        float a_c = (0.045f + 0.1f * fade * innerGlow) * bri * progress * gas;
                        
                        glowData.push_back(blackholePos.x + px); 
                        glowData.push_back(blackholePos.y + yOffset); 
                        glowData.push_back(blackholePos.z + pz);
                        glowData.push_back(1.0f); glowData.push_back(g_c); glowData.push_back(b_c);
                        glowData.push_back(a_c);
                        glowData.push_back(pSizeBase + cloud * 8.0f); 
                    }
                }

                // ── TOP LENSING ARC (flowing lensed clouds) ──
                if (viewingAngleFactor > 0.02f) {
                    for (int ring = 0; ring < 20; ring++) {
                        float ringT = (float)ring / 20.0f;
                        float baseLrx = ringR * 1.0f + ringT * 10.0f;
                        float baseLry = ringR * (0.95f + ringT * 0.45f);
                        float fSpd = 1.0f / sqrt(0.1f + ringT * 2.5f);
                        int pts = (int)(150 * progress * viewingAngleFactor * (1.0f + ringT));
                        
                        float innerGlow = exp(-ringT * 3.0f);
                        float fade = 1.0f - ringT * ringT;
                        float puffiness = 0.05f + ringT * 0.8f;
                        float timeSpd = time * fSpd * 8.0f;
                        float base_g_c = 0.1f + 0.1f * (1.0f - ringT);
                        float n1_offset = ringT * 20.0f;
                        float n2_offset = -ringT * 15.0f + time * 1.2f;
                        float n3_offset = ringT * 35.0f;
                        float pSizeBase = 35.0f + (1.0f - ringT) * 15.0f;
                        
                        float invPts = PI / (float)pts;
                        
                        for (int i = 0; i < pts; i++) {
                            float ang = i * invPts;
                            float cosA = cos(ang);
                            float sinA = sin(ang);
                            
                            float flowAng = ang - timeSpd;
                            float n1 = sin(flowAng * 6.0f + n1_offset);
                            float n2 = sin(flowAng * 13.0f + n2_offset);
                            float n3 = cos(flowAng * 29.0f + n3_offset);
                            float noise = (n1 * 0.5f + n2 * 0.25f + n3 * 0.125f) + 0.5f;
                            
                            float t_c = noise < 0.2f ? 0.0f : (noise > 0.8f ? 1.0f : (noise - 0.2f) * 1.66667f);
                            float cloud = t_c * t_c * (3.0f - 2.0f * t_c);
                            
                            float yOffset = (noise - 0.5f) * puffiness * 2.0f;
                            
                            float localX = cosA * baseLrx + yOffset;
                            float localY = sinA * baseLry;
                            
                            float px = camRight.x * localX + camUp.x * localY;
                            float py = camRight.y * localX + camUp.y * localY;
                            float pz = camRight.z * localX + camUp.z * localY;
                                            
                            float distXZ = sqrt(px * px + pz * pz) + 0.0001f;
                            float vx = pz / distXZ;
                            float vz = -px / distXZ;
                            float approach = vx * camDir.x + vz * camDir.z;
                            float dop = 0.5f + 0.5f * approach;
                            
                            float dopSq = dop * dop;
                            float bri = 0.15f + 1.2f * (dopSq * dop);
                            
                            float g_c = base_g_c + 0.8f * dop * innerGlow;
                            float b_c = 0.15f * innerGlow * dop;
                            
                            float gas = 0.2f + 1.2f * cloud;
                            float a_c = (0.015f + 0.035f * fade * innerGlow) * bri * progress * gas * viewingAngleFactor;
                            
                            glowData.push_back(blackholePos.x + px); 
                            glowData.push_back(blackholePos.y + py); 
                            glowData.push_back(blackholePos.z + pz);
                            glowData.push_back(1.0f); glowData.push_back(g_c); glowData.push_back(b_c);
                            glowData.push_back(a_c);
                            glowData.push_back(pSizeBase + cloud * 8.0f);
                        }
                    }
                    // ── BOTTOM LENSING ARC ──
                    for (int ring = 0; ring < 20; ring++) {
                        float ringT = (float)ring / 20.0f;
                        float baseLrx = ringR * 1.0f + ringT * 10.0f;
                        float baseLry = ringR * (0.9f + ringT * 0.3f);
                        float fSpd = 1.0f / sqrt(0.1f + ringT * 2.5f);
                        int pts = (int)(150 * progress * viewingAngleFactor * (1.0f + ringT));
                        
                        float innerGlow = exp(-ringT * 3.0f);
                        float fade = 1.0f - ringT * ringT;
                        float puffiness = 0.05f + ringT * 0.8f;
                        float timeSpd = time * fSpd * 8.0f;
                        float base_g_c = 0.1f + 0.1f * (1.0f - ringT);
                        float n1_offset = ringT * 20.0f;
                        float n2_offset = -ringT * 15.0f + time * 1.2f;
                        float n3_offset = ringT * 35.0f;
                        float pSizeBase = 35.0f + (1.0f - ringT) * 15.0f;
                        
                        float invPts = PI / (float)pts;
                        
                        for (int i = 0; i < pts; i++) {
                            float ang = PI + i * invPts;
                            float cosA = cos(ang);
                            float sinA = sin(ang);
                            
                            float flowAng = ang - timeSpd;
                            float n1 = sin(flowAng * 6.0f + n1_offset);
                            float n2 = sin(flowAng * 13.0f + n2_offset);
                            float n3 = cos(flowAng * 29.0f + n3_offset);
                            float noise = (n1 * 0.5f + n2 * 0.25f + n3 * 0.125f) + 0.5f;
                            
                            float t_c = noise < 0.2f ? 0.0f : (noise > 0.8f ? 1.0f : (noise - 0.2f) * 1.66667f);
                            float cloud = t_c * t_c * (3.0f - 2.0f * t_c);
                            
                            float yOffset = (noise - 0.5f) * puffiness * 2.0f;
                            
                            float localX = cosA * baseLrx + yOffset;
                            float localY = sinA * baseLry;
                            
                            float px = camRight.x * localX + camUp.x * localY;
                            float py = camRight.y * localX + camUp.y * localY;
                            float pz = camRight.z * localX + camUp.z * localY;
                                            
                            float distXZ = sqrt(px * px + pz * pz) + 0.0001f;
                            float vx = pz / distXZ;
                            float vz = -px / distXZ;
                            float approach = vx * camDir.x + vz * camDir.z;
                            float dop = 0.5f + 0.5f * approach;
                            
                            float dopSq = dop * dop;
                            float bri = 0.15f + 1.2f * (dopSq * dop);
                            
                            float g_c = base_g_c + 0.8f * dop * innerGlow;
                            float b_c = 0.15f * innerGlow * dop;
                            
                            float gas = 0.2f + 1.2f * cloud;
                            float a_c = (0.045f + 0.1f * fade * innerGlow) * bri * progress * gas * viewingAngleFactor;
                            
                            glowData.push_back(blackholePos.x + px); 
                            glowData.push_back(blackholePos.y + py); 
                            glowData.push_back(blackholePos.z + pz);
                            glowData.push_back(1.0f); glowData.push_back(g_c); glowData.push_back(b_c);
                            glowData.push_back(a_c);
                            glowData.push_back(pSizeBase + cloud * 8.0f);
                        }
                    }
                }

                // ── PHOTON RING (crisp bright white line at shadow edge) ──
                for (int i = 0; i < 5000; i++) {
                    float ang = (float)i / 5000.0f * 6.2832f + time * 1.5f;
                    float pr = ringR * 0.98f;
                    glm::vec3 pos = blackholePos + camRight * cos(ang) * pr + camUp * sin(ang) * pr;
                    float dop = 0.3f + 0.7f * (0.5f + 0.5f * -cos(ang));
                    glowData.push_back(pos.x); glowData.push_back(pos.y); glowData.push_back(pos.z);
                    glowData.push_back(1.0f); glowData.push_back(0.85f); glowData.push_back(0.5f);
                    glowData.push_back(0.12f * progress * dop);
                    glowData.push_back(2.5f);
                }

                // ── QUASAR JETS (Relativistic Plasma Beams) ──
                if (progress > 0.25f) {
                    float jetP = glm::clamp((progress - 0.25f) / 0.5f, 0.0f, 1.0f);
                    for (int i = 0; i < 3000; i++) {
                        float t = (float)i / 3000.0f; 
                        float h = t * 70.0f * jetP; 
                        
                        // Stable pseudo-randomness for jet structure
                        float s1 = sin(i * 13.37f);
                        float s2 = cos(i * 9.42f);
                        float s3 = sin(i * 21.11f);
                        
                        float cone = 0.2f + t * 2.5f; 
                        
                        // Relativistic shockwaves traveling up
                        float flow = time * 25.0f - t * 30.0f;
                        float knot = pow(sin(flow * 1.5f + s1), 12.0f); // Bright dense clumps
                        
                        float tw = time * 20.0f + t * 50.0f + s2 * 6.28f;
                        float radius = cone * (0.1f + 0.9f * abs(s3)) * (1.0f - knot * 0.3f);
                        
                        glm::vec3 upPos = blackholePos + glm::vec3(cos(tw)*radius, h + bhR*0.8f, sin(tw)*radius);
                        glm::vec3 dnPos = blackholePos + glm::vec3(cos(tw+PI)*radius, -h - bhR*0.8f, sin(tw+PI)*radius);
                        
                        float heat = exp(-t * 5.0f);
                        float r_c = 0.6f * heat + 0.1f * knot;
                        float g_c = 0.8f * heat + 0.3f * knot;
                        float b_c = 1.0f; 
                        
                        float fade = exp(-t * 2.0f);
                        float a_c = (0.04f + 0.1f * knot + 0.2f * heat) * jetP * fade;
                        float size = (12.0f + t * 25.0f) + knot * 20.0f;
                        
                        glowData.push_back(upPos.x); glowData.push_back(upPos.y); glowData.push_back(upPos.z);
                        glowData.push_back(r_c); glowData.push_back(g_c); glowData.push_back(b_c);
                        glowData.push_back(a_c); glowData.push_back(size);
                        
                        glowData.push_back(dnPos.x); glowData.push_back(dnPos.y); glowData.push_back(dnPos.z);
                        glowData.push_back(r_c); glowData.push_back(g_c); glowData.push_back(b_c);
                        glowData.push_back(a_c); glowData.push_back(size);
                    }
                }

                if (!glowData.empty()) {
                    glBufferSubData(GL_ARRAY_BUFFER, 0, glowData.size()*sizeof(float), glowData.data());
                    glDrawArrays(GL_POINTS, 0, (int)(glowData.size()/8));
                }
            }
            glDepthMask(GL_TRUE);
        }

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDisable(GL_PROGRAM_POINT_SIZE);
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) { if (button == GLFW_MOUSE_BUTTON_LEFT) mouseButtonPressed = (action == GLFW_PRESS); }
void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (mouseButtonPressed) { cameraYaw += (float)(xpos - lastMouseX) * 0.15f; cameraPitch -= (float)(ypos - lastMouseY) * 0.15f; cameraPitch = glm::clamp(cameraPitch, -89.0f, 89.0f); }
    lastMouseX = xpos; lastMouseY = ypos;
}
void scrollCallback(GLFWwindow* window, double xoff, double yoff) { cameraDistance = glm::clamp(cameraDistance - yoff * 2.5, 5.0, 200.0); }

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Rossler Formula Black Hole", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window); glewInit();
    glfwSwapInterval(0); // Disable VSync to unlock framerate (120+ FPS)
    glEnable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glfwSetMouseButtonCallback(window, mouseButtonCallback); glfwSetCursorPosCallback(window, mouseCallback); glfwSetScrollCallback(window, scrollCallback);

    IMGUI_CHECKVERSION(); ImGui::CreateContext(); ImGui_ImplGlfw_InitForOpenGL(window, true); ImGui_ImplOpenGL3_Init("#version 330");
    RosslerSystem::init();

    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double current = glfwGetTime(); float dt = (float)(current - lastTime); lastTime = current;
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame(); ImGui_ImplGlfw_NewFrame(); ImGui::NewFrame();
        
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
        ImGui::Begin("BLACK HOLE SIMULATOR", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0, 0.8f, 0.8f));
        if (ImGui::Button("!!! INITIATE COLLAPSE !!!", ImVec2(300, 60))) {
            if (RosslerSystem::currentState == RosslerSystem::STATE_OSCILLATING) {
                RosslerSystem::currentState = RosslerSystem::STATE_COLLAPSING;
                RosslerSystem::stateTimer = 0.0f;
            }
        }
        ImGui::PopStyleColor();
        if (ImGui::Button("Reset Simulation", ImVec2(300, 30))) {
            if (RosslerSystem::currentState == RosslerSystem::STATE_BLACKHOLE || RosslerSystem::currentState == RosslerSystem::STATE_COLLAPSING) {
                RosslerSystem::currentState = RosslerSystem::STATE_EXPLODING;
                RosslerSystem::stateTimer = 0.0f;
                RosslerSystem::explosionTriggered = false;
            }
        }
        
        const char* stateName = "Oscillating";
        if (RosslerSystem::currentState == RosslerSystem::STATE_COLLAPSING) stateName = "Collapsing";
        else if (RosslerSystem::currentState == RosslerSystem::STATE_BLACKHOLE) stateName = "Black Hole";
        else if (RosslerSystem::currentState == RosslerSystem::STATE_EXPLODING) stateName = "Exploding";
        ImGui::Text("Current State: %s", stateName);
        ImGui::Separator();
        ImGui::Text("Rossler Parameters:");
        ImGui::SliderFloat("a (Chaos)", &RosslerSystem::a, 0.1f, 0.5f);
        ImGui::SliderFloat("c (Scale)", &RosslerSystem::c, 2.0f, 15.0f);
        ImGui::Separator();
        ImGui::Text("Thomas Parameter:");
        ImGui::SliderFloat("b (Damping)", &RosslerSystem::thomasB, 0.1f, 0.3f);
        ImGui::Separator();
        ImGui::Checkbox("Auto Rotate Camera", &autoRotate);
        ImGui::Text("Morph: %.0f%% %s", RosslerSystem::morphBlend * 100.0f, RosslerSystem::morphBlend > 0.5f ? "(Thomas)" : "(Rossler)");
        ImGui::End();

        if (autoRotate) cameraYaw += 0.3f;
        float shakeX = RosslerSystem::screenShake * sin(glfwGetTime() * 50.0f);
        float shakeY = RosslerSystem::screenShake * cos(glfwGetTime() * 37.0f);
        float cX = (float)(cameraDistance * cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch)));
        float cY = (float)(cameraDistance * sin(glm::radians(cameraPitch)));
        float cZ = (float)(cameraDistance * sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch)));
        glm::vec3 camP = glm::vec3(cX + shakeX, cY + shakeY, cZ);
        glm::mat4 view = glm::lookAt(camP, cameraTarget, glm::vec3(0, 1, 0));
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)WIDTH/HEIGHT, 0.1f, 1000.0f);

        // Flash effect on collapse initiation or anomaly morph
        float flash = (RosslerSystem::flashTimer < 0.3f) ? (0.3f - RosslerSystem::flashTimer) * 0.5f : 0.0f;
        if (RosslerSystem::anomalyFlash) { flash += 0.35f; RosslerSystem::anomalyFlash = false; }
        float cf = RosslerSystem::chromaFlash;
        float bgR = 0.005f + flash + cf * 0.15f * (RosslerSystem::anomalyDirection > 0 ? 0.3f : 1.0f);
        float bgG = 0.005f + flash * 0.5f + cf * 0.08f;
        float bgB = 0.01f + flash * 0.2f + cf * 0.15f * (RosslerSystem::anomalyDirection > 0 ? 1.0f : 0.3f);
        glClearColor(bgR, bgG, bgB, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        RosslerSystem::update(dt); RosslerSystem::render(view, proj, camP);
        RosslerSystem::renderOverlay();

        ImGui::Render(); ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
    glfwTerminate(); return 0;
}