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

// ═══════════════════════════════════════════════════════════════
// GLOBAL CONSTANTS & STATE
// ═══════════════════════════════════════════════════════════════
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

// ═══════════════════════════════════════════════════════════════
// FONT DATA (8x8 Bitmaps)
// ═══════════════════════════════════════════════════════════════
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

// ═══════════════════════════════════════════════════════════════
// PARTICLE SYSTEM (Equations + Attractor)
// ═══════════════════════════════════════════════════════════════
namespace RosslerSystem {
    GLuint glowShader, pointVAO, pointVBO;
    float a = 0.2f, b = 0.2f, c = 5.7f, dt = 0.007f;

    // Thomas's cyclically symmetric attractor
    float thomasB = 0.208186f;
    float thomasScale = 4.0f; // scale Thomas to match Rossler visual size

    // Morph oscillation state
    float morphTimer = 0.0f;
    float morphBlend = 0.0f; // 0 = Rossler, 1 = Thomas
    float rosslerDuration = 8.0f;
    float morphDuration = 3.0f;
    float thomasDuration = 8.0f;
    float cyclePeriod = 22.0f; // 8+3+8+3
    bool anomalyFlash = false;
    
    struct Particle {
        glm::vec3 pos;
        glm::vec3 vel;
        glm::vec3 target;
        glm::vec3 color;
        float size;
        float alpha;
        bool isText;
        bool isStar;
    };

    std::vector<Particle> particles;
    bool isCollapsing = false;
    float collapseTimer = 0.0f;
    float flashTimer = 0.0f;
    glm::vec3 blackholePos = glm::vec3(0.0f, 0.0f, 0.0f);
    float screenShake = 0.0f;

    glm::vec3 derivative(const glm::vec3& p) {
        return glm::vec3(-(p.y + p.z), p.x + a * p.y, b + p.z * (p.x - c));
    }

    glm::vec3 thomasDerivative(const glm::vec3& p) {
        float s = 1.0f / thomasScale;
        glm::vec3 ps = p * s;
        return glm::vec3(sin(ps.y) - thomasB*ps.x, sin(ps.z) - thomasB*ps.y, sin(ps.x) - thomasB*ps.z) * thomasScale;
    }

    glm::vec3 blendedDerivative(const glm::vec3& p, float blend) {
        return derivative(p) * (1.0f - blend) + thomasDerivative(p) * blend;
    }

    glm::vec3 rk4(const glm::vec3& p, float h) {
        glm::vec3 k1 = derivative(p), k2 = derivative(p + h * 0.5f * k1), k3 = derivative(p + h * 0.5f * k2), k4 = derivative(p + h * k3);
        return p + (h / 6.0f) * (k1 + 2.0f * k2 + 2.0f * k3 + k4);
    }

    glm::vec3 blendedRk4(const glm::vec3& p, float h, float blend) {
        auto d = [&](const glm::vec3& q) { return blendedDerivative(q, blend); };
        glm::vec3 k1 = d(p), k2 = d(p+h*0.5f*k1), k3 = d(p+h*0.5f*k2), k4 = d(p+h*k3);
        return p + (h/6.0f)*(k1 + 2.0f*k2 + 2.0f*k3 + k4);
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
                            p.vel = glm::vec3(0.0f);
                            p.color = color;
                            p.size = 2.5f;
                            p.alpha = 0.9f;
                            p.isText = true;
                            p.isStar = false;
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
void main() { gl_Position = projection * view * vec4(aPos, 1.0); gl_PointSize = aSize * (35.0 / gl_Position.w); vCol = aCol; })",
R"(#version 330 core
in vec4 vCol; out vec4 FragColor;
void main() { vec2 c = gl_PointCoord - vec2(0.5); float d = length(c); if (d > 0.5) discard;
float g = exp(-d*d*10.0)*1.5 + exp(-d*d*3.0)*0.5; FragColor = vec4(vCol.rgb * g, vCol.a * g); })");

        glGenVertexArrays(1, &pointVAO); glGenBuffers(1, &pointVBO);
        glBindVertexArray(pointVAO); glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
        // pos(3) + col(4) + size(1) = 8 floats per particle
        glBufferData(GL_ARRAY_BUFFER, 300000 * 8 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(7 * sizeof(float))); glEnableVertexAttribArray(2);

        particles.clear();
        isCollapsing = false; collapseTimer = 0.0f; flashTimer = 0.0f; screenShake = 0.0f;
        morphTimer = 0.0f; morphBlend = 0.0f; anomalyFlash = false;

        // Background star field
        std::mt19937 rng(42); std::uniform_real_distribution<float> d(-0.5f, 0.5f);
        std::uniform_real_distribution<float> starDist(-80.0f, 80.0f);
        for (int i = 0; i < 800; i++) {
            Particle p; p.pos = glm::vec3(starDist(rng), starDist(rng), starDist(rng));
            p.target = p.pos; p.vel = glm::vec3(0);
            float bright = 0.3f + 0.7f * ((float)(i % 10) / 10.0f);
            p.color = glm::vec3(0.7f + bright*0.3f, 0.8f + bright*0.2f, 1.0f);
            p.size = 0.5f + bright; p.alpha = 0.2f + bright * 0.4f;
            p.isText = false; p.isStar = true;
            particles.push_back(p);
        }

        // Attractor particles with speed-based color gradient
        for (int i = 0; i < 20000; i++) {
            Particle p;
            p.pos = glm::vec3(0.1f + d(rng), d(rng), d(rng));
            for (int w = 0; w < 500 + (i * 7) % 3000; w++) p.pos = rk4(p.pos, dt);
            p.target = p.pos; p.vel = glm::vec3(0.0f);
            float t = (float)(i % 1000) / 1000.0f;
            p.color = glm::vec3(0.9f + 0.1f*t, 0.15f + 0.45f*t, 0.05f + 0.15f*t);
            p.size = 1.2f + t * 1.0f; p.alpha = 0.5f + 0.3f * t;
            p.isText = false; p.isStar = false;
            particles.push_back(p);
        }

    }

    void renderOverlay() {
        // Title
        ImGui::SetNextWindowPos(ImVec2(WIDTH/2 - 200, 20), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(400, 50));
        ImGui::Begin("##Title", nullptr, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoScrollbar);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.6f, 1.0f));
        float titleW = ImGui::CalcTextSize("ROSSLER ATTRACTOR").x;
        ImGui::SetCursorPosX((400 - titleW) * 0.5f);
        ImGui::Text("ROSSLER ATTRACTOR");
        ImGui::PopStyleColor();
        ImGui::End();

        // Equations
        ImGui::SetNextWindowPos(ImVec2(WIDTH - 280, HEIGHT - 120), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(270, 110));
        ImGui::Begin("##Equations", nullptr, ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoScrollbar);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.4f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.55f, 0.3f, 1.0f));
        ImGui::Text("  dx/dt = -y - z");
        ImGui::Text("  dy/dt = x + ay");
        ImGui::Text("  dz/dt = b + z(x - c)");
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::Text("  a=%.2f  b=%.2f  c=%.2f", a, b, c);
        ImGui::PopStyleColor(3);
        ImGui::End();
    }

    void update(float dt_frame) {
        if (isCollapsing) { collapseTimer += dt_frame; flashTimer += dt_frame; }
        screenShake *= 0.95f;
        // Much slower ramp — takes 30 seconds to fully collapse
        float progress = glm::clamp(collapseTimer / 30.0f, 0.0f, 1.0f);
        float pull = progress * progress * 4.0f; // gentle cubic ramp

        for (auto& p : particles) {
            if (!isCollapsing) {
                if (p.isText) {
                    p.pos = p.target + glm::vec3(0, sin(glfwGetTime()*2.0f + p.target.x*0.5f)*0.1f, 0);
                } else if (!p.isStar) {
                    p.pos = rk4(p.pos, dt);
                }
            } else {
                if (p.isStar) {
                    if (progress > 0.7f) {
                        glm::vec3 toBH = blackholePos - p.pos;
                        p.vel += glm::normalize(toBH) * (progress - 0.7f) * 1.0f * dt_frame;
                        p.pos += p.vel * dt_frame;
                        p.alpha *= (1.0f - dt_frame * 0.1f * progress);
                    }
                    continue;
                }
                // Fade out the Rossler attractor particles instantly into the void
                p.alpha *= (1.0f - dt_frame * 5.0f);
                p.vel *= (1.0f - dt_frame * 2.0f);
                p.pos += p.vel * dt_frame;
                if (p.alpha < 0.0f) p.alpha = 0.0f;
                screenShake = glm::max(screenShake, progress * 0.03f);
            }
        }
    }

    void render(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& camPos) {
        glEnable(GL_PROGRAM_POINT_SIZE); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glUseProgram(glowShader);
        glUniformMatrix4fv(glGetUniformLocation(glowShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(glowShader, "projection"), 1, GL_FALSE, glm::value_ptr(proj));

        std::vector<float> vData;
        for (const auto& p : particles) {
            if (p.alpha < 0.01f) continue;
            vData.push_back(p.pos.x); vData.push_back(p.pos.y); vData.push_back(p.pos.z);
            vData.push_back(p.color.r); vData.push_back(p.color.g); vData.push_back(p.color.b); vData.push_back(p.alpha);
            vData.push_back(p.size);
        }

        if (!vData.empty()) {
            glBindVertexArray(pointVAO); glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, vData.size() * sizeof(float), vData.data());
            glDrawArrays(GL_POINTS, 0, (int)(vData.size() / 8));
        }

        if (isCollapsing) {
            float progress = glm::clamp(collapseTimer / 30.0f, 0.0f, 1.0f);
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
                for (int ring = 0; ring < 120; ring++) {
                    float ringT = (float)ring / 120.0f;
                    float baseRadius = ringR * 1.0f + ringT * 12.0f;
                    float orbSpd = 1.0f / sqrt(0.1f + ringT * 2.5f);
                    int ptsInRing = (int)(900 * progress * (1.0f + ringT));
                    
                    // Precompute math for the entire ring
                    float innerGlow = exp(-ringT * 3.0f);
                    float fade = 1.0f - ringT * ringT;
                    float puffiness = 0.05f + ringT * 0.8f;
                    float timeSpd = time * orbSpd * 8.0f;
                    float base_g_c = 0.1f + 0.1f * (1.0f - ringT);
                    float n1_offset = ringT * 20.0f;
                    float n2_offset = -ringT * 15.0f + time * 1.2f;
                    float n3_offset = ringT * 35.0f;
                    float pSizeBase = 12.0f + (1.0f - ringT) * 8.0f;
                    
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
                        float a_c = (0.015f + 0.035f * fade * innerGlow) * bri * progress * gas;
                        
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
                    for (int ring = 0; ring < 70; ring++) {
                        float ringT = (float)ring / 70.0f;
                        float baseLrx = ringR * 1.0f + ringT * 10.0f;
                        float baseLry = ringR * (0.95f + ringT * 0.45f);
                        float fSpd = 1.0f / sqrt(0.1f + ringT * 2.5f);
                        int pts = (int)(500 * progress * viewingAngleFactor * (1.0f + ringT));
                        
                        float innerGlow = exp(-ringT * 3.0f);
                        float fade = 1.0f - ringT * ringT;
                        float puffiness = 0.05f + ringT * 0.8f;
                        float timeSpd = time * fSpd * 8.0f;
                        float base_g_c = 0.1f + 0.1f * (1.0f - ringT);
                        float n1_offset = ringT * 20.0f;
                        float n2_offset = -ringT * 15.0f + time * 1.2f;
                        float n3_offset = ringT * 35.0f;
                        float pSizeBase = 12.0f + (1.0f - ringT) * 8.0f;
                        
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
                    for (int ring = 0; ring < 50; ring++) {
                        float ringT = (float)ring / 50.0f;
                        float baseLrx = ringR * 1.0f + ringT * 10.0f;
                        float baseLry = ringR * (0.9f + ringT * 0.3f);
                        float fSpd = 1.0f / sqrt(0.1f + ringT * 2.5f);
                        int pts = (int)(400 * progress * viewingAngleFactor * (1.0f + ringT));
                        
                        float innerGlow = exp(-ringT * 3.0f);
                        float fade = 1.0f - ringT * ringT;
                        float puffiness = 0.05f + ringT * 0.8f;
                        float timeSpd = time * fSpd * 8.0f;
                        float base_g_c = 0.1f + 0.1f * (1.0f - ringT);
                        float n1_offset = ringT * 20.0f;
                        float n2_offset = -ringT * 15.0f + time * 1.2f;
                        float n3_offset = ringT * 35.0f;
                        float pSizeBase = 12.0f + (1.0f - ringT) * 8.0f;
                        
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
                            float a_c = (0.015f + 0.035f * fade * innerGlow) * bri * progress * gas * viewingAngleFactor;
                            
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
            RosslerSystem::isCollapsing = true;
            RosslerSystem::flashTimer = 0.0f;
            RosslerSystem::screenShake = 0.5f;
        }
        ImGui::PopStyleColor();
        if (ImGui::Button("Reset Simulation", ImVec2(300, 30))) {
            RosslerSystem::isCollapsing = false; RosslerSystem::collapseTimer = 0.0f;
            RosslerSystem::init();
        }
        ImGui::Separator();
        ImGui::Text("Attractor Parameters:");
        ImGui::SliderFloat("a (Chaos)", &RosslerSystem::a, 0.1f, 0.5f);
        ImGui::SliderFloat("c (Scale)", &RosslerSystem::c, 2.0f, 15.0f);
        ImGui::Checkbox("Auto Rotate Camera", &autoRotate);
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

        // Flash effect on collapse initiation
        float flash = (RosslerSystem::flashTimer < 0.3f && RosslerSystem::isCollapsing) ? (0.3f - RosslerSystem::flashTimer) * 0.5f : 0.0f;
        glClearColor(0.005f + flash, 0.005f + flash * 0.5f, 0.01f + flash * 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        RosslerSystem::update(dt); RosslerSystem::render(view, proj, camP);
        RosslerSystem::renderOverlay();

        ImGui::Render(); ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
    glfwTerminate(); return 0;
}