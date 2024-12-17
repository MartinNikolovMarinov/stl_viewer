#include <platform.h>
#include <vulkan_include.h>

#include <windows.h>
#include <iostream>

static HINSTANCE g_hInstance = nullptr;
static HWND g_hWnd = nullptr;
static const char* g_windowClassName = "VulkanWindowClass";

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CLOSE:
            DestroyWindow(hWnd);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

Platform::Error Platform::init(const char* windowTitle, i32 windowWidth, i32 windowHeight) {
    g_hInstance = GetModuleHandle(NULL);

    // Register the window class
    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = g_hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = g_windowClassName;

    if (!RegisterClassExA(&wc)) {
        std::cerr << "Failed to register window class.\n";
        return Platform::Error::FAILED_TO_CREATE_SURFACE; // Use a generic error
    }

    DWORD style = WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX);
    RECT rect = {0, 0, windowWidth, windowHeight};
    AdjustWindowRect(&rect, style, FALSE);

    g_hWnd = CreateWindowA(g_windowClassName, windowTitle, style,
                           CW_USEDEFAULT, CW_USEDEFAULT,
                           rect.right - rect.left, rect.bottom - rect.top,
                           NULL, NULL, g_hInstance, NULL);

    if (!g_hWnd) {
        std::cerr << "Failed to create window.\n";
        return Platform::Error::FAILED_TO_CREATE_SURFACE;
    }

    ShowWindow(g_hWnd, SW_SHOW);
    UpdateWindow(g_hWnd);

    return Platform::Error::SUCCESS;
}

Platform::Error Platform::start() {
    MSG msg = {};
    while (true) {
        BOOL res = GetMessageA(&msg, NULL, 0, 0);
        if (res == -1) {
            std::cerr << "Error in message loop.\n";
            return Platform::Error::FAILED_TO_CREATE_SURFACE;
        }
        else if (res == 0) {
            // WM_QUIT received
            break;
        }
        else {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

    return Platform::Error::SUCCESS;
}

void Platform::requiredVulkanExtsCount(i32& count) {
    count = 1;
}

void Platform::requiredVulkanExts(const char** extensions) {
    extensions[0] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
}

Platform::Error Platform::createVulkanSurface(VkInstance instance, VkSurfaceKHR& outSurface) {
    VkWin32SurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hinstance = g_hInstance;
    createInfo.hwnd = g_hWnd;

    VkResult result = vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &outSurface);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create Win32 Vulkan surface.\n";
        return Platform::Error::FAILED_TO_CREATE_SURFACE;
    }

    return Platform::Error::SUCCESS;
}
