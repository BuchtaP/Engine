#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "Camera.h"
#include <glm/gtc/type_ptr.hpp>
#include "Terrain.h"
#include "Sphere.h"

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));

float lastX = 640, lastY = 360;
bool firstMouse = true;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    camera.yaw += xoffset;
    camera.pitch += yoffset;

    if (camera.pitch > 89.0f) camera.pitch = 89.0f;
    if (camera.pitch < -89.0f) camera.pitch = -89.0f;
}

const char* vertexShaderSource = R"(
#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 view;
uniform mat4 projection;

out vec3 Normal;
out float Height;

void main()
{
    Normal = aNormal;
    Height = aPos.y;
    gl_Position = projection * view * vec4(aPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 460 core
in vec3 Normal;
in float Height;
out vec4 FragColor;

void main()
{
    vec3 n = normalize(Normal);

    vec3 sandColor  = vec3(0.76, 0.70, 0.50);
    vec3 grassColor = vec3(0.30, 0.55, 0.20);
    vec3 rockColor  = vec3(0.45, 0.42, 0.38);
    vec3 snowColor  = vec3(0.95, 0.95, 0.97);

    float slope = n.y;

    vec3 colorByHeight;
    float t1 = smoothstep(0.0, 4.0, Height);
    colorByHeight = mix(sandColor, grassColor, t1);

    float t2 = smoothstep(25.0, 35.0, Height);
    colorByHeight = mix(colorByHeight, rockColor, t2);

    float t3 = smoothstep(42.0, 48.0, Height);
    colorByHeight = mix(colorByHeight, snowColor, t3);

    float rockBySlope = smoothstep(0.9, 0.6, slope);
    vec3 finalColor = mix(colorByHeight, rockColor, rockBySlope);

    vec3 lightDir = normalize(vec3(0.8, 0.15, 0.3)); // Nízké západní slunce
    float diff = max(dot(n, lightDir), 0.0);

    vec3 ambient = finalColor * 0.3;
    vec3 result = ambient + finalColor * diff * vec3(1.0, 0.6, 0.4); // Teplý západ slunce na terénu

    FragColor = vec4(result, 1.0);
}
)";

const char* waterVertexShaderSource = R"(
#version 460 core
layout (location = 0) in vec3 aPos;

uniform mat4 view;
uniform mat4 projection;
uniform float time;

out float waveHeight;

void main()
{
    vec3 pos = aPos;
    waveHeight = sin(pos.x * 0.1 + time) * 0.2 + cos(pos.z * 0.1 + time * 0.8) * 0.2;
    pos.y += waveHeight;

    gl_Position = projection * view * vec4(pos, 1.0);
}
)";

const char* waterFragmentShaderSource = R"(
#version 460 core
in float waveHeight;
out vec4 FragColor;

void main()
{
    vec3 shallowColor = vec3(0.2, 0.4, 0.5);
    vec3 deepColor = vec3(0.05, 0.15, 0.3);

    vec3 color = mix(deepColor, shallowColor, waveHeight + 0.5);
    FragColor = vec4(color, 0.8);
}
)";

const char* skyVertexShaderSource = R"(
#version 460 core
layout (location = 0) in vec3 aPos;

uniform mat4 view;
uniform mat4 projection;

out vec3 Direction;

void main()
{
    Direction = aPos;
    mat4 rotOnlyView = mat4(mat3(view));
    vec4 pos = projection * rotOnlyView * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}
)";

// SKY & CLOUDS SHADER S NASTAVITELNÝMI UNIFORMY A KALIFORNSKÝM ZÁPADEM SLUNCE
const char* skyFragmentShaderSource = R"(
#version 460 core
in vec3 Direction;
out vec4 FragColor;

uniform vec3 sunDirection;
uniform float time;
uniform vec3 camPos;

// --- NOVÁ NASTAVENÍ MRAKŮ A BARVY ---
uniform float cloudCoverage;  // Pokrytí mraků (0.1 = málo, 0.5 = středně, 0.8 = hodně)
uniform float cloudSpeed;     // Rychlost posuvu
uniform float cloudScale;     // Měřítko šumu
uniform vec3 sunColor;        // Barva slunce
uniform vec3 cloudSunColor;   // Barva nasvícené části mraku
uniform vec3 cloudBaseColor;  // Barva stínu mraku

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
                float dist = length(diff);
                minDist = min(minDist, dist);
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
    
    // Výškový profil vrstvy mraků (150 m až 380 m)
    float heightFraction = (pos.y - 120.0) / 150.0;
    float heightFade = smoothstep(0.0, 0.15, heightFraction) * smoothstep(1.0, 0.7, heightFraction);

    // cloudCoverage dynamicky mění práh pro generování mraku
    float threshold = mix(0.75, 0.35, clamp(cloudCoverage, 0.0, 1.0));
    float baseShape = smoothstep(threshold, threshold + 0.25, baseNoise) * heightFade;

    if (baseShape <= 0.001) return 0.0;

    float detailWorley = worley3D((pos + wind) * 0.006 * cloudScale);
    float finalDensity = smoothstep(0.1, 0.9, baseShape - (1.0 - detailWorley) * 0.3);

    return clamp(finalDensity, 0.0, 1.0);
}

float hgPhase(float cosAngle, float g) {
    float g2 = g * g;
    return (1.0 - g2) / (4.0 * 3.14159265 * pow(1.0 + g2 - 2.0 * g * cosAngle, 1.5));
}

void main()
{
    vec3 dir = normalize(Direction);

    // Kalifornský večerní přechod oblohy (oranžovo-růžový horizont, hluboká modř nahoře)
    vec3 horizonColor = vec3(0.95, 0.5, 0.3);
    vec3 zenithColor  = vec3(0.1, 0.2, 0.5);
    float t = clamp(dir.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 skyColor = mix(horizonColor, zenithColor, t);

    // Slunce
    vec3 sunDir = normalize(sunDirection);
    float sunCos = max(dot(dir, sunDir), 0.0);
    skyColor += sunColor * pow(sunCos, 512.0) * 2.0;

    // Volumetrické mraky
    const float cloudBase = 150.0;
    const float cloudTop  = 380.0;
    const int steps = 48;

    // Odstraněn ostrý ořez u horizontu – mraky nezmizí ani při pohledu z malé výšky
    float horizonFade = smoothstep(-0.05, 0.12, dir.y);

    if (horizonFade > 0.001)
    {
        // Bezpečné ošetření dir.y, aby dělení 0 nezpůsobovalo mizení mraků z nízké výšky
        float safeDirY = max(dir.y, 0.02);
        
        float t0 = max((cloudBase - camPos.y) / safeDirY, 0.0);
        float t1 = (cloudTop - camPos.y) / safeDirY;
        t1 = min(t1, t0 + 3000.0);

        if (t1 > t0)
        {
            float stepSize = (t1 - t0) / float(steps);

            // Dithering pro odstranění proužků
            float jitter = random2D(gl_FragCoord.xy);
            vec3 samplePos = camPos + dir * (t0 + stepSize * jitter);

            float transmittance = 1.0;
            vec3 lightEnergy = vec3(0.0);

            float phase = hgPhase(dot(dir, sunDir), 0.65);

            for (int i = 0; i < steps; i++)
            {
                float density = cloudDensity(samplePos);

                if (density > 0.005)
                {
                    // Vzorkování světla směrem ke Slunci
                    vec3 lightSamplePos = samplePos + sunDir * 18.0;
                    float lightDensity = cloudDensity(lightSamplePos);
                    
                    float lightAbsorption = exp(-lightDensity * 3.0);
                    float powderEffect = 1.0 - exp(-density * 2.5);
                    float lightIntensity = lightAbsorption * powderEffect;

                    // Mísení barvy stínu a nasvícené barvy podle nastavitelných uniformů
                    vec3 currentLight = mix(cloudBaseColor, cloudSunColor * (1.0 + phase * 2.0), lightIntensity);

                    float stepVis = exp(-density * stepSize * 0.03);
                    lightEnergy += currentLight * (1.0 - stepVis) * transmittance;
                    transmittance *= stepVis;

                    if (transmittance < 0.01) break;
                }

                samplePos += dir * stepSize;
            }

            vec3 cloudsResult = skyColor * transmittance + lightEnergy;
            skyColor = mix(skyColor, cloudsResult, horizonFade);
        }
    }

    FragColor = vec4(skyColor, 1.0);
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

void main()
{
    Normal = aNormal;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* sphereFragmentShaderSource = R"(
#version 460 core
in vec3 Normal;
out vec4 FragColor;

void main()
{
    vec3 lightDir = normalize(vec3(0.8, 0.15, 0.3));
    float diff = max(dot(normalize(Normal), lightDir), 0.0);

    vec3 baseColor = vec3(0.9, 0.3, 0.2);
    vec3 ambient = baseColor * 0.3;
    vec3 result = ambient + baseColor * diff;

    FragColor = vec4(result, 1.0);
}
)";

float skyboxVertices[] = {
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};

int main()
{
    if (!glfwInit())
    {
        std::cerr << "Nepodarilo se inicializovat GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Zenith - California Clouds", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Nepodarilo se vytvorit okno\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Nepodarilo se nacist GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Shader příprava
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    unsigned int waterVertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(waterVertexShader, 1, &waterVertexShaderSource, nullptr);
    glCompileShader(waterVertexShader);

    unsigned int waterFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(waterFragmentShader, 1, &waterFragmentShaderSource, nullptr);
    glCompileShader(waterFragmentShader);

    unsigned int waterShaderProgram = glCreateProgram();
    glAttachShader(waterShaderProgram, waterVertexShader);
    glAttachShader(waterShaderProgram, waterFragmentShader);
    glLinkProgram(waterShaderProgram);
    glDeleteShader(waterVertexShader);
    glDeleteShader(waterFragmentShader);

    unsigned int skyVertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(skyVertexShader, 1, &skyVertexShaderSource, nullptr);
    glCompileShader(skyVertexShader);

    unsigned int skyFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(skyFragmentShader, 1, &skyFragmentShaderSource, nullptr);
    glCompileShader(skyFragmentShader);

    unsigned int skyShaderProgram = glCreateProgram();
    glAttachShader(skyShaderProgram, skyVertexShader);
    glAttachShader(skyShaderProgram, skyFragmentShader);
    glLinkProgram(skyShaderProgram);
    glDeleteShader(skyVertexShader);
    glDeleteShader(skyFragmentShader);

    unsigned int sphereVertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(sphereVertexShader, 1, &sphereVertexShaderSource, nullptr);
    glCompileShader(sphereVertexShader);

    unsigned int sphereFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(sphereFragmentShader, 1, &sphereFragmentShaderSource, nullptr);
    glCompileShader(sphereFragmentShader);

    unsigned int sphereShaderProgram = glCreateProgram();
    glAttachShader(sphereShaderProgram, sphereVertexShader);
    glAttachShader(sphereShaderProgram, sphereFragmentShader);
    glLinkProgram(sphereShaderProgram);
    glDeleteShader(sphereVertexShader);
    glDeleteShader(sphereFragmentShader);

    Terrain terrain;
    terrain.GenerateFromHeightmap("assets/heightmap/heightmap.png", 50.0f, 1.0f);

    float playerRadius = 1.5f;
    Sphere playerSphere;
    playerSphere.Generate(playerRadius, 20);

    glm::vec3 playerPosition = glm::vec3(terrain.width / 2.0f, 0.0f, terrain.height / 2.0f);
    playerPosition.y = terrain.GetHeightAt(playerPosition.x, playerPosition.z) + playerRadius;

    camera.yaw = 90.0f;
    camera.pitch = -20.0f;

    float margin = 0.0f;
    float waterLevel = 0.5f;
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

    unsigned int skyVAO, skyVBO;
    glGenVertexArrays(1, &skyVAO);
    glGenBuffers(1, &skyVBO);
    glBindVertexArray(skyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    float cameraDistance = 8.0f;
    float cameraHeight = 3.0f;
    float playerSpeed = 6.0f;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        glm::vec3 forward = camera.GetFront();
        forward.y = 0.0f;
        forward = glm::normalize(forward);
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));

        float moveDistance = playerSpeed * deltaTime;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            playerPosition += forward * moveDistance;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            playerPosition -= forward * moveDistance;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            playerPosition -= right * moveDistance;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            playerPosition += right * moveDistance;

        playerPosition.y = terrain.GetHeightAt(playerPosition.x, playerPosition.z) + playerRadius;
        camera.position = playerPosition - camera.GetFront() * cameraDistance + glm::vec3(0.0f, cameraHeight, 0.0f);

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 500.0f);

        // --- VYKRESLENÍ OBLOHY A MRAKŮ ---
        glUseProgram(skyShaderProgram);

        // Základní uniformy
        glUniformMatrix4fv(glGetUniformLocation(skyShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(skyShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform1f(glGetUniformLocation(skyShaderProgram, "time"), (float)glfwGetTime());
        glUniform3f(glGetUniformLocation(skyShaderProgram, "camPos"), camera.position.x, camera.position.y, camera.position.z);

        // --- NASTAVENÍ KALIFORNSKÉHO ZÁPADU SLUNCE & MRAKŮ ---
        // Směr slunce (nízko nad horizontem)
        glUniform3f(glGetUniformLocation(skyShaderProgram, "sunDirection"), 0.8f, 0.15f, 0.3f);
        
        // Parametry mraků
        glUniform1f(glGetUniformLocation(skyShaderProgram, "cloudCoverage"), 0.6f); // 0.1 = jasno, 0.8 = zataženo
        glUniform1f(glGetUniformLocation(skyShaderProgram, "cloudSpeed"), 1.0f);    // Rychlost posuvu mraků
        glUniform1f(glGetUniformLocation(skyShaderProgram, "cloudScale"), 0.8f);    // Velikost mraků

        // Barvy (Kalifornský Sunset)
        glUniform3f(glGetUniformLocation(skyShaderProgram, "sunColor"), 1.0f, 0.35f, 0.05f);        // Červeno-oranžové slunce
        glUniform3f(glGetUniformLocation(skyShaderProgram, "cloudSunColor"), 1.0f, 0.45f, 0.25f);   // Zářící červeno-zlaté okraje
        glUniform3f(glGetUniformLocation(skyShaderProgram, "cloudBaseColor"), 0.22f, 0.18f, 0.32f); // Fialovo-modrý stín spodku mraku

        glBindVertexArray(skyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Render terénu
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        terrain.Draw();

        // Render hráče
        glUseProgram(sphereShaderProgram);
        glm::mat4 sphereModel = glm::translate(glm::mat4(1.0f), playerPosition);
        glUniformMatrix4fv(glGetUniformLocation(sphereShaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(sphereModel));
        glUniformMatrix4fv(glGetUniformLocation(sphereShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(sphereShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        playerSphere.Draw();

        // Render vody
        glUseProgram(waterShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(waterShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(waterShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform1f(glGetUniformLocation(waterShaderProgram, "time"), (float)glfwGetTime());
        glBindVertexArray(waterVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &terrain.VAO);
    glDeleteBuffers(1, &terrain.VBO);
    glDeleteBuffers(1, &terrain.EBO);
    glDeleteProgram(shaderProgram);

    glDeleteVertexArrays(1, &waterVAO);
    glDeleteBuffers(1, &waterVBO);
    glDeleteProgram(waterShaderProgram);

    glDeleteVertexArrays(1, &skyVAO);
    glDeleteBuffers(1, &skyVBO);
    glDeleteProgram(skyShaderProgram);

    glDeleteVertexArrays(1, &playerSphere.VAO);
    glDeleteBuffers(1, &playerSphere.VBO);
    glDeleteBuffers(1, &playerSphere.EBO);
    glDeleteProgram(sphereShaderProgram);

    glfwTerminate();
    return 0;
}