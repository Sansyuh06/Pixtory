import re

with open('Source1.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Globals
content = re.sub(
r'    bool isCollapsing = false;\n    float collapseTimer = 0\.0f;\n    float flashTimer = 0\.0f;\n    glm::vec3 blackholePos = glm::vec3\(0\.0f, 0\.0f, 0\.0f\);\n    float screenShake = 0\.0f;\n    \n    // Hawking Reset Sequence State\n    bool isExploding = false;\n    float explodeTimer = 0\.0f;\n    float blackholeSpinSpeed = 1\.0f;\n    float hawkingRadiance = 0\.0f;\n    float simTime = 0\.0f;',
'''    // --- Unified Cinematic Master Loop ---
    float masterLoopTimer = 0.0f;
    const float LOOP_DURATION = 120.0f;
    bool explosionTriggered = false;
    
    float flashTimer = 0.0f;
    glm::vec3 blackholePos = glm::vec3(0.0f, 0.0f, 0.0f);
    float screenShake = 0.0f;
    
    float blackholeSpinSpeed = 1.0f;
    float hawkingRadiance = 0.0f;
    float simTime = 0.0f;''', content, count=1)

# 2. Particles optimization
content = content.replace(
    '// 1. Background web: 150,000 particles for dense ribbon look\n        for (int i = 0; i < 150000; i++) {',
    '// 1. Background web: 80,000 particles for optimized dense ribbon look\n        for (int i = 0; i < 80000; i++) {'
)

content = content.replace(
    '// 2. Foreground flowing tracer lines: 48,000 particles\n        for (int i = 0; i < 48000; i++) {',
    '// 2. Foreground flowing tracer lines: 24,000 particles\n        for (int i = 0; i < 24000; i++) {'
)

# 3. Render black hole check
content = content.replace(
    'if (isCollapsing) {\n            float progress = glm::clamp((collapseTimer - 30.0f) / 20.0f, 0.0f, 1.0f);',
    'bool drawBlackhole = (masterLoopTimer >= 60.0f && masterLoopTimer < 93.0f);\n        if (drawBlackhole) {\n            float progress = glm::clamp((masterLoopTimer - 60.0f) / 20.0f, 0.0f, 1.0f);'
)

# 4. Buttons in ImGui
buttons_old = """        if (ImGui::Button("!!! INITIATE COLLAPSE !!!", ImVec2(300, 60))) {
            RosslerSystem::isCollapsing = true;
            RosslerSystem::flashTimer = 0.0f;
        }
        ImGui::PopStyleColor();
        if (ImGui::Button("Reset Simulation", ImVec2(300, 30))) {
            if (!RosslerSystem::isExploding) {
                RosslerSystem::isExploding = true;
                RosslerSystem::explodeTimer = 0.0f;
                if (!RosslerSystem::isCollapsing) {
                    RosslerSystem::isCollapsing = true; 
                    RosslerSystem::collapseTimer = 60.0f; // Force black hole state for explosion
                }
            }
        }"""
buttons_new = """        if (ImGui::Button("!!! SKIP TO NEXT PHASE !!!", ImVec2(300, 60))) {
            // Fast forward to next major cinematic phase
            if (RosslerSystem::masterLoopTimer < 40.0f) RosslerSystem::masterLoopTimer = 40.0f; // To Halvorsen
            else if (RosslerSystem::masterLoopTimer < 60.0f) RosslerSystem::masterLoopTimer = 60.0f; // To Black Hole
            else if (RosslerSystem::masterLoopTimer < 90.0f) RosslerSystem::masterLoopTimer = 90.0f; // To Explosion
            else RosslerSystem::masterLoopTimer = 119.0f; // To Reset
        }
        ImGui::PopStyleColor();
        
        ImGui::Text("Cinematic Loop Time: %.1f / 120.0s", RosslerSystem::masterLoopTimer);
        ImGui::ProgressBar(RosslerSystem::masterLoopTimer / 120.0f, ImVec2(300, 15));"""

content = content.replace(buttons_old, buttons_new)

# 5. Replace update function completely
import re
update_pattern = re.compile(r'    void update\(float dt_frame\) \{.*?\n    \}\n', re.DOTALL)

update_new = """    void update(float dt_frame) {
        simTime += dt_frame * blackholeSpinSpeed;
        masterLoopTimer += dt_frame;
        if (masterLoopTimer >= LOOP_DURATION) {
            masterLoopTimer -= LOOP_DURATION;
            explosionTriggered = false;
        }

        float t = masterLoopTimer;
        
        // --- Calculate Timeline Phases ---
        float morphBlend = 0.0f;
        if (t < 40.0f) {
            float cyclePos = fmod(t, 20.0f);
            if (cyclePos < 10.0f) morphBlend = 0.0f;
            else {
                float mt = (cyclePos - 10.0f) / 10.0f;
                morphBlend = mt * mt * mt * (mt * (mt * 6.0f - 15.0f) + 10.0f);
            }
            if (t >= 20.0f) morphBlend = 1.0f - morphBlend;
        }
        
        float halvPhase = glm::clamp((t - 40.0f) / 10.0f, 0.0f, 1.0f);
        if (t > 96.0f) halvPhase = 0.0f;
        
        float spherePhase = glm::clamp((t - 50.0f) / 10.0f, 0.0f, 1.0f);
        if (t > 96.0f) spherePhase = 0.0f;
        
        float bhPhase = glm::clamp((t - 60.0f) / 20.0f, 0.0f, 1.0f);
        if (t > 93.0f) bhPhase = 0.0f;
        
        if (t > 90.0f && t < 93.0f) {
            blackholeSpinSpeed = 1.0f + ((t - 90.0f) / 3.0f) * 20.0f;
        } else if (t >= 93.0f) {
            blackholeSpinSpeed = 1.0f;
        }
        
        if (t >= 93.0f && !explosionTriggered) {
            explosionTriggered = true;
            hawkingRadiance = 1.0f;
            
            std::mt19937 rng(42);
            std::normal_distribution<float> nd(0, 1);
            
            std::for_each(std::execution::par_unseq, particles.begin(), particles.end(), [&](Particle& p) {
                if (p.isStar) return;
                glm::vec3 toP = p.pos - blackholePos;
                if (glm::length(toP) < 0.1f) toP = glm::vec3(nd(rng), nd(rng), nd(rng));
                toP = glm::normalize(toP);
                p.vel = toP * (100.0f + abs(nd(rng)) * 200.0f);
                p.pos = blackholePos + toP * 5.0f;
            });
        }
        
        if (t >= 93.0f && t < 96.0f) {
            hawkingRadiance = glm::clamp(1.0f - (t - 93.0f) / 3.0f, 0.0f, 1.0f);
        } else if (t >= 96.0f) {
            hawkingRadiance = 0.0f;
        }
        
        float reformPhase = 0.0f;
        if (t >= 96.0f) {
            reformPhase = glm::clamp((t - 96.0f) / 24.0f, 0.0f, 1.0f);
        }

        // --- Per-Particle Update (Parallel) ---
        float safe_dt = glm::min(dt_frame * 1.5f, 0.015f);
        
        std::for_each(std::execution::par_unseq, particles.begin(), particles.end(), [=](Particle& p) {
            if (p.isStar) {
                if (bhPhase > 0.7f && t < 93.0f) {
                    glm::vec3 toBH = blackholePos - p.pos;
                    p.vel += glm::normalize(toBH) * (bhPhase - 0.7f) * 1.0f * dt_frame;
                    p.pos += p.vel * dt_frame;
                    p.alpha *= (1.0f - dt_frame * 0.1f * bhPhase);
                }
                return;
            }
            
            p.target = rosslerRk4(p.target, safe_dt * 0.8f);
            p.thomasPos = thomasRk4(p.thomasPos, safe_dt * 3.0f);
            
            if (t >= 93.0f && t < 96.0f) {
                p.pos += p.vel * dt_frame;
                p.vel *= (1.0f - dt_frame * 1.5f);
                return;
            }
            
            if (t >= 96.0f) {
                glm::vec3 mathPos = p.target;
                glm::vec3 toTarget = mathPos - p.pos;
                p.vel += toTarget * dt_frame * (10.0f + reformPhase * 50.0f);
                p.vel *= (1.0f - dt_frame * 3.0f);
                p.pos += p.vel * dt_frame;
                p.alpha = glm::min(p.alpha + dt_frame * 0.5f, p.baseAlpha);
                
                // Keep thomasPos synchronized so it's ready for the next loop
                p.thomasPos = glm::mix(p.thomasPos, p.target, 0.0f); // we will morph naturally later
                return;
            }
            
            glm::vec3 currentPos = glm::mix(p.pos, p.thomasPos, morphBlend);
            glm::vec3 rPos = rosslerRk4(p.pos, safe_dt * 0.8f);
            glm::vec3 tPos = p.thomasPos;
            
            glm::vec3 hPos = halvorsenRk4(currentPos, safe_dt * 0.8f);
            
            glm::vec3 newPos = glm::mix(rPos, hPos, halvPhase);
            glm::vec3 newTPos = glm::mix(tPos, hPos, halvPhase);
            
            if (spherePhase > 0.0f) {
                float targetRadius = 15.0f;
                glm::vec3 sphereTarget = glm::normalize(hPos) * targetRadius;
                newPos = glm::mix(newPos, sphereTarget, spherePhase * 0.05f); 
                newTPos = newPos;
                
                float angSpd = spherePhase * 1.5f;
                float c = cos(angSpd * dt_frame);
                float s = sin(angSpd * dt_frame);
                float px = newPos.x * c - newPos.z * s;
                float pz = newPos.x * s + newPos.z * c;
                newPos.x = px;
                newPos.z = pz;
                newTPos.x = px;
                newTPos.z = pz;
            }
            
            if (bhPhase > 0.0f) {
                float pull = bhPhase * bhPhase * 150.0f;
                float swirl = bhPhase * 120.0f * blackholeSpinSpeed;
                
                glm::vec3 toBH = blackholePos - newPos;
                float dist = glm::length(toBH);
                
                glm::vec3 up(0, 1, 0);
                glm::vec3 tangent = dist > 0.1f ? glm::normalize(glm::cross(toBH, up)) : glm::vec3(1,0,0);
                
                glm::vec3 force = glm::normalize(toBH) * pull + tangent * swirl;
                
                p.vel += force * dt_frame;
                p.vel *= (1.0f - dt_frame * 1.2f);
                
                newPos += p.vel * dt_frame;
                newTPos = newPos;
                
                if (dist < 12.0f) {
                    p.alpha *= (1.0f - dt_frame * 2.5f);
                } else if (bhPhase > 0.8f) {
                    p.alpha *= (1.0f - dt_frame * 0.5f);
                }
                if (p.alpha < 0.0f) p.alpha = 0.0f;
            }
            
            p.pos = newPos;
            p.thomasPos = newTPos;
        });
        
        screenShake *= 0.93f;
        glitchIntensity *= (1.0f - dt_frame * 2.5f);
        chromaFlash *= (1.0f - dt_frame * 3.0f);
        if (glitchIntensity < 0.01f) glitchIntensity = 0.0f;
        if (chromaFlash < 0.01f) chromaFlash = 0.0f;
    }
"""
content = update_pattern.sub(update_new, content)

with open('Source1.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
