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

// Window size
const int WIDTH = 1280;
const int HEIGHT = 720;

// Font constants
const int FONT_WIDTH = 8;
const int FONT_HEIGHT = 8;

// Simulation enum
enum SimulationType {
    SIM_NONE,
    SIM_GRAVITY,
    SIM_FIXED_ELECTRIC,
    SIM_RADIATION,
    SIM_SPIN_SIMULATION,
    SIM_WIRE
};

SimulationType currentSim = SIM_NONE;

// Shared camera variables
glm::vec3 cameraPos = glm::vec3(0.0f, 5.0f, 12.0f);
glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
float cameraYaw = -90.0f, cameraPitch = -25.0f;
double cameraDistance = 12.0f;
bool mouseButtonPressed = false;
double lastMouseX = WIDTH / 2.0, lastMouseY = HEIGHT / 2.0;
bool autoRotate = true;

// Shared font data (abridged; add full data from your original code)
std::map<char, std::vector<uint8_t>> sharedFontData = {
    {'G', {0x3E, 0x40, 0x40, 0x4E, 0x42, 0x42, 0x3E, 0x00}},
    {'=', {0x00, 0x00, 0x7E, 0x00, 0x7E, 0x00, 0x00, 0x00}},
    {'m', {0x00, 0x00, 0x76, 0x49, 0x49, 0x49, 0x49, 0x00}},
    {' ', {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {'E', {0x7F, 0x40, 0x40, 0x7E, 0x40, 0x40, 0x7F, 0x00}},
    {'q', {0x00, 0x00, 0x3C, 0x22, 0x22, 0x3C, 0x04, 0x02}},
    {'B', {0x7C, 0x42, 0x42, 0x7C, 0x42, 0x42, 0x7C, 0x00}},
    {'z', {0x00, 0x00, 0x7E, 0x04, 0x18, 0x20, 0x7E, 0x00}},
    {'I', {0x3E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x3E, 0x00}},
    {'V', {0x42, 0x42, 0x42, 0x24, 0x24, 0x18, 0x18, 0x00}},
    {'R', {0x7C, 0x42, 0x42, 0x7C, 0x48, 0x44, 0x42, 0x00}},
    {'*', {0x00, 0x22, 0x14, 0x7F, 0x14, 0x22, 0x00, 0x00}},
    {'0', {0x3C, 0x42, 0x46, 0x4A, 0x52, 0x62, 0x3C, 0x00}},
    {'.', {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00}},
    // Add more characters if needed
};

// Shared OpenGL objects for text rendering
unsigned int sharedTextVAO = 0, sharedTextVBO = 0, sharedFontTexture = 0;

// Shader sources
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    uniform vec3 color;
    void main() {
        FragColor = vec4(color, 1.0);
    }
)";

const char* textVertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec4 vertex;
    out vec2 TexCoords;
    uniform mat4 projection;
    void main() {
        gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
        TexCoords = vertex.zw;
    }
)";

const char* textFragmentShaderSource = R"(
    #version 330 core
    in vec2 TexCoords;
    out vec4 FragColor;
    uniform sampler2D text;
    uniform vec3 textColor;
    void main() {
        vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
        FragColor = vec4(textColor, 1.0) * sampled;
    }
)";

GLuint compileShader(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, nullptr);
    glCompileShader(vertexShader);
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Vertex shader compilation failed: " << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Fragment shader compilation failed: " << infoLog << std::endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Shader program linking failed: " << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}

bool initSharedTextRendering() {
    const int texWidth = 16 * FONT_WIDTH;
    const int texHeight = 8 * FONT_HEIGHT;
    std::vector<uint8_t> textureData(texWidth * texHeight, 0);

    for (const auto& pair : sharedFontData) {
        char c = pair.first;
        const std::vector<uint8_t>& bitmap = pair.second;
        int charX = (c % 16) * FONT_WIDTH;
        int charY = (c / 16) * FONT_HEIGHT;

        for (int y = 0; y < FONT_HEIGHT; ++y) {
            uint8_t row = bitmap[y];
            for (int x = 0; x < FONT_WIDTH; ++x) {
                int pixelIndex = (charY + y) * texWidth + (charX + x);
                textureData[pixelIndex] = (row & (1 << (FONT_WIDTH - 1 - x))) ? 255 : 0;
            }
        }
    }

    glGenTextures(1, &sharedFontTexture);
    glBindTexture(GL_TEXTURE_2D, sharedFontTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, texWidth, texHeight, 0, GL_RED, GL_UNSIGNED_BYTE, textureData.data());

    glGenVertexArrays(1, &sharedTextVAO);
    glGenBuffers(1, &sharedTextVBO);
    glBindVertexArray(sharedTextVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sharedTextVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return true;
}

void sharedRenderText(GLuint shader, const std::string& text, float x, float y, float scale, glm::vec3 color) {
    glUseProgram(shader);
    glUniform3f(glGetUniformLocation(shader, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(sharedTextVAO);

    glm::mat4 projection = glm::ortho(0.0f, (float)WIDTH, 0.0f, (float)HEIGHT);
    glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    float startX = x;
    for (char c : text) {
        if (c == '\n') {
            y -= 30 * scale;
            x = startX;
            continue;
        }

        if (sharedFontData.find(c) == sharedFontData.end()) {
            c = ' ';
        }

        float xpos = x;
        float ypos = y;
        float w = 20 * scale;
        float h = 30 * scale;

        float texX = (c % 16) / 16.0f;
        float texY = (c / 16) / 8.0f;
        float texW = 1.0f / 16.0f;
        float texH = 1.0f / 8.0f;

        float vertices[6][4] = {
            { xpos,     ypos + h,   texX,         texY },
            { xpos,     ypos,       texX,         texY + texH },
            { xpos + w, ypos,       texX + texW,  texY + texH },
            { xpos,     ypos + h,   texX,         texY },
            { xpos + w, ypos,       texX + texW,  texY + texH },
            { xpos + w, ypos + h,   texX + texW,  texY }
        };

        glBindTexture(GL_TEXTURE_2D, sharedFontTexture);
        glBindBuffer(GL_ARRAY_BUFFER, sharedTextVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        x += w * 0.6f;
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        mouseButtonPressed = (action == GLFW_PRESS);
    }
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (mouseButtonPressed) {
        double deltaX = xpos - lastMouseX;
        double deltaY = ypos - lastMouseY;
        cameraYaw += deltaX * 0.1f;
        cameraPitch -= deltaY * 0.1f;
        cameraPitch = glm::clamp(cameraPitch, -89.0f, 89.0f);
    }
    lastMouseX = xpos;
    lastMouseY = ypos;
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    cameraDistance -= yoffset * 0.5f;
    cameraDistance = glm::clamp(cameraDistance, 2.0, 50.0);
}

glm::mat4 updateCamera() {
    if (autoRotate) {
        cameraYaw += 0.1f;
    }
    float camX = cameraDistance * cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
    float camY = cameraDistance * sin(glm::radians(cameraPitch));
    float camZ = cameraDistance * sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
    cameraPos = glm::vec3(camX, camY, camZ) + cameraTarget;
    return glm::lookAt(cameraPos, cameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));
}

namespace GravityField {
    GLuint shaderProgram, textShaderProgram, sphereVAO, sphereVBO, sphereEBO, lineVAO, lineVBO;
    std::vector<glm::vec3> masses;
    std::vector<float> massValues;
    std::vector<glm::vec3> velocities;
    std::vector<std::vector<glm::vec3>> fieldLines;
    std::vector<glm::vec3> testParticles;
    bool showFieldLines = true;
    bool showTestParticles = true;
    const float G = 0.1f;
    const float MASS = 1.0f;

    struct SphereVertex {
        glm::vec3 position;
    };

    void initSphere() {
        std::vector<SphereVertex> vertices;
        std::vector<unsigned int> indices;
        const int stacks = 20, slices = 20;
        const float radius = 0.2f;

        for (int i = 0; i <= stacks; ++i) {
            float phi = glm::pi<float>() * float(i) / stacks;
            float sinPhi = sin(phi), cosPhi = cos(phi);
            for (int j = 0; j <= slices; ++j) {
                float theta = 2.0f * glm::pi<float>() * float(j) / slices;
                float sinTheta = sin(theta), cosTheta = cos(theta);
                vertices.push_back({ glm::vec3(cosTheta * sinPhi, cosPhi, sinTheta * sinPhi) * radius });
            }
        }

        for (int i = 0; i < stacks; ++i) {
            for (int j = 0; j < slices; ++j) {
                int k1 = i * (slices + 1) + j;
                int k2 = k1 + slices + 1;
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
                indices.push_back(k1 + 1);
            }
        }

        glGenVertexArrays(1, &sphereVAO);
        glGenBuffers(1, &sphereVBO);
        glGenBuffers(1, &sphereEBO);
        glBindVertexArray(sphereVAO);
        glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(SphereVertex), vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SphereVertex), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    void initLine() {
        glGenVertexArrays(1, &lineVAO);
        glGenBuffers(1, &lineVBO);
        glBindVertexArray(lineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
        glBufferData(GL_ARRAY_BUFFER, 1000 * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    void init() {
        shaderProgram = compileShader(vertexShaderSource, fragmentShaderSource);
        textShaderProgram = compileShader(textVertexShaderSource, textFragmentShaderSource);
        initSphere();
        initLine();

        masses = { glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(2.0f, 0.0f, 0.0f) };
        massValues = { MASS, MASS };
        velocities = { glm::vec3(0.0f, 0.0f, 0.1f), glm::vec3(0.0f, 0.0f, -0.1f) };

        for (int i = 0; i < 20; ++i) {
            float angle = i * 2.0f * glm::pi<float>() / 20;
            glm::vec3 start = masses[0] + glm::vec3(cos(angle), 0.0f, sin(angle)) * 0.3f;
            std::vector<glm::vec3> line;
            glm::vec3 pos = start;
            for (int j = 0; j < 50; ++j) {
                line.push_back(pos);
                glm::vec3 force(0.0f);
                for (size_t k = 0; k < masses.size(); ++k) {
                    glm::vec3 r = masses[k] - pos;
                    float dist = glm::length(r);
                    if (dist > 0.1f) {
                        force += G * massValues[k] / (dist * dist) * glm::normalize(r);
                    }
                }
                pos += force * 0.05f;
            }
            fieldLines.push_back(line);
        }

        for (int i = 0; i < 10; ++i) {
            float x = ((float)rand() / RAND_MAX - 0.5f) * 5.0f;
            float z = ((float)rand() / RAND_MAX - 0.5f) * 5.0f;
            testParticles.push_back(glm::vec3(x, 0.0f, z));
        }
    }

    void cleanup() {
        glDeleteVertexArrays(1, &sphereVAO);
        glDeleteBuffers(1, &sphereVBO);
        glDeleteBuffers(1, &sphereEBO);
        glDeleteVertexArrays(1, &lineVAO);
        glDeleteBuffers(1, &lineVBO);
        glDeleteProgram(shaderProgram);
        glDeleteProgram(textShaderProgram);
        masses.clear();
        massValues.clear();
        velocities.clear();
        fieldLines.clear();
        testParticles.clear();
    }

    void update(float deltaTime) {
        for (size_t i = 0; i < masses.size(); ++i) {
            glm::vec3 force(0.0f);
            for (size_t j = 0; j < masses.size(); ++j) {
                if (i != j) {
                    glm::vec3 r = masses[j] - masses[i];
                    float dist = glm::length(r);
                    if (dist > 0.1f) {
                        force += G * massValues[j] / (dist * dist) * glm::normalize(r);
                    }
                }
            }
            velocities[i] += force * deltaTime / massValues[i];
            masses[i] += velocities[i] * deltaTime;
        }

        for (glm::vec3& pos : testParticles) {
            glm::vec3 force(0.0f);
            for (size_t i = 0; i < masses.size(); ++i) {
                glm::vec3 r = masses[i] - pos;
                float dist = glm::length(r);
                if (dist > 0.1f) {
                    force += G * massValues[i] / (dist * dist) * glm::normalize(r);
                }
            }
            pos += force * deltaTime * 0.1f;
        }
    }

    void render(const glm::mat4& view, const glm::mat4& projection) {
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(sphereVAO);
        glUniform3f(glGetUniformLocation(shaderProgram, "color"), 1.0f, 0.0f, 0.0f);
        for (const glm::vec3& pos : masses) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glDrawElements(GL_TRIANGLES, 20 * 20 * 6, GL_UNSIGNED_INT, 0);
        }

        if (showFieldLines) {
            glBindVertexArray(lineVAO);
            glUniform3f(glGetUniformLocation(shaderProgram, "color"), 0.0f, 1.0f, 0.0f);
            glm::mat4 model = glm::mat4(1.0f);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
            for (const auto& line : fieldLines) {
                glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, line.size() * sizeof(glm::vec3), line.data());
                glDrawArrays(GL_LINE_STRIP, 0, line.size());
            }
        }

        if (showTestParticles) {
            glUniform3f(glGetUniformLocation(shaderProgram, "color"), 0.0f, 0.0f, 1.0f);
            for (const glm::vec3& pos : testParticles) {
                glm::mat4 model = glm::translate(glm::mat4(1.0f), pos) * glm::scale(glm::mat4(1.0f), glm::vec3(0.1f));
                glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
                glDrawElements(GL_TRIANGLES, 20 * 20 * 6, GL_UNSIGNED_INT, 0);
            }
        }
        glBindVertexArray(0);

        sharedRenderText(textShaderProgram, "Gravity Simulation\nG = 0.1", 10.0f, HEIGHT - 40.0f, 0.5f, glm::vec3(1.0f));
    }
}

namespace FixedElectric {
    GLuint shaderProgram, textShaderProgram, sphereVAO, sphereVBO, sphereEBO, lineVAO, lineVBO;
    std::vector<glm::vec3> charges;
    std::vector<float> chargeValues;
    std::vector<glm::vec3> testCharges;
    std::vector<glm::vec3> testVelocities;
    std::vector<std::vector<glm::vec3>> fieldLines;
    bool showFieldLines = true;
    bool showTestCharges = true;
    const float K = 0.1f; // Coulomb constant
    const float CHARGE = 1.0f;

    struct SphereVertex {
        glm::vec3 position;
    };

    void initSphere() {
        std::vector<SphereVertex> vertices;
        std::vector<unsigned int> indices;
        const int stacks = 20, slices = 20;
        const float radius = 0.2f;

        for (int i = 0; i <= stacks; ++i) {
            float phi = glm::pi<float>() * float(i) / stacks;
            float sinPhi = sin(phi), cosPhi = cos(phi);
            for (int j = 0; j <= slices; ++j) {
                float theta = 2.0f * glm::pi<float>() * float(j) / slices;
                float sinTheta = sin(theta), cosTheta = cos(theta);
                vertices.push_back({ glm::vec3(cosTheta * sinPhi, cosPhi, sinTheta * sinPhi) * radius });
            }
        }

        for (int i = 0; i < stacks; ++i) {
            for (int j = 0; j < slices; ++j) {
                int k1 = i * (slices + 1) + j;
                int k2 = k1 + slices + 1;
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
                indices.push_back(k1 + 1);
            }
        }

        glGenVertexArrays(1, &sphereVAO);
        glGenBuffers(1, &sphereVBO);
        glGenBuffers(1, &sphereEBO);
        glBindVertexArray(sphereVAO);
        glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(SphereVertex), vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SphereVertex), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    void initLine() {
        glGenVertexArrays(1, &lineVAO);
        glGenBuffers(1, &lineVBO);
        glBindVertexArray(lineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
        glBufferData(GL_ARRAY_BUFFER, 1000 * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    void init() {
        shaderProgram = compileShader(vertexShaderSource, fragmentShaderSource);
        textShaderProgram = compileShader(textVertexShaderSource, textFragmentShaderSource);
        initSphere();
        initLine();

        charges = { glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(2.0f, 0.0f, 0.0f) };
        chargeValues = { CHARGE, -CHARGE };
        testVelocities = std::vector<glm::vec3>(10, glm::vec3(0.0f));

        for (int i = 0; i < 20; ++i) {
            float angle = i * 2.0f * glm::pi<float>() / 20;
            glm::vec3 start = charges[0] + glm::vec3(cos(angle), 0.0f, sin(angle)) * 0.3f;
            std::vector<glm::vec3> line;
            glm::vec3 pos = start;
            for (int j = 0; j < 50; ++j) {
                line.push_back(pos);
                glm::vec3 force(0.0f);
                for (size_t k = 0; k < charges.size(); ++k) {
                    glm::vec3 r = charges[k] - pos;
                    float dist = glm::length(r);
                    if (dist > 0.1f) {
                        force += K * chargeValues[k] / (dist * dist) * glm::normalize(r);
                    }
                }
                pos += force * 0.05f;
            }
            fieldLines.push_back(line);
        }

        for (int i = 0; i < 10; ++i) {
            float x = ((float)rand() / RAND_MAX - 0.5f) * 5.0f;
            float z = ((float)rand() / RAND_MAX - 0.5f) * 5.0f;
            testCharges.push_back(glm::vec3(x, 0.0f, z));
        }
    }

    void cleanup() {
        glDeleteVertexArrays(1, &sphereVAO);
        glDeleteBuffers(1, &sphereVBO);
        glDeleteBuffers(1, &sphereEBO);
        glDeleteVertexArrays(1, &lineVAO);
        glDeleteBuffers(1, &lineVBO);
        glDeleteProgram(shaderProgram);
        glDeleteProgram(textShaderProgram);
        charges.clear();
        chargeValues.clear();
        testCharges.clear();
        testVelocities.clear();
        fieldLines.clear();
    }

    void update(float deltaTime) {
        for (size_t i = 0; i < testCharges.size(); ++i) {
            glm::vec3 force(0.0f);
            for (size_t j = 0; j < charges.size(); ++j) {
                glm::vec3 r = charges[j] - testCharges[i];
                float dist = glm::length(r);
                if (dist > 0.1f) {
                    force += K * chargeValues[j] / (dist * dist) * glm::normalize(r);
                }
            }
            testVelocities[i] += force * deltaTime * 0.1f;
            testCharges[i] += testVelocities[i] * deltaTime;
        }
    }

    void render(const glm::mat4& view, const glm::mat4& projection) {
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(sphereVAO);
        for (size_t i = 0; i < charges.size(); ++i) {
            glUniform3f(glGetUniformLocation(shaderProgram, "color"), chargeValues[i] > 0 ? 1.0f : 0.0f, 0.0f, chargeValues[i] < 0 ? 1.0f : 0.0f);
            glm::mat4 model = glm::translate(glm::mat4(1.0f), charges[i]);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glDrawElements(GL_TRIANGLES, 20 * 20 * 6, GL_UNSIGNED_INT, 0);
        }

        if (showFieldLines) {
            glBindVertexArray(lineVAO);
            glUniform3f(glGetUniformLocation(shaderProgram, "color"), 0.0f, 1.0f, 0.0f);
            glm::mat4 model = glm::mat4(1.0f);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
            for (const auto& line : fieldLines) {
                glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, line.size() * sizeof(glm::vec3), line.data());
                glDrawArrays(GL_LINE_STRIP, 0, line.size());
            }
        }

        if (showTestCharges) {
            glUniform3f(glGetUniformLocation(shaderProgram, "color"), 0.0f, 0.0f, 1.0f);
            for (const glm::vec3& pos : testCharges) {
                glm::mat4 model = glm::translate(glm::mat4(1.0f), pos) * glm::scale(glm::mat4(1.0f), glm::vec3(0.1f));
                glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
                glDrawElements(GL_TRIANGLES, 20 * 20 * 6, GL_UNSIGNED_INT, 0);
            }
        }
        glBindVertexArray(0);

        sharedRenderText(textShaderProgram, "Electric Field\nK = 0.1", 10.0f, HEIGHT - 40.0f, 0.5f, glm::vec3(1.0f));
    }
}

namespace RadiationField {
    GLuint shaderProgram, textShaderProgram, lineVAO, lineVBO;
    std::vector<glm::vec3> wavePoints;
    float time = 0.0f;
    bool showWave = true;

    void initLine() {
        glGenVertexArrays(1, &lineVAO);
        glGenBuffers(1, &lineVBO);
        glBindVertexArray(lineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
        glBufferData(GL_ARRAY_BUFFER, 1000 * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    void init() {
        shaderProgram = compileShader(vertexShaderSource, fragmentShaderSource);
        textShaderProgram = compileShader(textVertexShaderSource, textFragmentShaderSource);
        initLine();

        for (int i = 0; i < 100; ++i) {
            float x = (i / 99.0f) * 10.0f - 5.0f;
            wavePoints.push_back(glm::vec3(x, 0.0f, 0.0f));
        }
    }

    void cleanup() {
        glDeleteVertexArrays(1, &lineVAO);
        glDeleteBuffers(1, &lineVBO);
        glDeleteProgram(shaderProgram);
        glDeleteProgram(textShaderProgram);
        wavePoints.clear();
    }

    void update(float deltaTime) {
        time += deltaTime;
        for (size_t i = 0; i < wavePoints.size(); ++i) {
            float x = (i / 99.0f) * 10.0f - 5.0f;
            wavePoints[i].y = sin(x * 2.0f + time * 2.0f) * 0.5f;
        }
    }

    void render(const glm::mat4& view, const glm::mat4& projection) {
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        if (showWave) {
            glBindVertexArray(lineVAO);
            glUniform3f(glGetUniformLocation(shaderProgram, "color"), 1.0f, 1.0f, 0.0f);
            glm::mat4 model = glm::mat4(1.0f);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, wavePoints.size() * sizeof(glm::vec3), wavePoints.data());
            glDrawArrays(GL_LINE_STRIP, 0, wavePoints.size());
            glBindVertexArray(0);
        }

        sharedRenderText(textShaderProgram, "Radiation Field", 10.0f, HEIGHT - 40.0f, 0.5f, glm::vec3(1.0f));
    }
}

namespace SpinSimulation {
    GLuint shaderProgram, textShaderProgram, sphereVAO, sphereVBO, sphereEBO;
    glm::vec3 particlePos;
    glm::vec3 particleVel;
    float spinAngle = 0.0f;
    const float B = 0.1f; // Magnetic field strength
    const float Q = 1.0f; // Charge
    bool showParticle = true;

    struct SphereVertex {
        glm::vec3 position;
    };

    void initSphere() {
        std::vector<SphereVertex> vertices;
        std::vector<unsigned int> indices;
        const int stacks = 20, slices = 20;
        const float radius = 0.2f;

        for (int i = 0; i <= stacks; ++i) {
            float phi = glm::pi<float>() * float(i) / stacks;
            float sinPhi = sin(phi), cosPhi = cos(phi);
            for (int j = 0; j <= slices; ++j) {
                float theta = 2.0f * glm::pi<float>() * float(j) / slices;
                float sinTheta = sin(theta), cosTheta = cos(theta);
                vertices.push_back({ glm::vec3(cosTheta * sinPhi, cosPhi, sinTheta * sinPhi) * radius });
            }
        }

        for (int i = 0; i < stacks; ++i) {
            for (int j = 0; j < slices; ++j) {
                int k1 = i * (slices + 1) + j;
                int k2 = k1 + slices + 1;
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
                indices.push_back(k1 + 1);
            }
        }

        glGenVertexArrays(1, &sphereVAO);
        glGenBuffers(1, &sphereVBO);
        glGenBuffers(1, &sphereEBO);
        glBindVertexArray(sphereVAO);
        glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(SphereVertex), vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SphereVertex), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    void init() {
        shaderProgram = compileShader(vertexShaderSource, fragmentShaderSource);
        textShaderProgram = compileShader(textVertexShaderSource, textFragmentShaderSource);
        initSphere();
        particlePos = glm::vec3(0.0f, 0.0f, 0.0f);
        particleVel = glm::vec3(0.1f, 0.0f, 0.0f);
    }

    void cleanup() {
        glDeleteVertexArrays(1, &sphereVAO);
        glDeleteBuffers(1, &sphereVBO);
        glDeleteBuffers(1, &sphereEBO);
        glDeleteProgram(shaderProgram);
        glDeleteProgram(textShaderProgram);
    }

    void update(float deltaTime) {
        glm::vec3 B_field(0.0f, 0.0f, B);
        glm::vec3 force = Q * glm::cross(particleVel, B_field);
        particleVel += force * deltaTime;
        particlePos += particleVel * deltaTime;
        spinAngle += 2.0f * deltaTime;
    }

    void render(const glm::mat4& view, const glm::mat4& projection) {
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        if (showParticle) {
            glBindVertexArray(sphereVAO);
            glUniform3f(glGetUniformLocation(shaderProgram, "color"), 0.0f, 1.0f, 1.0f);
            glm::mat4 model = glm::translate(glm::mat4(1.0f), particlePos) * glm::rotate(glm::mat4(1.0f), spinAngle, glm::vec3(0.0f, 1.0f, 0.0f));
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glDrawElements(GL_TRIANGLES, 20 * 20 * 6, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

        sharedRenderText(textShaderProgram, "Spin Simulation\nB = 0.1", 10.0f, HEIGHT - 40.0f, 0.5f, glm::vec3(1.0f));
    }
}

namespace OhmsLawWire {
    GLuint shaderProgram, textShaderProgram, lineVAO, lineVBO;
    std::vector<glm::vec3> wirePoints;
    std::vector<glm::vec3> currentParticles;
    std::vector<glm::vec3> particleVelocities;
    float voltage = 1.0f;
    float resistance = 1.0f;
    bool showCurrent = true;

    void initLine() {
        glGenVertexArrays(1, &lineVAO);
        glGenBuffers(1, &lineVBO);
        glBindVertexArray(lineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
        glBufferData(GL_ARRAY_BUFFER, 1000 * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    void init() {
        shaderProgram = compileShader(vertexShaderSource, fragmentShaderSource);
        textShaderProgram = compileShader(textVertexShaderSource, textFragmentShaderSource);
        initLine();

        for (int i = 0; i < 50; ++i) {
            float x = (i / 49.0f) * 10.0f - 5.0f;
            wirePoints.push_back(glm::vec3(x, 0.0f, 0.0f));
        }

        for (int i = 0; i < 10; ++i) {
            float x = ((float)rand() / RAND_MAX) * 10.0f - 5.0f;
            currentParticles.push_back(glm::vec3(x, 0.0f, 0.0f));
            particleVelocities.push_back(glm::vec3(voltage / resistance * 0.1f, 0.0f, 0.0f));
        }
    }

    void cleanup() {
        glDeleteVertexArrays(1, &lineVAO);
        glDeleteBuffers(1, &lineVBO);
        glDeleteProgram(shaderProgram);
        glDeleteProgram(textShaderProgram);
        wirePoints.clear();
        currentParticles.clear();
        particleVelocities.clear();
    }

    void update(float deltaTime) {
        for (size_t i = 0; i < currentParticles.size(); ++i) {
            currentParticles[i] += particleVelocities[i] * deltaTime;
            if (currentParticles[i].x > 5.0f) {
                currentParticles[i].x -= 10.0f;
            }
        }
    }

    void render(const glm::mat4& view, const glm::mat4& projection) {
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(lineVAO);
        glUniform3f(glGetUniformLocation(shaderProgram, "color"), 0.5f, 0.5f, 0.5f);
        glm::mat4 model = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, wirePoints.size() * sizeof(glm::vec3), wirePoints.data());
        glDrawArrays(GL_LINE_STRIP, 0, wirePoints.size());

        if (showCurrent) {
            glUniform3f(glGetUniformLocation(shaderProgram, "color"), 1.0f, 0.0f, 1.0f);
            for (const glm::vec3& pos : currentParticles) {
                model = glm::translate(glm::mat4(1.0f), pos) * glm::scale(glm::mat4(1.0f), glm::vec3(0.1f));
                glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
                glDrawArrays(GL_POINTS, 0, 1);
            }
        }
        glBindVertexArray(0);

        sharedRenderText(textShaderProgram, "Ohm's Law Wire\nV = I * R", 10.0f, HEIGHT - 40.0f, 0.5f, glm::vec3(1.0f));
    }
}

void switchSimulation(SimulationType newSim) {
    switch (currentSim) {
    case SIM_GRAVITY: GravityField::cleanup(); break;
    case SIM_FIXED_ELECTRIC: FixedElectric::cleanup(); break;
    case SIM_RADIATION: RadiationField::cleanup(); break;
    case SIM_SPIN_SIMULATION: SpinSimulation::cleanup(); break;
    case SIM_WIRE: OhmsLawWire::cleanup(); break;
    default: break;
    }

    switch (newSim) {
    case SIM_GRAVITY: GravityField::init(); break;
    case SIM_FIXED_ELECTRIC: FixedElectric::init(); break;
    case SIM_RADIATION: RadiationField::init(); break;
    case SIM_SPIN_SIMULATION: SpinSimulation::init(); break;
    case SIM_WIRE: OhmsLawWire::init(); break;
    default: break;
    }

    currentSim = newSim;
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Physics Simulations", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        glfwTerminate();
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetScrollCallback(window, scrollCallback);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    if (!initSharedTextRendering()) {
        std::cerr << "Failed to initialize text rendering" << std::endl;
        glfwTerminate();
        return -1;
    }

    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Simulation Selector");
        if (ImGui::Button("Gravity Field")) switchSimulation(SIM_GRAVITY);
        if (ImGui::Button("Fixed Electric Field")) switchSimulation(SIM_FIXED_ELECTRIC);
        if (ImGui::Button("Radiation Field")) switchSimulation(SIM_RADIATION);
        if (ImGui::Button("Spin Simulation")) switchSimulation(SIM_SPIN_SIMULATION);
        if (ImGui::Button("Ohm's Law Wire")) switchSimulation(SIM_WIRE);
        ImGui::Checkbox("Auto Rotate", &autoRotate);
        ImGui::End();

        glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = updateCamera();
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);

        switch (currentSim) {
        case SIM_GRAVITY:
            ImGui::Begin("Gravity Controls");
            ImGui::Checkbox("Show Field Lines", &GravityField::showFieldLines);
            ImGui::Checkbox("Show Test Particles", &GravityField::showTestParticles);
            ImGui::End();
            GravityField::update(deltaTime);
            GravityField::render(view, projection);
            break;
        case SIM_FIXED_ELECTRIC:
            ImGui::Begin("Electric Controls");
            ImGui::Checkbox("Show Field Lines", &FixedElectric::showFieldLines);
            ImGui::Checkbox("Show Test Charges", &FixedElectric::showTestCharges);
            ImGui::End();
            FixedElectric::update(deltaTime);
            FixedElectric::render(view, projection);
            break;
        case SIM_RADIATION:
            ImGui::Begin("Radiation Controls");
            ImGui::Checkbox("Show Wave", &RadiationField::showWave);
            ImGui::End();
            RadiationField::update(deltaTime);
            RadiationField::render(view, projection);
            break;
        case SIM_SPIN_SIMULATION:
            ImGui::Begin("Spin Controls");
            ImGui::Checkbox("Show Particle", &SpinSimulation::showParticle);
            ImGui::End();
            SpinSimulation::update(deltaTime);
            SpinSimulation::render(view, projection);
            break;
        case SIM_WIRE:
            ImGui::Begin("Wire Controls");
            ImGui::Checkbox("Show Current", &OhmsLawWire::showCurrent);
            ImGui::End();
            OhmsLawWire::update(deltaTime);
            OhmsLawWire::render(view, projection);
            break;
        default:
            sharedRenderText(GravityField::textShaderProgram, "Select a Simulation", 10.0f, HEIGHT - 40.0f, 0.5f, glm::vec3(1.0f));
            break;
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    switchSimulation(SIM_NONE);

    glDeleteTextures(1, &sharedFontTexture);
    glDeleteVertexArrays(1, &sharedTextVAO);
    glDeleteBuffers(1, &sharedTextVBO);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}