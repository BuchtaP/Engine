#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "Camera.h"
#include <glm/gtc/type_ptr.hpp>
#include "Terrain.h"
#include "Sphere.h"

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));

float lastX = 640, lastY = 360; // střed okna (1280/2, 720/2)
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
    float yoffset = lastY - ypos; // obracene, Y roste smerem dolu na obrazovce
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    camera.yaw += xoffset;
    camera.pitch += yoffset;

    // omez pitch, aby se kamera "nepreklopila"
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

void main()
{
    Normal = aNormal;
    gl_Position = projection * view * vec4(aPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 460 core
in vec3 Normal;
out vec4 FragColor;

void main()
{
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diff = max(dot(normalize(Normal), lightDir), 0.0);

    vec3 baseColor = vec3(0.3, 0.6, 0.2);
    vec3 ambient = baseColor * 0.3;
    vec3 result = ambient + baseColor * diff;

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
    vec3 shallowColor = vec3(0.1, 0.5, 0.6);
    vec3 deepColor = vec3(0.0, 0.2, 0.4);

    vec3 color = mix(deepColor, shallowColor, waveHeight + 0.5);
    FragColor = vec4(color, 0.75); // alpha 0.75 = mirne pruhledne
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

const char* skyFragmentShaderSource = R"(
#version 460 core
in vec3 Direction;
out vec4 FragColor;

uniform vec3 sunDirection;

void main()
{
    vec3 dir = normalize(Direction);

    vec3 horizonColor = vec3(0.7, 0.8, 0.9);
    vec3 zenithColor  = vec3(0.2, 0.4, 0.8);

    float t = clamp(dir.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 skyColor = mix(horizonColor, zenithColor, t);

    float sun = pow(max(dot(dir, normalize(sunDirection)), 0.0), 256.0);
    skyColor += vec3(1.0, 0.9, 0.7) * sun;

    FragColor = vec4(skyColor, 1.0);
}
)";

// Sphere (hrac) shadery - stejny princip osvetleni jako teren, ale s "model" maticí,
// protoze koule ma vrcholy kolem stredu (0,0,0) a musime ji posunout na pozici hrace
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
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float diff = max(dot(normalize(Normal), lightDir), 0.0);

    vec3 baseColor = vec3(0.9, 0.3, 0.2); // cervena/oranzova - hrac
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
    // Inicializace GLFW
    if (!glfwInit())
    {
        std::cerr << "Nepodarilo se inicializovat GLFW\n";
        return -1;
    }

    // Nastaveni verze OpenGL (4.6 core profile)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Vytvoreni okna
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Zenith", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Nepodarilo se vytvorit okno\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);

    // Nacteni OpenGL funkci pres GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Nepodarilo se nacist GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL); // potreba kvuli skybox triku (z = w, tedy presne na far plane)

    // Povoleni blendingu - potrebne pro pruhlednou vodu
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

     // Vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    // Fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    // Spojeni do shader programu
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Uklid - jednotlive shadery uz nejsou potreba, jsou soucasti programu
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Water shadery
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

    // Sky shadery
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

    // Sphere (hrac) shadery
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

    // Hrac (koule) - polomer a pocatecni pozice na povrchu terenu
    float playerRadius = 1.5f;
    Sphere playerSphere;
    playerSphere.Generate(playerRadius, 20);

    glm::vec3 playerPosition = glm::vec3(terrain.width / 2.0f, 0.0f, terrain.height / 2.0f);
    playerPosition.y = terrain.GetHeightAt(playerPosition.x, playerPosition.z) + playerRadius;

    // Kamera - pocatecni natoceni (pozice se dopocita kazdy snimek podle hrace)
camera.yaw = 90.0f;
camera.pitch = -20.0f;

    // Vodni rovina - velikost navazana na skutecnou velikost terenu
    float margin = 0.0f; // jak daleko za okraj ostrova voda saha
    float waterLevel = 0.5f; // mirne nad 0, aby se voda neprolinala s terenem (z-fighting)
    float waterVertices[] = {
        -margin, waterLevel, -margin,
        (float)terrain.width + margin, waterLevel, -margin,
        (float)terrain.width + margin, waterLevel, (float)terrain.height + margin,

        -margin, waterLevel, -margin,
        (float)terrain.width + margin, waterLevel, (float)terrain.height + margin,
        -margin, waterLevel, (float)terrain.height + margin,
    };

    // VAO/VBO pro vodu
    unsigned int waterVAO, waterVBO;
    glGenVertexArrays(1, &waterVAO);
    glGenBuffers(1, &waterVBO);

    glBindVertexArray(waterVAO);
    glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(waterVertices), waterVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // VAO/VBO pro oblohu
    unsigned int skyVAO, skyVBO;
    glGenVertexArrays(1, &skyVAO);
    glGenBuffers(1, &skyVBO);

    glBindVertexArray(skyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Nastaveni kamery pro treti osobu (za a nad hracem)
    float cameraDistance = 10.0f;
    float cameraHeight = 1.0f;
    float playerSpeed = 6.0f;

    // Hlavni smycka
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
       
        // Vstup
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        // Pohyb hrace (koule) - smer podle toho, kam se kamera divá (vodorovne, bez sklonu)
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

        // "Prilnuti" k terenu - koule vzdy sedi na povrchu
        playerPosition.y = terrain.GetHeightAt(playerPosition.x, playerPosition.z) + playerRadius;

        // Kamera sleduje hrace zezadu a shora, podle smeru pohledu (yaw/pitch z mysi)
        camera.position = playerPosition - camera.GetFront() * cameraDistance + glm::vec3(0.0f, cameraHeight, 0.0f);

        // Vykresleni
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 500.0f);

        // Vykresleni oblohy - jako prvni, ma byt vzdy "za vsim"
        glUseProgram(skyShaderProgram);

        int skyViewLoc = glGetUniformLocation(skyShaderProgram, "view");
        int skyProjLoc = glGetUniformLocation(skyShaderProgram, "projection");
        int sunDirLoc = glGetUniformLocation(skyShaderProgram, "sunDirection");

        glUniformMatrix4fv(skyViewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(skyProjLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3f(sunDirLoc, 0.5f, 1.0f, 0.3f); // stejny smer jako svetlo na terenu

        glBindVertexArray(skyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Vykresleni terenu
        glUseProgram(shaderProgram);

        int viewLoc = glGetUniformLocation(shaderProgram, "view");
        int projLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        terrain.Draw();

        // Vykresleni hrace (koule)
        glUseProgram(sphereShaderProgram);

        glm::mat4 sphereModel = glm::translate(glm::mat4(1.0f), playerPosition);

        int sphereModelLoc = glGetUniformLocation(sphereShaderProgram, "model");
        int sphereViewLoc = glGetUniformLocation(sphereShaderProgram, "view");
        int sphereProjLoc = glGetUniformLocation(sphereShaderProgram, "projection");

        glUniformMatrix4fv(sphereModelLoc, 1, GL_FALSE, glm::value_ptr(sphereModel));
        glUniformMatrix4fv(sphereViewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(sphereProjLoc, 1, GL_FALSE, glm::value_ptr(projection));

        playerSphere.Draw();

        // Vykresleni vody
        glUseProgram(waterShaderProgram);

        int waterViewLoc = glGetUniformLocation(waterShaderProgram, "view");
        int waterProjLoc = glGetUniformLocation(waterShaderProgram, "projection");
        int timeLoc = glGetUniformLocation(waterShaderProgram, "time");

        glUniformMatrix4fv(waterViewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(waterProjLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniform1f(timeLoc, (float)glfwGetTime());

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
