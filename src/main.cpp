#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "Camera.h"
#include <glm/gtc/type_ptr.hpp>
#include "Terrain.h"
#include "Sphere.h"

int SCR_WIDTH = 1280;
int SCR_HEIGHT = 720;

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = 640.0f, lastY = 360.0f;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Globální nastavení mraků a oblohy
float cloudCoverage  = 0.55f;
float cloudSpeed     = 1.0f;
float cloudScale     = 2.0f;
float cloudBase      = 50.0f;
float cloudTop       = 350.0f;

glm::vec3 sunColor       = glm::vec3(0.92f, 0.42f, 0.26f);
glm::vec3 cloudSunColor  = glm::vec3(0.95f, 0.50f, 0.30f);
glm::vec3 cloudBaseColor = glm::vec3(0.38f, 0.20f, 0.24f);
float sunAngle = 0.0f;

unsigned int hdrFBO, colorBuffers[2], rboDepth;
unsigned int pingpongFBO[2], pingpongColorbuffers[2];

void initFramebuffers(int width, int height) {
    static bool initialized = false;
    if (initialized) {
        glDeleteFramebuffers(1, &hdrFBO);
        glDeleteTextures(2, colorBuffers);
        glDeleteRenderbuffers(1, &rboDepth);
        glDeleteFramebuffers(2, pingpongFBO);
        glDeleteTextures(2, pingpongColorbuffers);
    }
    initialized = true;

    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; i++) {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
    }

    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);
    for (unsigned int i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    if (width == 0 || height == 0) return;
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
    glViewport(0, 0, width, height);
    initFramebuffers(width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }
    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;
    lastX = (float)xpos;
    lastY = (float)ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    camera.yaw += xoffset;
    camera.pitch += yoffset;

    if (camera.pitch > 89.0f) camera.pitch = 89.0f;
    if (camera.pitch < -89.0f) camera.pitch = -89.0f;
}

// ================= SHADERY =================

const char* vertexShaderSource = R"(
#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
uniform mat4 view;
uniform mat4 projection;
out vec3 Normal;
out float Height;
void main() {
    Normal = aNormal;
    Height = aPos.y;
    gl_Position = projection * view * vec4(aPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 460 core
in vec3 Normal;
in float Height;
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

void main() {
    vec3 n = normalize(Normal);
    vec3 sandColor  = vec3(0.76, 0.70, 0.50);
    vec3 grassColor = vec3(0.30, 0.55, 0.20);
    vec3 rockColor  = vec3(0.45, 0.42, 0.38);
    vec3 snowColor  = vec3(0.95, 0.95, 0.97);

    float slope = n.y;
    vec3 colorByHeight = mix(sandColor, grassColor, smoothstep(0.0, 4.0, Height));
    colorByHeight = mix(colorByHeight, rockColor, smoothstep(25.0, 35.0, Height));
    colorByHeight = mix(colorByHeight, snowColor, smoothstep(42.0, 48.0, Height));

    float rockBySlope = smoothstep(0.9, 0.6, slope);
    vec3 finalColor = mix(colorByHeight, rockColor, rockBySlope);

    vec3 lightDir = normalize(vec3(0.0, 0.5, -1.0));
    float diff = max(dot(n, lightDir), 0.0);

    vec3 ambient = finalColor * 0.3;
    vec3 result = ambient + finalColor * diff * vec3(1.0, 0.6, 0.4);

    FragColor = vec4(result, 1.0);
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
)";

const char* waterVertexShaderSource = R"(
#version 460 core
layout (location = 0) in vec3 aPos;
uniform mat4 view;
uniform mat4 projection;
uniform float time;
out float waveHeight;
void main() {
    vec3 pos = aPos;
    waveHeight = sin(pos.x * 0.1 + time) * 0.2 + cos(pos.z * 0.1 + time * 0.8) * 0.2;
    pos.y += waveHeight;
    gl_Position = projection * view * vec4(pos, 1.0);
}
)";

const char* waterFragmentShaderSource = R"(
#version 460 core
in float waveHeight;
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;
void main() {
    vec3 shallowColor = vec3(0.2, 0.4, 0.5);
    vec3 deepColor = vec3(0.05, 0.15, 0.3);
    vec3 color = mix(deepColor, shallowColor, waveHeight + 0.5);
    FragColor = vec4(color, 0.8);
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
)";

const char* skyVertexShaderSource = R"(
#version 460 core
layout (location = 0) in vec3 aPos;
uniform mat4 view;
uniform mat4 projection;
out vec3 Direction;
void main() {
    Direction = aPos;
    mat4 rotOnlyView = mat4(mat3(view));
    vec4 pos = projection * rotOnlyView * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}
)";

const char* skyFragmentShaderSource = R"(
#version 460 core
in vec3 Direction;
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

uniform vec3 sunDirection;
uniform float time;
uniform vec3 camPos;
uniform vec3 skyHorizonColor;
uniform vec3 skyZenithColor;

uniform float cloudCoverage;
uniform float cloudSpeed;
uniform float cloudScale;
uniform vec3 sunColor;
uniform vec3 cloudSunColor;
uniform vec3 cloudBaseColor;
uniform float cloudBase;
uniform float cloudTop;
uniform float sunExponent;

float hash3(vec3 p) {
    p = fract(p * vec3(0.1031, 0.1030, 0.0973));
    p += dot(p, p.yxz + 33.33);
    return fract((p.x + p.y) * p.z);
}

float random2D(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123);
}

float noise3(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(mix(hash3(i + vec3(0,0,0)), hash3(i + vec3(1,0,0)), f.x),
            mix(hash3(i + vec3(0,1,0)), hash3(i + vec3(1,1,0)), f.x), f.y),
        mix(mix(hash3(i + vec3(0,0,1)), hash3(i + vec3(1,0,1)), f.x),
            mix(hash3(i + vec3(0,1,1)), hash3(i + vec3(1,1,1)), f.x), f.y), f.z);
}

float worley3D(vec3 p) {
    vec3 id = floor(p);
    vec3 fd = fract(p);
    float minDist = 1.0;
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            for (int z = -1; z <= 1; z++) {
                vec3 offset = vec3(x, y, z);
                vec3 cellPoint = vec3(
                    hash3(id + offset),
                    hash3(id + offset + vec3(11.0, 37.0, 71.0)),
                    hash3(id + offset + vec3(53.0, 13.0, 97.0))
                );
                vec3 diff = offset + cellPoint - fd;
                minDist = min(minDist, length(diff));
            }
        }
    }
    return 1.0 - minDist;
}

float fbm3(vec3 p) {
    float val = 0.0;
    float amp = 0.5;
    for (int i = 0; i < 4; i++) {
        val += amp * noise3(p);
        p *= 2.0;
        amp *= 0.5;
    }
    return val;
}

float cloudDensity(vec3 pos) {
    vec3 wind = vec3(time * cloudSpeed * 12.0, 0.0, time * cloudSpeed * 5.0);
    vec3 samplePos = (pos + wind) * 0.001 * cloudScale;
    float baseNoise = fbm3(samplePos);
    
    float heightFraction = (pos.y - cloudBase) / (cloudTop - cloudBase);
    float heightFade = smoothstep(0.0, 0.15, heightFraction) * smoothstep(1.0, 0.7, heightFraction);

    float threshold = mix(0.75, 0.35, clamp(cloudCoverage, 0.0, 1.0));
    float baseShape = smoothstep(threshold, threshold + 0.25, baseNoise) * heightFade;

    if (baseShape <= 0.001) return 0.0;
    float detailWorley = worley3D((pos + wind) * 0.006 * cloudScale);
    return clamp(smoothstep(0.1, 0.9, baseShape - (1.0 - detailWorley) * 0.3), 0.0, 1.0);
}

float hgPhase(float cosAngle, float g) {
    float g2 = g * g;
    return (1.0 - g2) / (4.0 * 3.14159265 * pow(1.0 + g2 - 2.0 * g * cosAngle, 1.5));
}

void main() {
    vec3 dir = normalize(Direction);
    vec3 horizonColor = skyHorizonColor;
    vec3 zenithColor  = skyZenithColor;
    vec3 skyColor = mix(horizonColor, zenithColor, clamp(dir.y * 0.5 + 0.5, 0.0, 1.0));

    vec3 sunDir = normalize(sunDirection);
    float sunCos = max(dot(dir, sunDir), 0.0);
    vec3 sunSpot = sunColor * pow(sunCos, 256.0) * 1.5;
    skyColor += sunSpot;

    const int steps = 48;
    float horizonFade = smoothstep(-0.05, 0.12, dir.y);

    if (horizonFade > 0.001) {
        float safeDirY = max(dir.y, 0.02);
        float t0 = max((cloudBase - camPos.y) / safeDirY, 0.0);
        float t1 = min((cloudTop - camPos.y) / safeDirY, t0 + 3000.0);

        if (t1 > t0) {
            float stepSize = (t1 - t0) / float(steps);
            float jitter = random2D(gl_FragCoord.xy);
            vec3 samplePos = camPos + dir * (t0 + stepSize * jitter);

            float transmittance = 1.0;
            vec3 lightEnergy = vec3(0.0);
            float phase = hgPhase(dot(dir, sunDir), 0.65);

            for (int i = 0; i < steps; i++) {
                float density = cloudDensity(samplePos);
                if (density > 0.005) {
                    vec3 lightSamplePos = samplePos + sunDir * 18.0;
                    float lightDensity = cloudDensity(lightSamplePos);
                    
                    float lightIntensity = exp(-lightDensity * 3.0) * (1.0 - exp(-density * 2.5));
                    vec3 currentLight = mix(cloudBaseColor, cloudSunColor * (1.0 + phase * 2.0), lightIntensity);

                    float stepVis = exp(-density * stepSize * 0.03);
                    lightEnergy += currentLight * (1.0 - stepVis) * transmittance;
                    transmittance *= stepVis;

                    if (transmittance < 0.01) break;
                }
                samplePos += dir * stepSize;
            }
            skyColor = mix(skyColor, skyColor * transmittance + lightEnergy, horizonFade);
        }
    }

    FragColor = vec4(skyColor, 1.0);

    // Zdroj jasného světla pro God Rays
    if (sunCos > 0.985) {
        BrightColor = vec4(sunColor * pow(sunCos, 512.0) * 5.0, 1.0);
    } else {
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
)";

const char* sphereVertexShaderSource = R"(
#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec3 Normal;
void main() {
    Normal = aNormal;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* sphereFragmentShaderSource = R"(
#version 460 core
in vec3 Normal;
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;
void main() {
    vec3 lightDir = normalize(vec3(0.0, 0.5, -1.0));
    float diff = max(dot(normalize(Normal), lightDir), 0.0);
    vec3 baseColor = vec3(0.9, 0.3, 0.2);
    FragColor = vec4(baseColor * 0.3 + baseColor * diff, 1.0);
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
)";

const char* quadVertexShaderSource = R"(
#version 460 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;
out vec2 TexCoords;
void main() {
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

const char* blurFragmentShaderSource = R"(
#version 460 core
in vec2 TexCoords;
out vec4 FragColor;
uniform sampler2D image;
uniform bool horizontal;
uniform float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {             
    vec2 tex_offset = 1.0 / textureSize(image, 0); 
    vec3 result = texture(image, TexCoords).rgb * weight[0];
    if(horizontal) {
        for(int i = 1; i < 5; ++i) {
            result += texture(image, TexCoords + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
            result += texture(image, TexCoords - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
        }
    } else {
        for(int i = 1; i < 5; ++i) {
            result += texture(image, TexCoords + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
            result += texture(image, TexCoords - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
        }
    }
    FragColor = vec4(result, 1.0);
}
)";

// COMPOSITING SHADER (GOD RAYS + LENS FLARES + BLOOM + VIGNETTE)
const char* finalHdrFragmentShaderSource = R"(
#version 460 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D hdrBuffer;

void main() {
    FragColor = vec4(texture(hdrBuffer, TexCoords).rgb, 1.0);
}
)";

unsigned int createProgram(const char* vShader, const char* fShader) {
    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vShader, nullptr);
    glCompileShader(vs);

    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fShader, nullptr);
    glCompileShader(fs);

    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Zenith Engine - God Rays & Lens Flares Fixed", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    unsigned int shaderProgram = createProgram(vertexShaderSource, fragmentShaderSource);
    unsigned int waterShaderProgram = createProgram(waterVertexShaderSource, waterFragmentShaderSource);
    unsigned int skyShaderProgram = createProgram(skyVertexShaderSource, skyFragmentShaderSource);
    unsigned int sphereShaderProgram = createProgram(sphereVertexShaderSource, sphereFragmentShaderSource);
    unsigned int blurShaderProgram = createProgram(quadVertexShaderSource, blurFragmentShaderSource);
    unsigned int finalHdrShaderProgram = createProgram(quadVertexShaderSource, finalHdrFragmentShaderSource);

    initFramebuffers(SCR_WIDTH, SCR_HEIGHT);

    Terrain terrain;
    terrain.GenerateFromHeightmap("assets/heightmap/heightmap.png", 50.0f, 1.0f);

    float playerRadius = 1.5f;
    Sphere playerSphere;
    playerSphere.Generate(playerRadius, 20);

    glm::vec3 playerPosition = glm::vec3(terrain.width / 2.0f, 0.0f, terrain.height / 2.0f);
    playerPosition.y = terrain.GetHeightAt(playerPosition.x, playerPosition.z) + playerRadius;

    // Namíření kamery přímo na slunce při startu
    camera.yaw = -90.0f;
    camera.pitch = 20.0f;

    float margin = 1000.0f, waterLevel = 0.5f;
    float waterVertices[] = {
        -margin, waterLevel, -margin,
        (float)terrain.width + margin, waterLevel, -margin,
        (float)terrain.width + margin, waterLevel, (float)terrain.height + margin,
        -margin, waterLevel, -margin,
        (float)terrain.width + margin, waterLevel, (float)terrain.height + margin,
        -margin, waterLevel, (float)terrain.height + margin,
    };

    unsigned int waterVAO, waterVBO;
    glGenVertexArrays(1, &waterVAO);
    glGenBuffers(1, &waterVBO);
    glBindVertexArray(waterVAO);
    glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(waterVertices), waterVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    float skyboxVertices[] = {
        -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f
    };
    unsigned int skyVAO, skyVBO;
    glGenVertexArrays(1, &skyVAO);
    glGenBuffers(1, &skyVBO);
    glBindVertexArray(skyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    float quadVertices[] = {
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    float cameraDistance = 8.0f, cameraHeight = 3.0f, playerSpeed = 60.0f;
    // Směr slunce vyšší na obloze
    glm::vec3 sunDir = glm::normalize(glm::vec3(0.0f, 0.5f, -1.0f));

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        glm::vec3 forward = glm::normalize(glm::vec3(camera.GetFront().x, 0.0f, camera.GetFront().z));
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        float moveDistance = playerSpeed * deltaTime;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) playerPosition += forward * moveDistance;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) playerPosition -= forward * moveDistance;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) playerPosition -= right * moveDistance;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) playerPosition += right * moveDistance;

        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) sunAngle -= 0.9f * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) sunAngle += 0.9f * deltaTime;

        playerPosition.y = terrain.GetHeightAt(playerPosition.x, playerPosition.z) + playerRadius;
        camera.position = playerPosition - camera.GetFront() * cameraDistance + glm::vec3(0.0f, cameraHeight, 0.0f);

        float baseSunHeight = 0.65f * glm::sin(sunAngle);
        float sunHeight = glm::clamp(baseSunHeight, -0.35f, 0.95f);
        sunDir = glm::normalize(glm::vec3(glm::cos(sunAngle), sunHeight, glm::sin(sunAngle)));

        float daylight = glm::clamp((sunDir.y + 0.15f) / 0.95f, 0.0f, 1.0f);
        glm::vec3 dawnHorizon = glm::vec3(0.90f, 0.52f, 0.34f);
        glm::vec3 dayHorizon = glm::vec3(0.66f, 0.78f, 0.95f);
        glm::vec3 duskHorizon = glm::vec3(0.96f, 0.43f, 0.28f);
        glm::vec3 skyHorizonColor = glm::mix(duskHorizon, dayHorizon, daylight);
        skyHorizonColor = glm::mix(skyHorizonColor, dawnHorizon, glm::smoothstep(0.15f, -0.15f, sunDir.y));

        glm::vec3 dawnZenith = glm::vec3(0.48f, 0.28f, 0.35f);
        glm::vec3 dayZenith = glm::vec3(0.18f, 0.34f, 0.72f);
        glm::vec3 duskZenith = glm::vec3(0.52f, 0.30f, 0.38f);
        glm::vec3 skyZenithColor = glm::mix(duskZenith, dayZenith, daylight);
        skyZenithColor = glm::mix(skyZenithColor, dawnZenith, glm::smoothstep(0.15f, -0.15f, sunDir.y));

        sunColor = glm::mix(glm::vec3(1.0f, 0.82f, 0.62f), glm::vec3(0.96f, 0.42f, 0.18f), 1.0f - daylight);
        cloudSunColor = glm::mix(glm::vec3(1.0f, 0.85f, 0.68f), glm::vec3(0.96f, 0.52f, 0.24f), 1.0f - daylight);
        cloudBaseColor = glm::mix(glm::vec3(0.34f, 0.32f, 0.42f), glm::vec3(0.40f, 0.18f, 0.20f), 1.0f - daylight);

        // 1. Render Scény
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 500.0f);

        glUseProgram(skyShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(skyShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(skyShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform1f(glGetUniformLocation(skyShaderProgram, "time"), currentFrame);
        glUniform3f(glGetUniformLocation(skyShaderProgram, "camPos"), camera.position.x, camera.position.y, camera.position.z);
        
        glUniform3f(glGetUniformLocation(skyShaderProgram, "sunDirection"), sunDir.x, sunDir.y, sunDir.z);
        glUniform3f(glGetUniformLocation(skyShaderProgram, "skyHorizonColor"), skyHorizonColor.r, skyHorizonColor.g, skyHorizonColor.b);
        glUniform3f(glGetUniformLocation(skyShaderProgram, "skyZenithColor"), skyZenithColor.r, skyZenithColor.g, skyZenithColor.b);
        glUniform1f(glGetUniformLocation(skyShaderProgram, "sunExponent"), 4050.0f);
        glUniform1f(glGetUniformLocation(skyShaderProgram, "cloudCoverage"), cloudCoverage);
        glUniform1f(glGetUniformLocation(skyShaderProgram, "cloudSpeed"), cloudSpeed);
        glUniform1f(glGetUniformLocation(skyShaderProgram, "cloudScale"), cloudScale);
        glUniform1f(glGetUniformLocation(skyShaderProgram, "cloudBase"), cloudBase);
        glUniform1f(glGetUniformLocation(skyShaderProgram, "cloudTop"), cloudTop);
        
        glUniform3f(glGetUniformLocation(skyShaderProgram, "sunColor"), sunColor.r, sunColor.g, sunColor.b);
        glUniform3f(glGetUniformLocation(skyShaderProgram, "cloudSunColor"), cloudSunColor.r, cloudSunColor.g, cloudSunColor.b);
        glUniform3f(glGetUniformLocation(skyShaderProgram, "cloudBaseColor"), cloudBaseColor.r, cloudBaseColor.g, cloudBaseColor.b);

        glBindVertexArray(skyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        terrain.Draw();

        glUseProgram(sphereShaderProgram);
        glm::mat4 sphereModel = glm::translate(glm::mat4(1.0f), playerPosition);
        glUniformMatrix4fv(glGetUniformLocation(sphereShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(sphereModel));
        glUniformMatrix4fv(glGetUniformLocation(sphereShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(sphereShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        playerSphere.Draw();

        glUseProgram(waterShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(waterShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(waterShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform1f(glGetUniformLocation(waterShaderProgram, "time"), currentFrame);
        glBindVertexArray(waterVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 2. Bloom Blur Pass disabled: amount = 0 keeps the scene sharp.
        bool horizontal = true, first_iteration = true;
        unsigned int amount = 0;
        glUseProgram(blurShaderProgram);

        for (unsigned int i = 0; i < amount; i++) {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            glUniform1i(glGetUniformLocation(blurShaderProgram, "horizontal"), horizontal);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);

            glBindVertexArray(quadVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            horizontal = !horizontal;
            if (first_iteration) first_iteration = false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 3. Výpočet pozice slunce na obrazovce
        glm::mat4 rotOnlyView = glm::mat4(glm::mat3(camera.GetViewMatrix()));
        glm::vec4 viewSun = rotOnlyView * glm::vec4(sunDir, 0.0f);
        bool sunInFront = viewSun.z < 0.0f;
        glm::vec2 sunScreenPos(0.5f);

        if (sunInFront) {
            glm::vec4 clipSun = projection * viewSun;
            glm::vec3 ndcSun = glm::vec3(clipSun) / clipSun.w;
            sunScreenPos = glm::vec2(ndcSun.x * 0.5f + 0.5f, ndcSun.y * 0.5f + 0.5f);
        }

        // 4. Finální Post-processing
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(finalHdrShaderProgram);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
        glUniform1i(glGetUniformLocation(finalHdrShaderProgram, "hdrBuffer"), 0);

        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
