#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "config/config.h"
#include "modules/debug/debug.h"

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

    // Nacteni OpenGL funkci pres GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Nepodarilo se nacist GLAD\n";
        return -1;
    }

    // Hlavni smycka
    while (!glfwWindowShouldClose(window))
    {
        // Vstup
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        // Vykresleni (zatim jen vycisteni obrazovky)
        glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}