#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "config/config.h"
#include "modules/debug/debug.h"
#include "modules/camera/camera.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);

    if (height > 0)
      camera.aspectRatio = (float)width / (float)height;
        
}

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
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    
    // Nacteni OpenGL funkci pres GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Nepodarilo se nacist GLAD\n";
        return -1;
    }

    //Debugging
    initImGui(window);
    FPSCounter fpsCounter;
    bool showDebugInfo = false;
    bool prevF1Pressed = false;

    // Hlavni smycka
    while (!glfwWindowShouldClose(window))
    {

        //FPS a  Imgui debug
        double currentTime = glfwGetTime();
        updateFPS(fpsCounter, currentTime);

        const bool f1Pressed = glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS;
        if (f1Pressed && !prevF1Pressed)
        {
            showDebugInfo = !showDebugInfo;   
        }
            prevF1Pressed = f1Pressed;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();  
        ImGui::NewFrame();
        renderImGui(fpsCounter, showDebugInfo);

        // Vstup
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        // Vykresleni (zatim jen vycisteni obrazovky)
        glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        //render imgui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}