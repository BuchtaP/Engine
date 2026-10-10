#pragma once
#include <GLFW/glfw3.h>
#include "config/config.h"
#include "debug/imgui/imgui.h"
#include "debug/imgui/backends/imgui_impl_glfw.h"
#include "debug/imgui/backends/imgui_impl_opengl3.h"

struct FPSCounter
{
    double lastTime = 0.0;
    int frameCount = 0;
    float fps = 0.0f;
};

inline void initImGui(GLFWwindow* window)
{
IMGUI_CHECKVERSION();
ImGui::CreateContext();
ImGuiIO& io = ImGui::GetIO();
io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch

// Setup Platform/Renderer backends
ImGui_ImplGlfw_InitForOpenGL(window, true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
ImGui_ImplOpenGL3_Init("#version 460");
}

inline void updateFPS(FPSCounter& fpsCounter, double currentTime)
{
    fpsCounter.frameCount++;
    if (currentTime - fpsCounter.lastTime >= 1.0)
    {
        fpsCounter.fps = (float)(fpsCounter.frameCount / (currentTime - fpsCounter.lastTime));
        fpsCounter.frameCount = 0;
        fpsCounter.lastTime = currentTime;
    }
}

inline void renderImGui(FPSCounter& fpsCounter, bool show)
{
    #ifdef enable_debug
    if (!show) return;
    {
        ImGui::Begin("Debug Info");
        ImGui::Text("FPS: %.1f", fpsCounter.fps);
        ImGui::End();
    }
    #endif
}