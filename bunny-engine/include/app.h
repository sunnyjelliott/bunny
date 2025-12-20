#pragma once
#include "pch.h"

class Application {
public:
    void run();

private:
    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();

    // Vulkan instance creation
    void createInstance();

    GLFWwindow* m_window = nullptr;
    VkInstance m_instance = VK_NULL_HANDLE;

    const uint32_t WIDTH = 1200;
    const uint32_t HEIGHT = 800;
};