/*
 * Copyright (C) 2024-2026 WinlatorXR
 *
 * This file is part of WinlatorXR.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "WinXrApiUDP.h"
#include "wxr_bridge.h"

// Minimal OpenXR Simulator Runtime (D3D11/D3D12/OpenGL)
// - Implements enough of the runtime interface to let OpenXR apps start and render into runtime-owned swapchains
// - Opens a desktop window and presents the app's submitted images side-by-side
// - Supports D3D11, D3D12, and OpenGL graphics APIs

#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11
#define XR_USE_GRAPHICS_API_D3D12
#define XR_USE_GRAPHICS_API_OPENGL
#define XR_USE_GRAPHICS_API_VULKAN
#define VK_USE_PLATFORM_WIN32_KHR

#include <windows.h>
#include <WinUser.h>
#include <wrl/client.h>
#include <vulkan/vulkan.h>
#include <d3d11.h>
#include <d3d11_4.h>
#include <d3d12.h>
#include <d3d11on12.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>

//----------------
//OXRWXR CHANGE:
//---------------- 
// New includes
#include <Winsock2.h>
#include <cmath>
#include <condition_variable>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <io.h>
#include <iostream>
#include <locale>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <ws2tcpip.h>
#include <xr_linear.h>
#include <algorithm>
// New defines
#define D3DX12_COLOR_F(r, g, b, a) { (FLOAT)(r), (FLOAT)(g), (FLOAT)(b), (FLOAT)(a) }
#define D3D11_COLOR_ARGB(a, r, g, b) ((UINT)(((a) << 24) | ((r) << 16) | ((g) << 8) | (b)))


// OpenGL headers - minimal definitions for what we need
#include <GL/gl.h>

// OpenGL extension constants and types (from glext.h / wglext.h)
// We define these inline since Windows doesn't ship with glext.h
#ifndef GL_SRGB8_ALPHA8
#define GL_SRGB8_ALPHA8                   0x8C43
#endif
#ifndef GL_RGBA8
#define GL_RGBA8                          0x8058
#endif
#ifndef GL_BGRA
#define GL_BGRA                           0x80E1
#endif
#ifndef GL_RGBA16F
#define GL_RGBA16F                        0x881A
#endif
#ifndef GL_RGBA32F
#define GL_RGBA32F                        0x8814
#endif
#ifndef GL_RGB10_A2
#define GL_RGB10_A2                       0x8059
#endif
#ifndef GL_DEPTH_COMPONENT32F
#define GL_DEPTH_COMPONENT32F             0x8CAC
#endif
#ifndef GL_DEPTH24_STENCIL8
#define GL_DEPTH24_STENCIL8               0x88F0
#endif
#ifndef GL_DEPTH_COMPONENT16
#define GL_DEPTH_COMPONENT16              0x81A5
#endif
#ifndef GL_DEPTH_STENCIL
#define GL_DEPTH_STENCIL                  0x84F9
#endif
#ifndef GL_UNSIGNED_INT_24_8
#define GL_UNSIGNED_INT_24_8              0x84FA
#endif
#ifndef GL_TEXTURE_2D_ARRAY
#define GL_TEXTURE_2D_ARRAY               0x8C1A
#endif
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER                    0x8D40
#endif
#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0              0x8CE0
#endif
#ifndef GL_FRAMEBUFFER_COMPLETE
#define GL_FRAMEBUFFER_COMPLETE           0x8CD5
#endif

// Function pointer types for GL extension functions
typedef void (APIENTRY *PFNGLTEXIMAGE3DPROC)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void* pixels);
typedef void (APIENTRY *PFNGLGENFRAMEBUFFERSPROC)(GLsizei n, GLuint* framebuffers);
typedef void (APIENTRY *PFNGLDELETEFRAMEBUFFERSPROC)(GLsizei n, const GLuint* framebuffers);
typedef void (APIENTRY *PFNGLBINDFRAMEBUFFERPROC)(GLenum target, GLuint framebuffer);
typedef void (APIENTRY *PFNGLFRAMEBUFFERTEXTURE2DPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef GLenum (APIENTRY *PFNGLCHECKFRAMEBUFFERSTATUSPROC)(GLenum target);
typedef void (APIENTRY *PFNGLREADPIXELSPROC)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels);
typedef void (APIENTRY *PFNGLGETTEXTURESUBIMAGEPROC)(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLsizei bufSize, void* pixels);

// GL function pointers (loaded at runtime)
static PFNGLTEXIMAGE3DPROC g_glTexImage3D = nullptr;
static PFNGLGENFRAMEBUFFERSPROC g_glGenFramebuffers = nullptr;
static PFNGLDELETEFRAMEBUFFERSPROC g_glDeleteFramebuffers = nullptr;
static PFNGLBINDFRAMEBUFFERPROC g_glBindFramebuffer = nullptr;
static PFNGLFRAMEBUFFERTEXTURE2DPROC g_glFramebufferTexture2D = nullptr;
static PFNGLCHECKFRAMEBUFFERSTATUSPROC g_glCheckFramebufferStatus = nullptr;
static PFNGLGETTEXTURESUBIMAGEPROC g_glGetTextureSubImage = nullptr;
static bool g_glGetTextureSubImageLoaded = false;
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <cstring>
#include <cstdlib>
#include <thread>
#include <mutex>
#include <atomic>
#include <deque>
#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <chrono>

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <openxr/openxr_reflection.h>
#include <loader_interfaces.h>

using Microsoft::WRL::ComPtr;

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "opengl32.lib")

// D3D12 helper - calculates subresource index (from d3dx12.h)
inline UINT D3D12CalcSubresource(UINT MipSlice, UINT ArraySlice, UINT PlaneSlice, UINT MipLevels, UINT ArraySize) {
    return MipSlice + ArraySlice * MipLevels + PlaneSlice * MipLevels * ArraySize;
}


// Simple logging (debug output + file log)
static FILE* g_LogFile = nullptr;
static void EnsureLogFile(bool append = true) {
    if (g_LogFile) return;
    std::filesystem::create_directories("D:\\Winlator\\oxrwxr\\logs");
    if (append) {
        fopen_s(&g_LogFile, "D:\\Winlator\\oxrwxr\\logs\\openxr_wxr.log", "a");
    } else {
        fopen_s(&g_LogFile, "D:\\Winlator\\oxrwxr\\logs\\openxr_wxr.log", "w");
    }
}
static void Log(const char* msg) {
    OutputDebugStringA(msg);
    EnsureLogFile();
    if (g_LogFile) { fputs(msg, g_LogFile); if (msg[0] && msg[strlen(msg) - 1] != '\n') fputc('\n', g_LogFile); fflush(g_LogFile); }
}
static void Log(const std::string& msg) { Log(msg.c_str()); }
static void Logf(const char* fmt, ...) {
    char buf[2048];
    va_list ap; va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    Log(buf);
}

// Helper to load glTexImage3D (OpenGL 1.2+ function not in Windows GL headers)
static bool EnsureGLTexImage3D() {
    if (g_glTexImage3D) return true;
    g_glTexImage3D = (PFNGLTEXIMAGE3DPROC)wglGetProcAddress("glTexImage3D");
    if (!g_glTexImage3D) {
        Log("[SimXR] Failed to load glTexImage3D");
        return false;
    }
    return true;
}

static bool EnsureGLFramebufferFuncs() {
    if (g_glGenFramebuffers) return true;
    g_glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)wglGetProcAddress("glGenFramebuffers");
    g_glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)wglGetProcAddress("glDeleteFramebuffers");
    g_glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)wglGetProcAddress("glBindFramebuffer");
    g_glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)wglGetProcAddress("glFramebufferTexture2D");
    g_glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)wglGetProcAddress("glCheckFramebufferStatus");
    if (!g_glGenFramebuffers || !g_glDeleteFramebuffers || !g_glBindFramebuffer ||
        !g_glFramebufferTexture2D || !g_glCheckFramebufferStatus) {
        Log("[SimXR] Failed to load GL framebuffer functions");
        return false;
    }
    return true;
}

// Helper function to convert a typed format to typeless
static DXGI_FORMAT ToTypeless(DXGI_FORMAT format) {
    switch (format) {
        // R8G8B8A8 family
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
            return DXGI_FORMAT_R8G8B8A8_TYPELESS;
            
        // B8G8R8A8 family
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            return DXGI_FORMAT_B8G8R8A8_TYPELESS;
            
        // R16G16B16A16 family
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_SINT:
            return DXGI_FORMAT_R16G16B16A16_TYPELESS;
            
        // R32G32B32A32 family
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
            return DXGI_FORMAT_R32G32B32A32_TYPELESS;
            
        // R10G10B10A2 family
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R10G10B10A2_UINT:
            return DXGI_FORMAT_R10G10B10A2_TYPELESS;
            
        // Already typeless or depth formats - return as-is
        default:
            return format;
    }
}

static XrResult write_str(const char *str, uint32_t cap, uint32_t *count, char *buf)
{
    uint32_t n = (uint32_t)strlen(str) + 1;
    *count = n;
    if (cap == 0) return XR_SUCCESS;
    if (cap < n) return XR_ERROR_SIZE_INSUFFICIENT;
    memcpy(buf, str, n);
    return XR_SUCCESS;
}

//----------------
//OXRWXR CHANGE:
//---------------- 
// Create global variables
static bool disable2DLayer = false;
static bool disableRightEye = false;
static bool disableWindow = false;
static bool legacyInputMatching = false;
static bool monoRendering = false;
static bool verboseLogging = false;
static bool directTransport = false;
// Profile picked over the ranking when the app suggested it, e.g. "/oculus/touch_controller"
static std::string preferredProfile;
static bool sendHaptics = true;

static WinXrApiUDP* udpReader;

static std::string hmdMake;
static std::string hmdModel;

static bool isVR = false;
static int lastPosFrame = 0;
static int maxPosBuffer = 1;
static float toRadians = 3.14159265f / 180.0f;

static float IPDVal;
static float FOVH = -1;
static float FOVV = -1;

static std::map<std::pair<XrSpace, XrSpace>, XrPosef> SpacePoses;
static std::map<std::pair<XrSpace, XrSpace>, XrSpaceVelocity> SpaceVelocities;

static int OpenXRFrameID = 0;
static int OpenXRFrameWait = 0;
static int ViewportWidth = 1280;
static int ViewportHeight = 720;
static int DirectEyeWidth = 0;   // Per-eye size for direct transport from the host (headset x render scale), 0 if none
static int DirectEyeHeight = 0;
static bool BridgeReady();

static XrVector2f makeXrVector2f(float x, float y) {
    XrVector2f vec;
    vec.x = x;
    vec.y = y;
    return vec;
}

static bool parseBool(const std::string str) {
    size_t pos = str.find('=');
    if (pos == std::string::npos) return false;

    std::string value = str.substr(pos + 1);
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return std::tolower(c); });
    return value == "true" || value == "1" || value == "yes" || value == "t";
}

static bool compareValue(const std::string str, const std::string compareTo) {
    size_t pos = str.find('=');
    if (pos == std::string::npos) return false;

    std::string value = str.substr(pos + 1);
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return std::tolower(c); });
    std::string compareToLower = compareTo;
    std::transform(compareToLower.begin(), compareToLower.end(), compareToLower.begin(), [](unsigned char c) { return std::tolower(c); });
    return value == compareToLower;
}

static bool compareKey(const std::string str, const std::string compareTo) {
    size_t pos = str.find('=');
    if (pos == std::string::npos) return false;

    std::string key = str.substr(0, pos);
    std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return std::tolower(c); });
    std::string compareToLower = compareTo;
    std::transform(compareToLower.begin(), compareToLower.end(), compareToLower.begin(), [](unsigned char c) { return std::tolower(c); });
    return key == compareToLower;
}

// Runtime state
namespace rt {

struct Swapchain;

// Forward declarations
void PushState(XrSession s, XrSessionState newState);

// Global adapter LUID that we'll use consistently
static LUID g_adapterLuid = {};
static bool g_adapterLuidSet = false;

// Global persistent window that survives session creation/destruction
static HWND g_persistentWindow = nullptr;
static std::mutex g_windowMutex;
static ComPtr<IDXGISwapChain1> g_persistentSwapchain;
static bool g_windowClassRegistered = false;
static std::map<XrSpace, XrPosef> g_referenceSpacePose;
static std::map<XrSpace, XrReferenceSpaceType> g_referenceSpaceType;
static std::map<std::pair<XrSpace, XrSpace>, bool> g_locateSpaces;
static bool g_spacesChanged = false;

static PFN_vkGetInstanceProcAddr g_app_vk_gipa = NULL;
static XrCompositionLayerProjection g_last_proj;

struct Instance {
    XrInstance handle{(XrInstance)1};
    std::vector<std::string> enabledExtensions;
    XrVersion apiVersion{XR_API_VERSION_1_0};
};

struct Session {
    XrSession handle{(XrSession)1};
    XrSessionState state{XR_SESSION_STATE_IDLE};

    // Vulkan support
    VkInstance vkInstance{VK_NULL_HANDLE};
    VkPhysicalDevice vkPhysicalDevice{VK_NULL_HANDLE};
    VkDevice vkDevice{VK_NULL_HANDLE};
    uint32_t vkQueueFamilyIndex{0};
    uint32_t vkQueueIndex{0};
    VkQueue vkQueue{VK_NULL_HANDLE};
    VkCommandPool vkCmdPool{VK_NULL_HANDLE};
    VkFence vkAcquireFence;
    bool usesVulkan{false};

    // OpenGL support
    HDC glDC{nullptr};
    HGLRC glRC{nullptr};
    bool usesOpenGL{false};

    // DX11 support
    ComPtr<ID3D11Device> d3d11Device;
    ComPtr<ID3D11DeviceContext> d3d11Context;
    // DX12 support
    ComPtr<ID3D12Device> d3d12Device;
    ComPtr<ID3D12CommandQueue> d3d12Queue;
    bool usesD3D12{false};

    // DX12 preview resources
    ComPtr<IDXGISwapChain3> previewSwapchain12;
    ComPtr<ID3D12DescriptorHeap> previewRTVHeap;
    std::vector<ComPtr<ID3D12Resource>> previewBackbuffers;
    UINT previewRTVDescriptorSize{0};
    UINT previewBackbufferCount{0};
    ComPtr<ID3D12CommandAllocator> previewCmdAlloc;
    ComPtr<ID3D12GraphicsCommandList> previewCmdList;
    ComPtr<ID3D12Fence> previewFence;
    HANDLE previewFenceEvent{nullptr};
    UINT64 previewFenceValue{0};

    // Vulkan preview resources
    VkSurfaceKHR vkPreviewSurface;
    VkSwapchainKHR vkPreviewSwapchain;
    VkBuffer vkSyncBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vkSyncBufferMemory = VK_NULL_HANDLE;
    // Per preview image, so the blit can be handed to the present instead of waited on: the
    // semaphore orders blit before present on the GPU, the fence says when the command buffer of
    // that image is free to record into again, and both are created once with the swapchain.
    std::vector<VkImage> vkPreviewImages;
    std::vector<VkCommandBuffer> vkBlitCmds;
    std::vector<VkSemaphore> vkBlitDone;
    std::vector<VkFence> vkBlitFences;
    std::vector<bool> vkBlitSubmitted;

    // Blit resources
    ComPtr<ID3D11VertexShader> blitVS;
    ComPtr<ID3D11PixelShader> blitPS;
    ComPtr<ID3D11Buffer> blitConstantBuffer;
    ComPtr<ID3D11SamplerState> samplerState;
    ComPtr<ID3D11RasterizerState> noCullRS;  // Rasterizer state with culling disabled
    ComPtr<ID3D11BlendState> quadBlend[3];   // Quad layers: opaque, premultiplied, unpremultiplied alpha
    ComPtr<ID3D11VertexShader> solidColorVS;
    ComPtr<ID3D11PixelShader> solidColorPS;
    ComPtr<ID3D11InputLayout> simpleVertexLayout = nullptr;
    ComPtr<ID3D11Buffer> colorConstantBuffer;
    ComPtr<ID3D11Buffer> viewportConstantBuffer;
    ComPtr<ID3DBlob> solidColorVSBlob;
    ComPtr<ID3DBlob> solidColorPSBlob;


    // Desktop preview window (no thread - handled on main thread)
    HWND hwnd{nullptr};
    std::atomic<bool> isFocused{false};
    ComPtr<IDXGISwapChain1> previewSwapchain;
    UINT previewWidth{(UINT)ViewportWidth};
    UINT previewHeight{(UINT)ViewportHeight};
    int previewFormat = (int)DXGI_FORMAT_UNKNOWN;  // Track format for matching
    std::mutex previewMutex;
};

struct BlitConstants {
    float uvMinX;
    float uvMinY;
    float uvMaxX;
    float uvMaxY;
};

struct Swapchain {
    XrSwapchain handle{(XrSwapchain)1};
    DXGI_FORMAT format{DXGI_FORMAT_R8G8B8A8_UNORM};
    uint32_t width{0}, height{0}, arraySize{2};
    uint32_t mipCount{1};
    // Backend type and images
    enum class Backend { D3D11, D3D12, OpenGL, Vulkan } backend{Backend::D3D11};
    std::vector<ComPtr<ID3D11Texture2D>> images;      // D3D11 path
    std::vector<ComPtr<ID3D12Resource>> images12;     // D3D12 path
    std::vector<D3D12_RESOURCE_STATES> imageStates12;
    std::vector<GLuint> imagesGL;                     // OpenGL path
    GLenum glInternalFormat{GL_RGBA8};                // OpenGL internal format
    std::vector<VkImage> imagesVK;                    // Vulkan path
    std::vector<VkDeviceMemory> imageMemoriesVK;      // Vulkan path
    VkFormat vkFormat{VK_FORMAT_R8G8B8A8_UNORM};      // Vulkan format
    uint32_t nextIndex{0};
    uint32_t lastAcquired{UINT32_MAX};  // Initialize to invalid
    uint32_t lastReleased{UINT32_MAX};  // Initialize to invalid
    uint32_t imageCount{3};
    std::vector<uint32_t> bridgeIds;    // Direct transport slot per D3D11 image, empty when not shared
};

static Instance g_instance{};
static Session g_session{};
static std::unordered_map<XrSwapchain, Swapchain> g_swapchains;
uintptr_t g_nextSwapchainHandle = 1;

// Controller tracking state for motion controller emulation
// Positions are relative to head position, orientation follows head by default
struct ControllerState {
    // Input state for button/trigger emulation
    bool triggerPressed;   // Primary trigger (fire)
    bool gripPressed;      // Grip button
    bool menuPressed;      // Menu button
    bool primaryPressed;   // Primary button (A/X)
    bool secondaryPressed; // Secondary button (B/Y)
    bool thumbstickPressed;// Thumbstick click
    float triggerValue;    // 0.0-1.0 trigger analog value
    float gripValue;       // 0.0-1.0 grip analog value
    XrVector2f thumbstick; // -1.0 to 1.0 thumbstick position

    // Velocity tracking for motion detection
    XrVector3f linearVelocity;  // m/s in world space
    XrVector3f angularVelocity; // rad/s
};
static ControllerState g_leftController = {};
static ControllerState g_rightController = {};

// Map XrSpace handles to controller type
static std::unordered_map<XrAction, XrPath> g_actionPaths;
// A suggested binding, kept with the profile it was suggested under
struct ActionBinding {
    XrPath profile;
    XrPath binding;
};
// Every suggested binding for an action, not just the last one. An action is
// routinely suggested on both hands (and on several interaction profiles), so
// g_actionPaths alone cannot tell us which hand a query refers to.
static std::unordered_map<XrAction, std::vector<ActionBinding>> g_actionBindings;
static std::unordered_map<XrSpace, bool> g_controllerGrips;
static std::unordered_map<XrSpace, XrPosef> g_controllerPoses;
static std::unordered_map<XrSpace, int> g_controllerSpaces;
static std::unordered_map<XrPath, bool> g_interactionProfiles;
// Profiles in the order they were suggested, so a tie between equally ranked
// profiles resolves the same way on every run
static std::vector<XrPath> g_profileOrder;
// The profile the bindings were frozen to at xrAttachSessionActionSets
static XrPath g_activeProfile = XR_NULL_PATH;

// Cache for controllers to be created
static std::unordered_map<XrSpace, XrActionSpaceCreateInfo> g_controllerInfo;
// Guards the controller maps: they are drained from xrLocateSpace (render thread) and xrSyncActions (game thread)
static std::mutex g_controllerMutex;

// Whether this session has ever submitted a projection layer
static bool g_sessionWasVR = false;

// Map XrPath to path string for controller detection
static std::unordered_map<XrPath, std::string> g_pathStrings;

// Map XrAction to action name for input mapping
static std::unordered_map<XrAction, std::string> g_actionNames;

// Time tracking for velocity calculation
static XrTime g_lastFrameTime = 0;

// Returns whether the space was registered; an unresolved one is kept pending, since the
// suggested bindings it is resolved through may not have been made yet
static bool AddController(XrSpace* space, XrActionSpaceCreateInfo* info) {

    // Detect controller subaction paths and register the space
    bool controllerGrip = false;
    bool controllerPalm = false;
    int controllerType = 0;  // 0=none, 1=left, 2=right
    if (info->action != XR_NULL_PATH) {
        if (rt::g_actionPaths.find(info->action) != rt::g_actionPaths.end()) {
            auto it = rt::g_pathStrings.find(rt::g_actionPaths[info->action]);
            if (it != rt::g_pathStrings.end()) {
                const std::string& pathStr = it->second;
		if (pathStr.find("/grip/") != std::string::npos) {
                    controllerGrip = true;
                }
                // The host only knows grip and aim, so the palm is a grip with a fixed offset
                if (pathStr.find("/palm_ext/") != std::string::npos || pathStr.find("/grip_surface/") != std::string::npos) {
                    controllerGrip = true;
                    controllerPalm = true;
                }
            }
            if (info->subactionPath != XR_NULL_PATH) {
                it = rt::g_pathStrings.find(info->subactionPath);
            }

            if (it != rt::g_pathStrings.end()) {
                const std::string& pathStr = it->second;
                if (pathStr.find("/user/hand/left") != std::string::npos) {
                    controllerType = 1;  // Left controller
                } else if (pathStr.find("/user/hand/right") != std::string::npos) {
                    controllerType = 2;  // Right controller
                }
                Logf("[SimXR] AddController: space=%d, type=%d, grip=%d", *space, controllerType, (int)controllerGrip);
            }
        }
    } else {
        Log("[SimXR] AddController: subactionPath is XR_NULL_PATH");
    }

    if (controllerType > 0) {
        rt::g_controllerGrips[*space] = controllerGrip;
        rt::g_controllerPoses[*space] = info->poseInActionSpace;
        if (controllerPalm) {
            // Where the Touch controller's palm pose sits from its grip pose: tilted 70 degrees about X
            // and moved to the palm's surface. The right hand is the mirror image.
            XrPosef palmInGrip = {{-0.5735764f, 0.0f, 0.0f, 0.8191520f},
                                  {controllerType == 1 ? -0.01f : 0.01f, 0.013f, 0.0075f}};
            XrPosef_Multiply(&rt::g_controllerPoses[*space], &palmInGrip, &info->poseInActionSpace);
        }
        rt::g_controllerSpaces[*space] = controllerType;
        rt::g_spacesChanged = true;
    }
    return controllerType > 0;
}

// xrSyncActions is the usual moment to register action spaces, but a game that reads the legacy
// controller state through OpenComposite never calls it (Maquette), and its controllers would
// then never be reported to the host and never get a pose. So xrLocateSpace drains these too.
static void DrainPendingControllers() {
    std::lock_guard<std::mutex> lock(g_controllerMutex);
    for (auto it = g_controllerInfo.begin(); it != g_controllerInfo.end();) {
        XrSpace space = it->first;
        it = AddController(&space, &it->second) ? g_controllerInfo.erase(it) : std::next(it);
    }
}

// Initialize shader resources for blitting
bool InitBlitResources(Session& s) {
    if (s.blitVS && s.blitPS && s.samplerState && s.noCullRS && s.quadBlend[0]) {
        return true;
    }

    // Compile shaders
    const char* shaderSource = R"(
        Texture2D txDiffuse : register(t0);
        SamplerState samLinear : register(s0);

        cbuffer Constants : register(b0)
        {
            float uvMinX;
            float uvMinY;
            float uvMaxX;
            float uvMaxY;
        };

        struct VS_OUTPUT {
            float4 Pos : SV_POSITION;
            float2 Tex : TEXCOORD;
        };

        // Vertex Shader (generates fullscreen quad with correct UV mapping)
        VS_OUTPUT VSMain(uint vertexId : SV_VertexID) {
            VS_OUTPUT output;
            // Generate (0,0), (2,0), (0,2), (2,2) pattern
            float2 xy = float2((vertexId << 1) & 2, vertexId & 2);

            // Clip-space position: xy goes 0-2, we need -1 to 1
            // x: 0->-1, 2->1  means x_clip = xy.x - 1
            // y: 0->1, 2->-1  means y_clip = 1 - xy.y
            output.Pos = float4(xy.x - 1.0, 1.0 - xy.y, 0.0, 1.0);

            // Normalized UVs (0-1 range, not 0-2)
            output.Tex = xy * 0.5;

            return output;
        }

        // Pixel Shader - GPU handles sRGB conversion automatically with proper formats
        float4 PSMain(VS_OUTPUT input) : SV_TARGET {
            float2 uv;

            // Map fullscreen UVs to the requested source rectangle
            uv.x = lerp(uvMinX, uvMaxX, input.Tex.x);
            uv.y = lerp(uvMinY, uvMaxY, input.Tex.y);

            return txDiffuse.Sample(samLinear, uv);
        }
    )";

    ComPtr<ID3DBlob> vsBlob, psBlob, errorBlob;
    HRESULT hr;
    UINT compileFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;

    // Constant buffer for flipping upside down the output
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = 16; // Constant buffers must be a multiple of 16 bytes
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = 0;
    cbDesc.MiscFlags = 0;
    cbDesc.StructureByteStride = 0;
    hr = s.d3d11Device->CreateBuffer(&cbDesc, nullptr, s.blitConstantBuffer.GetAddressOf());
    if (FAILED(hr)) {
        Logf("[SimXR] Failed to create blit constant buffer: 0x%08X", hr);
        return false;
    }

    // Compile VS
    hr = D3DCompile(shaderSource, strlen(shaderSource), "BlitShader", nullptr, nullptr, 
                    "VSMain", "vs_5_0", compileFlags, 0, vsBlob.GetAddressOf(), errorBlob.GetAddressOf());
    if (FAILED(hr)) {
        Logf("[SimXR] Failed to compile VS: %s", errorBlob ? (char*)errorBlob->GetBufferPointer() : "Unknown error");
        return false;
    }
    hr = s.d3d11Device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), 
                                           nullptr, s.blitVS.GetAddressOf());
    if (FAILED(hr)) { Logf("[SimXR] Failed to create VS: 0x%08X", hr); return false; }

    // Compile PS
    hr = D3DCompile(shaderSource, strlen(shaderSource), "BlitShader", nullptr, nullptr, 
                    "PSMain", "ps_5_0", compileFlags, 0, psBlob.GetAddressOf(), errorBlob.GetAddressOf());
    if (FAILED(hr)) {
        Logf("[SimXR] Failed to compile PS: %s", errorBlob ? (char*)errorBlob->GetBufferPointer() : "Unknown error");
        return false;
    }
    hr = s.d3d11Device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), 
                                          nullptr, s.blitPS.GetAddressOf());
    if (FAILED(hr)) { Logf("[SimXR] Failed to create PS: 0x%08X", hr); return false; }

    //----------------
    //OXRWXR CHANGE:
    //----------------
    // OpenXR Frame Data Shader
    const char* solidColorShaderSource = R"(
        cbuffer ViewportBuffer : register(b1) {
            float2 viewportSize;
        };

        struct VertexInput
        {
            float2 pos : POSITION;
        };

        cbuffer ColorBuffer : register(b0) {
            float4 color;
        };

        struct PixelInput
        {
            float4 pos : SV_POSITION;
        };

        PixelInput VSMain(VertexInput input)
        {
            PixelInput output;
            output.pos = float4(input.pos.x / viewportSize.x * 2 - 1, 1 - input.pos.y / viewportSize.y * 2, 0, 1);
            return output;
        }

        float4 PSMain(PixelInput input) : SV_TARGET {
            return color;
        }
    )";

    // Compile vertex shader
    hr = D3DCompile(solidColorShaderSource, strlen(solidColorShaderSource), "SolidColorShader", nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &s.solidColorVSBlob, &errorBlob);
    if (FAILED(hr))
    {
        Logf("[WinXrApi] Failed to compile VS: %s", errorBlob ? (char*)errorBlob->GetBufferPointer() : "Unknown error");
        return false;
    }

    // Create vertex shader
    hr = s.d3d11Device->CreateVertexShader(s.solidColorVSBlob->GetBufferPointer(), s.solidColorVSBlob->GetBufferSize(), nullptr, s.solidColorVS.GetAddressOf());
    if (FAILED(hr))
    {
        Logf("[WinXrApi] Failed to create VS: 0x%08X", hr);
        return false;
    }

    // Compile pixel shader
    hr = D3DCompile(solidColorShaderSource, strlen(solidColorShaderSource), "SolidColorShader", nullptr, nullptr, "PSMain", "ps_5_0", 0, 0, &s.solidColorPSBlob, &errorBlob);
    if (FAILED(hr))
    {
        Logf("[WinXrApi] Failed to compile PS: %s", errorBlob ? (char*)errorBlob->GetBufferPointer() : "Unknown error");
        return false;
    }

    // Create pixel shader
    hr = s.d3d11Device->CreatePixelShader(s.solidColorPSBlob->GetBufferPointer(), s.solidColorPSBlob->GetBufferSize(), nullptr, s.solidColorPS.GetAddressOf());
    if (FAILED(hr))
    {
        Logf("[WinXrApi] Failed to create PS: 0x%08X", hr);
        return false;
    }

    // Create Sampler State
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0; 
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = s.d3d11Device->CreateSamplerState(&sampDesc, s.samplerState.GetAddressOf());
    if (FAILED(hr)) { Logf("[SimXR] Failed to create SamplerState: 0x%08X", hr); return false; }

    // Create Rasterizer State with culling disabled
    D3D11_RASTERIZER_DESC rsDesc{};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_NONE;  // Disable culling to prevent triangles from being discarded
    rsDesc.FrontCounterClockwise = FALSE;
    rsDesc.DepthClipEnable = TRUE;
    rsDesc.ScissorEnable = FALSE;
    rsDesc.MultisampleEnable = FALSE;
    rsDesc.AntialiasedLineEnable = FALSE;
    hr = s.d3d11Device->CreateRasterizerState(&rsDesc, s.noCullRS.GetAddressOf());
    if (FAILED(hr)) { Logf("[SimXR] Failed to create RasterizerState: 0x%08X", hr); return false; }

    // Quad layers composite over the eyes as OpenXR defines: without the blend bit the quad is opaque
    // and its alpha ignored, so the eyes' alpha is kept; with it, colour is premultiplied unless flagged
    for (int i = 0; i < 3; i++) {
        D3D11_BLEND_DESC blendDesc{};
        D3D11_RENDER_TARGET_BLEND_DESC& rt = blendDesc.RenderTarget[0];
        rt.BlendEnable = i > 0;
        rt.SrcBlend = i == 2 ? D3D11_BLEND_SRC_ALPHA : D3D11_BLEND_ONE;
        rt.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        rt.BlendOp = D3D11_BLEND_OP_ADD;
        rt.SrcBlendAlpha = D3D11_BLEND_ONE;
        rt.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        rt.BlendOpAlpha = D3D11_BLEND_OP_ADD;
        rt.RenderTargetWriteMask = i == 0
            ? D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN | D3D11_COLOR_WRITE_ENABLE_BLUE
            : D3D11_COLOR_WRITE_ENABLE_ALL;
        hr = s.d3d11Device->CreateBlendState(&blendDesc, s.quadBlend[i].GetAddressOf());
        if (FAILED(hr)) { Logf("[SimXR] Failed to create quad BlendState: 0x%08X", hr); return false; }
    }

    if (verboseLogging) Log("[SimXR] Blit resources initialized successfully.");
    return true;
}

static void ResetD3D12PreviewResources(rt::Session& s) {
    s.previewSwapchain12.Reset();
    s.previewRTVHeap.Reset();
    s.previewBackbuffers.clear();
    s.previewBackbufferCount = 0;
    s.previewRTVDescriptorSize = 0;
    s.previewCmdAlloc.Reset();
    s.previewCmdList.Reset();
    s.previewFence.Reset();
    s.previewFenceValue = 0;
    if (s.previewFenceEvent) {
        CloseHandle(s.previewFenceEvent);
        s.previewFenceEvent = nullptr;
    }
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CLOSE:
            if (rt::g_session.handle != XR_NULL_HANDLE) {
                rt::PushState(rt::g_session.handle, XR_SESSION_STATE_EXITING);
            }
            Log("[SimXR] WndProc: WM_CLOSE received");
            DestroyWindow(hWnd);
            return 0;
        case WM_DESTROY:
            // DON'T call PostQuitMessage - we're a DLL, not the main app!
            // PostQuitMessage would tell the host application to exit.
            Log("[SimXR] WndProc: WM_DESTROY received");
            return 0;
        case WM_ACTIVATE:
            rt::g_session.isFocused = true;
            if (verboseLogging) Log("[SimXR] WndProc: WM_ACTIVATE -> focused");
            return 0;
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MOUSEMOVE:
            return 0;
        default:
            break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

static void ensurePreview(Session& s) {
    if (s.hwnd) return;
    WNDCLASSW wc{}; wc.lpfnWndProc = WndProc; wc.hInstance = GetModuleHandleW(nullptr); wc.lpszClassName = L"OpenXR Simulator";
    RegisterClassW(&wc);
    s.hwnd = CreateWindowExW(WS_EX_NOACTIVATE, wc.lpszClassName, L"OpenXR Simulator", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                             CW_USEDEFAULT, CW_USEDEFAULT, (int)s.previewWidth, (int)s.previewHeight, nullptr, nullptr, wc.hInstance, nullptr);
    Logf("[SimXR] ensurePreview: hwnd=%p size=%ux%u usesD3D12=%d", s.hwnd, s.previewWidth, s.previewHeight, s.usesD3D12);

    // Make sure window is shown and updated
    if (s.hwnd) {
        ShowWindow(s.hwnd, SW_SHOWNA);
        UpdateWindow(s.hwnd);
        // Never take focus: Unity games ignore gamepad input unless their own window is active

        //----------------
        //OXRWXR CHANGE:
        //----------------
        // Always assume focus
        s.isFocused = true;
    }

    // Swapchain creation is now handled in ensurePreviewSized for both D3D11 and D3D12
}

// Window thread functions removed - window now handled on main thread

} // namespace rt

// ----------------- OpenXR runtime exports -----------------

static XrResult XRAPI_PTR xrGetInstanceProcAddr_runtime(XrInstance, const char* name, PFN_xrVoidFunction* fn);

#ifndef _WIN64
// The loader looks up the undecorated name; x86 stdcall exports it as _name@8
#pragma comment(linker, "/EXPORT:xrNegotiateLoaderRuntimeInterface=_xrNegotiateLoaderRuntimeInterface@8")
#endif
extern "C" __declspec(dllexport) XrResult XRAPI_CALL xrNegotiateLoaderRuntimeInterface(const XrNegotiateLoaderInfo* loaderInfo,
                                                                            XrNegotiateRuntimeRequest* runtimeRequest) {
    try {
        EnsureLogFile();
        Log("\n[SimXR] ========== OpenXR Simulator Runtime Starting ==========\n");
        if (!loaderInfo || !runtimeRequest) {
            Log("[SimXR] xrNegotiateLoaderRuntimeInterface: ERROR - null parameters");
            return XR_ERROR_INITIALIZATION_FAILED;
        }
        
        Logf("[SimXR] xrNegotiateLoaderRuntimeInterface: loaderInfo=%p, runtimeRequest=%p", loaderInfo, runtimeRequest);
        Logf("[SimXR]   Loader minInterfaceVersion=%u, maxInterfaceVersion=%u, minApiVersion=0x%X, maxApiVersion=0x%X",
             loaderInfo->minInterfaceVersion, loaderInfo->maxInterfaceVersion,
             loaderInfo->minApiVersion, loaderInfo->maxApiVersion);
        
        runtimeRequest->runtimeInterfaceVersion = XR_CURRENT_LOADER_RUNTIME_VERSION;
        runtimeRequest->getInstanceProcAddr = xrGetInstanceProcAddr_runtime;
        runtimeRequest->runtimeApiVersion = XR_CURRENT_API_VERSION;
        
        Logf("[SimXR] xrNegotiateLoaderRuntimeInterface: SUCCESS - runtimeApiVersion=0x%X (%u)", 
             runtimeRequest->runtimeApiVersion, runtimeRequest->runtimeApiVersion);
        return XR_SUCCESS;
    } catch (...) {
        Log("[SimXR] xrNegotiateLoaderRuntimeInterface: EXCEPTION caught!");
        return XR_ERROR_INITIALIZATION_FAILED;
    }
}

// xrGetD3D12GraphicsRequirementsKHR (XR_KHR_D3D12_enable)
static XrResult XRAPI_PTR xrGetD3D12GraphicsRequirementsKHR_runtime(
    XrInstance instance, XrSystemId systemId, XrGraphicsRequirementsD3D12KHR* req) {
    Logf("[SimXR] xrGetD3D12GraphicsRequirementsKHR called: instance=%p, systemId=%llu, req=%p",
         instance, (unsigned long long)systemId, req);
    if (!req) return XR_ERROR_VALIDATION_FAILURE;

    memset(req, 0, sizeof(*req));
    req->type = XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR;
    req->next = nullptr;

    Microsoft::WRL::ComPtr<IDXGIFactory1> f;
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(f.GetAddressOf()));
    if (FAILED(hr)) return XR_ERROR_RUNTIME_FAILURE;

    for (UINT i = 0;; ++i) {
        Microsoft::WRL::ComPtr<IDXGIAdapter1> a;
        if (f->EnumAdapters1(i, a.GetAddressOf()) == DXGI_ERROR_NOT_FOUND) break;
        DXGI_ADAPTER_DESC1 d{}; a->GetDesc1(&d);
        if (d.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
        req->adapterLuid = d.AdapterLuid;
        break;
    }
    req->minFeatureLevel = D3D_FEATURE_LEVEL_11_0;
    Log("[SimXR] xrGetD3D12GraphicsRequirementsKHR: SUCCESS");
    return XR_SUCCESS;
}
// xrGetD3D11GraphicsRequirementsKHR (XR_KHR_D3D11_enable)
static XrResult XRAPI_PTR xrGetD3D11GraphicsRequirementsKHR_runtime(
    XrInstance instance, XrSystemId systemId, XrGraphicsRequirementsD3D11KHR* req) {
    Logf("[SimXR] xrGetD3D11GraphicsRequirementsKHR called: instance=%p, systemId=%llu, req=%p",
         instance, (unsigned long long)systemId, req);
    if (!req) {
        Log("[SimXR] xrGetD3D11GraphicsRequirementsKHR: ERROR - null req");
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    Logf("[SimXR] xrGetD3D11GraphicsRequirementsKHR: req struct size = %zu, expected = %zu",
         sizeof(*req), sizeof(XrGraphicsRequirementsD3D11KHR));
    
    // Check if the struct type is already set (Unity might pre-fill it)
    if (req->type != 0) {
        Logf("[SimXR] xrGetD3D11GraphicsRequirementsKHR: req->type already set to %d", req->type);
    }
    
    // Zero initialize the entire structure first
    memset(req, 0, sizeof(XrGraphicsRequirementsD3D11KHR));
    req->type = XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR;
    req->next = nullptr;
    
    Microsoft::WRL::ComPtr<IDXGIFactory1> f;
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(f.GetAddressOf()));
    if (FAILED(hr)) { 
        Logf("[SimXR] CreateDXGIFactory1 failed: 0x%08X", hr); 
        return XR_ERROR_RUNTIME_FAILURE; 
    }
    
    Microsoft::WRL::ComPtr<IDXGIAdapter1> bestAdapter;
    DXGI_ADAPTER_DESC1 bestDesc{};
    bool foundHardware = false;
    
    // Find the best hardware adapter
    for (UINT i = 0; ; ++i) {
        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapt;
        if (f->EnumAdapters1(i, adapt.GetAddressOf()) == DXGI_ERROR_NOT_FOUND)
            break;
            
        DXGI_ADAPTER_DESC1 d{}; 
        adapt->GetDesc1(&d);
        
        // Skip software adapters
        if (d.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) 
            continue;
            
        // Use the first hardware adapter we find
        if (!foundHardware) {
            bestAdapter = adapt;
            bestDesc = d;
            foundHardware = true;
            
            wchar_t* descStr = d.Description;
            char descAscii[128];
            wcstombs(descAscii, descStr, sizeof(descAscii));
            descAscii[sizeof(descAscii)-1] = '\0';
            Logf("[SimXR] Found hardware adapter: %s", descAscii);
            Logf("[SimXR]   LUID: High=%ld, Low=%lu", 
                 (long)d.AdapterLuid.HighPart, 
                 (unsigned long)d.AdapterLuid.LowPart);
            Logf("[SimXR]   Dedicated Video Memory: %llu MB", 
                 (unsigned long long)(d.DedicatedVideoMemory / (1024*1024)));
        }
    }
    
    if (foundHardware) {
        req->adapterLuid = bestDesc.AdapterLuid;
        req->minFeatureLevel = D3D_FEATURE_LEVEL_11_0;
        
        // Save this LUID for later validation
        rt::g_adapterLuid = bestDesc.AdapterLuid;
        rt::g_adapterLuidSet = true;
        
        Logf("[SimXR] xrGetD3D11GraphicsRequirementsKHR: Returning:");
        Logf("[SimXR]   type = %d (expected %d)", req->type, XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR);
        Logf("[SimXR]   next = %p", req->next);
        Logf("[SimXR]   adapterLuid.HighPart = %ld (0x%08X)", 
             (long)req->adapterLuid.HighPart, (unsigned)req->adapterLuid.HighPart);
        Logf("[SimXR]   adapterLuid.LowPart = %lu (0x%08X)", 
             (unsigned long)req->adapterLuid.LowPart, (unsigned)req->adapterLuid.LowPart);
        Logf("[SimXR]   minFeatureLevel = 0x%X (D3D_FEATURE_LEVEL_11_0 = 0x%X)", 
             req->minFeatureLevel, D3D_FEATURE_LEVEL_11_0);
        
        Log("[SimXR] xrGetD3D11GraphicsRequirementsKHR: SUCCESS - Returning XR_SUCCESS");
        return XR_SUCCESS;
    }
    
    // No hardware adapter found, this is an error for VR
    Log("[SimXR] xrGetD3D11GraphicsRequirementsKHR: ERROR - No hardware graphics adapter found");
    return XR_ERROR_SYSTEM_INVALID;
}

// xrGetOpenGLGraphicsRequirementsKHR (XR_KHR_opengl_enable)
static XrResult XRAPI_PTR xrGetOpenGLGraphicsRequirementsKHR_runtime(
    XrInstance instance, XrSystemId systemId, XrGraphicsRequirementsOpenGLKHR* req) {
    Logf("[SimXR] xrGetOpenGLGraphicsRequirementsKHR called: instance=%p, systemId=%llu, req=%p",
         instance, (unsigned long long)systemId, req);
    if (!req) {
        Log("[SimXR] xrGetOpenGLGraphicsRequirementsKHR: ERROR - null req");
        return XR_ERROR_VALIDATION_FAILURE;
    }

    // Zero initialize the structure
    memset(req, 0, sizeof(XrGraphicsRequirementsOpenGLKHR));
    req->type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR;
    req->next = nullptr;

    // Minimum OpenGL version: 4.0.0 (good compatibility)
    // Maximum: 4.6.0 (latest)
    req->minApiVersionSupported = XR_MAKE_VERSION(4, 0, 0);
    req->maxApiVersionSupported = XR_MAKE_VERSION(4, 6, 0);

    Logf("[SimXR] xrGetOpenGLGraphicsRequirementsKHR: min=%d.%d.%d, max=%d.%d.%d",
         XR_VERSION_MAJOR(req->minApiVersionSupported),
         XR_VERSION_MINOR(req->minApiVersionSupported),
         XR_VERSION_PATCH(req->minApiVersionSupported),
         XR_VERSION_MAJOR(req->maxApiVersionSupported),
         XR_VERSION_MINOR(req->maxApiVersionSupported),
         XR_VERSION_PATCH(req->maxApiVersionSupported));

    Log("[SimXR] xrGetOpenGLGraphicsRequirementsKHR: SUCCESS");
    return XR_SUCCESS;
}

// --- Minimal implementations ---

static const char* kSupportedExtensions[] = {
    XR_KHR_D3D11_ENABLE_EXTENSION_NAME,
    XR_KHR_D3D12_ENABLE_EXTENSION_NAME,
    XR_KHR_OPENGL_ENABLE_EXTENSION_NAME,  // OpenGL support
    XR_KHR_VULKAN_ENABLE_EXTENSION_NAME,  // Vulkan support
    XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME, // Vulkan enable2
    XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME,
    XR_KHR_COMPOSITION_LAYER_CYLINDER_EXTENSION_NAME,  // UEVR uses this for UI layers
    XR_KHR_VISIBILITY_MASK_EXTENSION_NAME,
    XR_EXT_PALM_POSE_EXTENSION_NAME,
    "XR_KHR_win32_convert_performance_counter_time"    // Unity often requires this
};

static XrResult XRAPI_PTR xrEnumerateApiLayerProperties_runtime(uint32_t propertyCapacityInput,
                                                                uint32_t* propertyCountOutput,
                                                                XrApiLayerProperties* properties) {
    Log("[SimXR] xrEnumerateApiLayerProperties called");
    // Runtime doesn't provide API layers, only extensions
    if (propertyCountOutput) *propertyCountOutput = 0;
    return XR_SUCCESS;
}
static XrResult XRAPI_PTR xrEnumerateInstanceExtensionProperties_runtime(const char* layerName, uint32_t propertyCapacityInput,
                                                                         uint32_t* propertyCountOutput,
                                                                         XrExtensionProperties* properties) {
    if (layerName && layerName[0] != '\0') return XR_ERROR_LAYER_INVALID;
    const uint32_t count = (uint32_t)(sizeof(kSupportedExtensions)/sizeof(kSupportedExtensions[0]));
    if (propertyCountOutput) *propertyCountOutput = count;
    if (properties && propertyCapacityInput) {
        for (uint32_t i = 0; i < propertyCapacityInput && i < count; ++i) {
            properties[i].type = XR_TYPE_EXTENSION_PROPERTIES;
            properties[i].next = nullptr;
            std::strncpy(properties[i].extensionName, kSupportedExtensions[i], XR_MAX_EXTENSION_NAME_SIZE - 1);
            properties[i].extensionName[XR_MAX_EXTENSION_NAME_SIZE - 1] = '\0';
            properties[i].extensionVersion = 1;
            Logf("[SimXR] ext[%u]=%s", i, properties[i].extensionName);
        }
    }
    return XR_SUCCESS;
}

// ---- Vulkan extension functions ----

static PFN_vkGetInstanceProcAddr get_gipa(void)
{
    if (rt::g_app_vk_gipa) return rt::g_app_vk_gipa;
    static PFN_vkGetInstanceProcAddr loaded = NULL;
    if (!loaded) {
        HMODULE m = LoadLibraryA("vulkan-1.dll");
        if (m) loaded = (PFN_vkGetInstanceProcAddr)(void *)GetProcAddress(m, "vkGetInstanceProcAddr");
    }
    return loaded;
}

static XrResult XRAPI_PTR xrGetVulkanGraphicsRequirementsKHR_runtime(
    XrInstance instance, XrSystemId systemId, XrGraphicsRequirementsVulkanKHR* req) {
    Logf("[SimXR] xrGetVulkanGraphicsRequirementsKHR called: instance=%p, systemId=%llu",
         instance, (unsigned long long)systemId);
    if (!req) return XR_ERROR_VALIDATION_FAILURE;
    memset(req, 0, sizeof(*req));
    req->type = XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR;
    req->minApiVersionSupported = XR_MAKE_VERSION(1, 0, 0);
    req->maxApiVersionSupported = XR_MAKE_VERSION(1, 3, 0);
    Log("[SimXR] xrGetVulkanGraphicsRequirementsKHR: SUCCESS");
    return XR_SUCCESS;
}

// enable2 alias — identical requirements
static XrResult XRAPI_PTR xrGetVulkanGraphicsRequirements2KHR_runtime(
    XrInstance instance, XrSystemId systemId, XrGraphicsRequirementsVulkanKHR* req) {
    return xrGetVulkanGraphicsRequirementsKHR_runtime(instance, systemId, req);
}

static XrResult XRAPI_PTR xrGetVulkanInstanceExtensionsKHR_runtime(
    XrInstance h, XrSystemId id, uint32_t cap, uint32_t *count, char *buf) {
    return write_str("VK_KHR_get_physical_device_properties2", cap, count, buf);
}

static XrResult XRAPI_PTR xrGetVulkanDeviceExtensionsKHR_runtime(
    XrInstance h, XrSystemId id, uint32_t cap, uint32_t *count, char *buf) {
    return write_str("", cap, count, buf);
}

static XrResult XRAPI_PTR xrGetVulkanGraphicsDeviceKHR_runtime(
    XrInstance h, XrSystemId id, VkInstance vkInstance, VkPhysicalDevice *out) {

    PFN_vkGetInstanceProcAddr gipa = get_gipa();
    if (!gipa) return XR_ERROR_RUNTIME_FAILURE;
    PFN_vkEnumeratePhysicalDevices epd =
        (PFN_vkEnumeratePhysicalDevices)gipa(vkInstance, "vkEnumeratePhysicalDevices");
    if (!epd) return XR_ERROR_RUNTIME_FAILURE;
    uint32_t n = 1;
    VkPhysicalDevice dev = VK_NULL_HANDLE;
    epd(vkInstance, &n, &dev);            /* first GPU (the Adreno) */
    if (dev == VK_NULL_HANDLE) return XR_ERROR_RUNTIME_FAILURE;
    *out = dev;
    return XR_SUCCESS;
}

// Function pointer typedefs for enable2 pass-throughs
typedef VkResult (VKAPI_PTR *PFN_vkCreateInstance_t)(const VkInstanceCreateInfo*, const VkAllocationCallbacks*, VkInstance*);
typedef VkResult (VKAPI_PTR *PFN_vkCreateDevice_t)(VkPhysicalDevice, const VkDeviceCreateInfo*, const VkAllocationCallbacks*, VkDevice*);

// enable2: xrCreateVulkanInstanceKHR — call vkCreateInstance via app's proc addr
static XrResult XRAPI_PTR xrCreateVulkanInstanceKHR_runtime(
    XrInstance xrInstance, const XrVulkanInstanceCreateInfoKHR* createInfo,
    VkInstance* vulkanInstance, VkResult* vulkanResult) {
    if (!createInfo || !vulkanInstance || !vulkanResult) return XR_ERROR_VALIDATION_FAILURE;
    rt::g_app_vk_gipa = createInfo->pfnGetInstanceProcAddr;
    auto pfnCreateInstance = reinterpret_cast<PFN_vkCreateInstance_t>(
        createInfo->pfnGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance"));
    if (!pfnCreateInstance) return XR_ERROR_RUNTIME_FAILURE;
    *vulkanResult = pfnCreateInstance(createInfo->vulkanCreateInfo, createInfo->vulkanAllocator, vulkanInstance);
    Logf("[SimXR] xrCreateVulkanInstanceKHR: vkResult=%d", (int)*vulkanResult);
    return XR_SUCCESS;
}

// enable2: xrCreateVulkanDeviceKHR — call vkCreateDevice via app's proc addr
static XrResult XRAPI_PTR xrCreateVulkanDeviceKHR_runtime(
    XrInstance xrInstance, const XrVulkanDeviceCreateInfoKHR* createInfo,
    VkDevice* vulkanDevice, VkResult* vulkanResult) {
    if (!createInfo || !vulkanDevice || !vulkanResult) return XR_ERROR_VALIDATION_FAILURE;
    rt::g_app_vk_gipa = createInfo->pfnGetInstanceProcAddr;
    auto pfnCreateDevice = reinterpret_cast<PFN_vkCreateDevice_t>(createInfo->pfnGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateDevice"));
    if (!pfnCreateDevice) return XR_ERROR_RUNTIME_FAILURE;
    *vulkanResult = pfnCreateDevice(createInfo->vulkanPhysicalDevice, createInfo->vulkanCreateInfo,
                                    createInfo->vulkanAllocator, vulkanDevice);

    Logf("[SimXR] xrCreateVulkanDeviceKHR: vkResult=%d", (int)*vulkanResult);
    return XR_SUCCESS;
}

// enable2: xrGetVulkanGraphicsDevice2KHR — enumerate and pick best physical device
static XrResult XRAPI_PTR xrGetVulkanGraphicsDevice2KHR_runtime(
    XrInstance xrInstance, const XrVulkanGraphicsDeviceGetInfoKHR* getInfo, VkPhysicalDevice* physDevice) {
    if (!getInfo || !physDevice) return XR_ERROR_VALIDATION_FAILURE;
    return xrGetVulkanGraphicsDeviceKHR_runtime(xrInstance, getInfo->systemId, getInfo->vulkanInstance, physDevice);
}

// ---- End Vulkan extension functions ----

static XrResult XRAPI_PTR xrCreateInstance_runtime(const XrInstanceCreateInfo* createInfo, XrInstance* instance) {
    //----------------
    //OXRWXR CHANGE:
    //----------------
    // Do first time setup for this instance
    SetCursorPos(0, 0);
    ShowCursor(FALSE);

    Logf("[WinXrApi] Starting Up");
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    std::filesystem::path tmpPath = "Z:/";
    std::filesystem::path dirPath = "Z:/tmp/xr";
    std::filesystem::path confPath = "D:/Winlator/oxrwxr";
    std::filesystem::path confFile = confPath / "conf.txt";
    std::filesystem::path fallbackDir = "D:/xrtemp";
    std::filesystem::path filePath = dirPath / "vr";
    std::filesystem::path fallbackFile = fallbackDir / "vr";

    std::filesystem::path versionPath = dirPath / "version";
    std::filesystem::path fallbackVersion = fallbackDir / "version";

    if (std::filesystem::exists(tmpPath) && std::filesystem::is_directory(tmpPath))
    {
        if (std::filesystem::exists(dirPath) && std::filesystem::is_directory(dirPath))
        {
            // Nothing to do
        }
        else
        {
            // Try to create the folder
            try
            {
                std::filesystem::create_directories(dirPath);
            }
            catch (const std::exception& e)
            {
                Logf("[WinXrApi] Error creating tmp/xr directory: %s", e.what());
            }
        }

        try
        {
            // A host that can send the binary tracking packet says so on the seventh line of its
            // system file; an older one is asked for the text packet as before
            bool hostSendsBinary = false;
            {
                std::ifstream systemFile(dirPath / "system");
                std::string line;
                int lines = 0;
                while (lines < 7 && std::getline(systemFile, line)) lines++;
                hostSendsBinary = lines == 7 && line == "BINARY_UDP";
            }
            Logf("[WinXrApi] Asking the host for the %s tracking packet", hostSendsBinary ? "binary" : "text");

            std::ofstream verFile(versionPath);
            if (verFile.is_open())
            {
                verFile << (hostSendsBinary ? "0.7" : "0.6");
                verFile.close();
            }
        }
        catch (const std::exception& e)
        {
            Logf("[WinXrApi] Error writing VERSION file: %s", e.what());
        }

        try
        {
            std::ofstream file(filePath);
            if (file.is_open())
            {
                file << "VR";
                file.close();
            }
            else
            {
            }
        }
        catch (const std::exception& e)
        {
            Logf("[WinXrApi] Error writing VR file: %s", e.what());
        }
    }
    else
    {
        try
        {
            std::ofstream verFile(fallbackVersion);
            if (verFile.is_open())
            {
                verFile << "0.5";
                verFile.close();
            }
        }
        catch (const std::exception& e)
        {
            Logf("[WinXrApi] Error writing test VERSION file: %s", e.what());
        }

        try
        {
            std::ofstream file(fallbackFile);
            if (file.is_open())
            {
                file << "VR";
                file.close();
            }
            else
            {
            }
        }
        catch (const std::exception& e)
        {
            Logf("[WinXrApi] Error writing test VR file: %s", e.what());
        }

        dirPath = fallbackDir;
    }

    std::filesystem::path sysinfoPath = dirPath / "system";

    if (std::filesystem::exists(dirPath) && std::filesystem::is_directory(dirPath)) {
        if (std::filesystem::exists(sysinfoPath) && std::filesystem::is_regular_file(sysinfoPath)) {
            try {
                std::ifstream sysInfoFile(sysinfoPath);

                std::string hmdMakeStr;
                std::string hmdModelStr;
                std::string temp;

                if (sysInfoFile.is_open()) {
                    std::getline(sysInfoFile, hmdMakeStr);
                    std::getline(sysInfoFile, hmdModelStr);
                    std::getline(sysInfoFile, temp); //OS version
                    std::getline(sysInfoFile, temp); //Security patch
                    std::getline(sysInfoFile, temp); //Resolution
                    sscanf(temp.c_str(), "%dx%d", &ViewportWidth, &ViewportHeight);
                    if (std::getline(sysInfoFile, temp)) //Direct transport eye size, absent on older hosts
                        sscanf(temp.c_str(), "%dx%d", &DirectEyeWidth, &DirectEyeHeight);
                    sysInfoFile.close();
                }

                hmdMake = hmdMakeStr;
                hmdModel = hmdModelStr;
            }
            catch (const std::filesystem::filesystem_error& e) {

            }
            catch (const std::exception& e) {

            }
        }
    }

    if (hmdMake.empty()) {
        hmdMake = "META";
    }
    else if (hmdMake == "OCULUS") {
        hmdMake = "META";
    }

    if (hmdModel.empty()) {
        hmdModel = "QUEST 3";
    }
    else if (hmdModel == "EUREKA" || hmdModel == "PANTHER") {
        hmdModel = "QUEST 3";
    }
    else if (hmdMake == "META") {
        //SEACLIFF - Quest Pro - Untested, assuming hands upside down
        //HOLLYWOOD - Quest 2 - Works! Hands upside down, performance is OK (much better with Turnip driver)
        //MONTEREY - Quest 1 - Untested, unsupported
        hmdModel = "QUEST 2";
    }

    //Look for the flag to force hand fix ON always:
    std::filesystem::path handFixFile = confPath / "handsfix.txt";

    if (std::filesystem::exists(handFixFile) && std::filesystem::is_regular_file(handFixFile)) {
        hmdMake = "FORCE FIX HANDS";
    }

    //Load the config for the SimXR runtime
    if (std::filesystem::exists(confPath) && std::filesystem::is_directory(confPath)) {
        if (std::filesystem::exists(confFile) && std::filesystem::is_regular_file(confFile)) {
            try {
                std::ifstream confFileOpen(confFile);

                std::string line;
                while (std::getline(confFileOpen, line)) {
                    if (compareKey(line, "disable_haptics")) {
                        sendHaptics = !parseBool(line);
                    }
                    if (compareKey(line, "disable_2d_layer")) {
                        disable2DLayer = parseBool(line);
                    }
                    if (compareKey(line, "disable_right_eye")) {
                        disableRightEye = parseBool(line);
                    }
                    if (compareKey(line, "disable_window")) {
                        disableWindow = parseBool(line);
                    }
                    if (compareKey(line, "legacy_input_matching")) {
                        legacyInputMatching = parseBool(line);
                    }
                    if (compareKey(line, "mono_rendering")) {
                        monoRendering = parseBool(line);
                    }
                    if (compareKey(line, "verbose_logging")) {
                        verboseLogging = parseBool(line);
                    }
                    if (compareKey(line, "direct_transport")) {
                        directTransport = parseBool(line);
                    }
                    if (compareKey(line, "controller_profile")) {
                        preferredProfile = line.substr(line.find('=') + 1);
                    }
                }

                confFileOpen.close();
            }
            catch (const std::filesystem::filesystem_error& e) {

            }
            catch (const std::exception& e) {

            }
        }
    }

    Logf("[WinXrUDP] Starting UDP");
    udpReader = new WinXrApiUDP();
    if (!udpReader->HasSocket()) Log("[WinXrUDP] ERROR - could not bind the UDP port, is another VR process already running? No tracking data will arrive");
    udpReader->SendData("0 0 2 0");

    if (!createInfo || !instance) return XR_ERROR_VALIDATION_FAILURE;
    // applicationName may not be null-terminated
    char appName[XR_MAX_APPLICATION_NAME_SIZE + 1] = {0};
    memcpy(appName, createInfo->applicationInfo.applicationName, XR_MAX_APPLICATION_NAME_SIZE);
    Logf("[SimXR] xrCreateInstance: app=%s version=%u", 
         appName,
         createInfo->applicationInfo.applicationVersion);
    rt::g_instance = {};
    rt::g_instance.enabledExtensions.clear();

    XrVersion apiVersion = createInfo->applicationInfo.apiVersion;
    if (XR_VERSION_MAJOR(apiVersion) > 1 || (XR_VERSION_MAJOR(apiVersion) == 1 && XR_VERSION_MINOR(apiVersion) > 1)) {
        Logf("[SimXR] xrCreateInstance: ERROR - Unsupported OpenXR version %u.%u",
             (unsigned)XR_VERSION_MAJOR(apiVersion), (unsigned)XR_VERSION_MINOR(apiVersion));
        return XR_ERROR_API_VERSION_UNSUPPORTED;
    }
    rt::g_instance.apiVersion = apiVersion;
    
    // Validate that all requested extensions are supported
    const uint32_t supportedCount = (uint32_t)(sizeof(kSupportedExtensions)/sizeof(kSupportedExtensions[0]));
    for (uint32_t i = 0; i < createInfo->enabledExtensionCount; ++i) {
        bool supported = false;
        for (uint32_t j = 0; j < supportedCount; ++j) {
            if (strcmp(createInfo->enabledExtensionNames[i], kSupportedExtensions[j]) == 0) {
                supported = true;
                break;
            }
        }
        if (!supported) {
            Logf("[SimXR] xrCreateInstance: ERROR - Unsupported extension %s", createInfo->enabledExtensionNames[i]);
            return XR_ERROR_EXTENSION_NOT_PRESENT;
        }
        rt::g_instance.enabledExtensions.emplace_back(createInfo->enabledExtensionNames[i]);
        Logf("[SimXR]   enabledExt[%u]=%s", i, createInfo->enabledExtensionNames[i]);
    }
    rt::g_instance.handle = (XrInstance)1;  // Set a valid handle
    *instance = rt::g_instance.handle;
    Log("[SimXR] xrCreateInstance: SUCCESS");
    return XR_SUCCESS;
}

static void StopBridgeSender();

static XrResult XRAPI_PTR xrDestroyInstance_runtime(XrInstance instance) {
    StopBridgeSender();  // before the loader can unload this DLL
    //----------------
    //OXRWXR CHANGE:
    //----------------
    // Shutdown XrAPI + UDP
    try
    {
        Logf("[WinXrUDP] Shutting Down UDP");
        if (udpReader) {
            udpReader->KillReceiver();
            udpReader = nullptr;
        }
    }
    catch (const std::exception& e)
    {
        Logf("[WinXrUDP] Error killing UDP receiver: %s", e.what());
    }

    Logf("[WinXrApi] Shutting Down");

    Logf("[SimXR] xrDestroyInstance called: instance=%p", instance);

    // Clear the global instance
    if (instance == rt::g_instance.handle) {
        Log("[SimXR] xrDestroyInstance: Clearing global instance");
        rt::g_instance = {};

        // MUST destroy the window before DLL unloads!
        // The OpenXR loader may unload our DLL after this call.
        // If the window stays alive, its WndProc points to unloaded code = crash.
        {
            std::lock_guard<std::mutex> lock(rt::g_windowMutex);
            if (rt::g_persistentWindow) {
                Log("[SimXR] xrDestroyInstance: Destroying preview window");
                DestroyWindow(rt::g_persistentWindow);
                rt::g_persistentWindow = nullptr;
            }
            rt::g_persistentSwapchain.Reset();
        }
        // Also clear session window reference
        if (rt::g_session.hwnd) {
            rt::g_session.hwnd = nullptr;
        }

        // Unregister window class so it doesn't have dangling WndProc
        if (rt::g_windowClassRegistered) {
            UnregisterClassW(L"OpenXR Simulator", GetModuleHandleW(nullptr));
            rt::g_windowClassRegistered = false;
            Log("[SimXR] xrDestroyInstance: Window class unregistered");
        }
        Log("[SimXR] xrDestroyInstance: Window destroyed for safe DLL unload");
    }

    Log("[SimXR] xrDestroyInstance: SUCCESS - Returning XR_SUCCESS");
    Log("[SimXR] ========== Instance Destroyed - Waiting for new instance ==========");
    return XR_SUCCESS;
}

static XrResult XRAPI_PTR xrGetInstanceProperties_runtime(XrInstance, XrInstanceProperties* props) {
    if (!props) return XR_ERROR_VALIDATION_FAILURE;
    props->type = XR_TYPE_INSTANCE_PROPERTIES;
    props->next = nullptr;
    props->runtimeVersion = XR_MAKE_VERSION(1, 0, 27);
    strncpy(props->runtimeName, "OpenXR Simulator Runtime", XR_MAX_RUNTIME_NAME_SIZE - 1);
    props->runtimeName[XR_MAX_RUNTIME_NAME_SIZE - 1] = '\0';
    Log("[SimXR] xrGetInstanceProperties: returning OpenXR Simulator Runtime");
    return XR_SUCCESS;
}

static XrResult XRAPI_PTR xrGetSystem_runtime(XrInstance, const XrSystemGetInfo* info, XrSystemId* systemId) {
    if (!info || !systemId) return XR_ERROR_VALIDATION_FAILURE;
    Logf("[SimXR] xrGetSystem: formFactor=%d", info->formFactor);
    if (info->formFactor != XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY) {
        Log("[SimXR] xrGetSystem: ERROR - form factor not HMD");
        return XR_ERROR_FORM_FACTOR_UNSUPPORTED;
    }
    *systemId = (XrSystemId)1; 
    Log("[SimXR] xrGetSystem: SUCCESS -> systemId=1"); 
    return XR_SUCCESS;
}

static XrResult XRAPI_PTR xrGetSystemProperties_runtime(XrInstance, XrSystemId, XrSystemProperties* props) {
    if (!props) return XR_ERROR_VALIDATION_FAILURE;
    props->type = XR_TYPE_SYSTEM_PROPERTIES;
    props->next = nullptr;
    strncpy(props->systemName, "OpenXR Simulator", XR_MAX_SYSTEM_NAME_SIZE - 1);
    props->systemName[XR_MAX_SYSTEM_NAME_SIZE - 1] = '\0';
    props->systemId = 1;
    props->vendorId = 0;  // 0 = unknown vendor (more standard than 0xFFFF)
    props->graphicsProperties.maxSwapchainImageWidth = 4096;
    props->graphicsProperties.maxSwapchainImageHeight = 4096;
    props->graphicsProperties.maxLayerCount = 16;
    props->trackingProperties.positionTracking = XR_TRUE;
    props->trackingProperties.orientationTracking = XR_TRUE;
    Log("[SimXR] xrGetSystemProperties: returning OpenXR Simulator");
    return XR_SUCCESS;
}

static XrResult XRAPI_PTR xrEnumerateViewConfigurations_runtime(XrInstance, XrSystemId, uint32_t capacity, uint32_t* count, XrViewConfigurationType* types) {
    Logf("[SimXR] xrEnumerateViewConfigurations called: capacity=%u", capacity);
    if (count) *count = 1;
    if (capacity >= 1 && types) {
        types[0] = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        Log("[SimXR] xrEnumerateViewConfigurations: Returning PRIMARY_STEREO");
    }
    return XR_SUCCESS;
}

static XrResult XRAPI_PTR xrEnumerateViewConfigurationViews_runtime(XrInstance, XrSystemId, XrViewConfigurationType viewType, uint32_t capacity, uint32_t* count, XrViewConfigurationView* views) {
    if (verboseLogging) Logf("[SimXR] xrEnumerateViewConfigurationViews called: viewType=%d, capacity=%u", (int)viewType, capacity);
    if (count) *count = 2;
    if (capacity >= 2 && views) {
        // Direct frames skip the preview window, so they can be the headset's own size instead of half the screen
        bool direct = directTransport && DirectEyeWidth > 0 && DirectEyeHeight > 0 && BridgeReady();
        static bool logged = false;
        if (!logged) {
            Logf("[SimXR] Recommended eye size %dx%d (%s)", direct ? DirectEyeWidth : ViewportWidth / 2,
                 direct ? DirectEyeHeight : ViewportHeight, direct ? "direct transport" : "half the screen");
            logged = true;
        }
        for (uint32_t i = 0; i < 2; ++i) {
            views[i].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
            views[i].next = nullptr;
            views[i].recommendedImageRectWidth = direct ? DirectEyeWidth : monoRendering ? ViewportWidth : ViewportWidth / 2;
            views[i].recommendedImageRectHeight = direct ? DirectEyeHeight : ViewportHeight;
            views[i].recommendedSwapchainSampleCount = 1;
            views[i].maxImageRectWidth = 4096; views[i].maxImageRectHeight = 4096; views[i].maxSwapchainSampleCount = 1;
        }
        if (verboseLogging) Log("[SimXR] xrEnumerateViewConfigurationViews: Returned 2 views");
    }
    return XR_SUCCESS;
}

static XrResult XRAPI_PTR xrEnumerateEnvironmentBlendModes_runtime(
    XrInstance, XrSystemId, XrViewConfigurationType, uint32_t capacity, uint32_t* count, XrEnvironmentBlendMode* modes) {
    if (count) *count = 1;
    if (capacity >= 1 && modes) modes[0] = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    return XR_SUCCESS;
}

static XrResult XRAPI_PTR xrCreateSession_runtime(XrInstance instance, const XrSessionCreateInfo* info, XrSession* session) {
    static int sessionCount = 0;
    sessionCount++;
    Log("[SimXR] ============================================");
    Logf("[SimXR] xrCreateSession called (call #%d, instance=%llu)", sessionCount, (unsigned long long)instance);
    Log("[SimXR] ============================================");
    if (!info || !session) return XR_ERROR_VALIDATION_FAILURE;
    
    // Check if we already have an active session
    if (rt::g_session.handle != XR_NULL_HANDLE && rt::g_session.state != XR_SESSION_STATE_IDLE) {
        Logf("[SimXR] xrCreateSession: ERROR - Session already exists (handle=%llu, state=%d)",
            (unsigned long long)rt::g_session.handle, rt::g_session.state);
        // For now, reset the existing session to allow the new one
        // Reset session manually
        rt::g_session.handle = XR_NULL_HANDLE;
        rt::g_session.state = XR_SESSION_STATE_IDLE;
        rt::g_session.d3d11Device.Reset();
        rt::g_session.d3d11Context.Reset();
        rt::g_session.previewSwapchain.Reset();
        rt::g_session.usesD3D12 = false;
        rt::g_session.d3d12Device.Reset();
        rt::g_session.d3d12Queue.Reset();
        rt::ResetD3D12PreviewResources(rt::g_session);
        rt::g_session.previewWidth = ViewportWidth;
        rt::g_session.previewHeight = ViewportHeight;
        rt::g_session.isFocused = false;
    }
    // Accept D3D11 and D3D12
    const XrBaseInStructure* entry = reinterpret_cast<const XrBaseInStructure*>(info->next);
    while (entry) {
        if (entry->type == XR_TYPE_GRAPHICS_BINDING_D3D11_KHR) {
            const auto* b = reinterpret_cast<const XrGraphicsBindingD3D11KHR*>(entry);
            
            // Log the device details
            ComPtr<IDXGIDevice> dxgiDevice;
            if (SUCCEEDED(b->device->QueryInterface(IID_PPV_ARGS(&dxgiDevice)))) {
                ComPtr<IDXGIAdapter> adapter;
                if (SUCCEEDED(dxgiDevice->GetAdapter(&adapter))) {
                    DXGI_ADAPTER_DESC desc;
                    adapter->GetDesc(&desc);
                    Logf("[SimXR] xrCreateSession: App D3D11 device LUID=%llu/%llu", 
                         (unsigned long long)desc.AdapterLuid.HighPart,
                         (unsigned long long)desc.AdapterLuid.LowPart);
                }
            }
            
            // Use sessionCount to generate unique handles
            rt::g_session.handle = (XrSession)(uintptr_t)(0x1000 + sessionCount);
            rt::g_session.d3d11Device = b->device;
            rt::g_session.usesD3D12 = false;
            rt::g_session.d3d12Device.Reset();
            rt::g_session.d3d12Queue.Reset();
            rt::ResetD3D12PreviewResources(rt::g_session);
            rt::g_session.state = XR_SESSION_STATE_IDLE;
            b->device->GetImmediateContext(rt::g_session.d3d11Context.GetAddressOf());
            // Window will be created lazily on first frame
            *session = rt::g_session.handle;
            Logf("[SimXR] xrCreateSession: SUCCESS (D3D11, handle=%llu)", (unsigned long long)rt::g_session.handle);
            // Push READY event into queue
            rt::PushState(rt::g_session.handle, XR_SESSION_STATE_READY);
            return XR_SUCCESS;
        } else if (entry->type == XR_TYPE_GRAPHICS_BINDING_D3D12_KHR) {
            const auto* b12 = reinterpret_cast<const XrGraphicsBindingD3D12KHR*>(entry);
            rt::g_session.usesD3D12 = true;
            rt::g_session.d3d12Device = b12->device;
            rt::g_session.d3d12Queue = b12->queue;
            rt::g_session.d3d11Device.Reset();
            rt::g_session.d3d11Context.Reset();
            rt::g_session.previewSwapchain.Reset();
            rt::g_session.handle = (XrSession)(uintptr_t)(0x1000 + sessionCount);
            *session = rt::g_session.handle;
            Logf("[SimXR] xrCreateSession: SUCCESS (D3D12, handle=%llu)", (unsigned long long)rt::g_session.handle);
            rt::PushState(rt::g_session.handle, XR_SESSION_STATE_READY);
            return XR_SUCCESS;
        } else if (entry->type == XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR) {
            const auto* bGL = reinterpret_cast<const XrGraphicsBindingOpenGLWin32KHR*>(entry);
            rt::g_session.usesOpenGL = true;
            rt::g_session.usesD3D12 = false;
            rt::g_session.glDC = bGL->hDC;
            rt::g_session.glRC = bGL->hGLRC;
            rt::g_session.d3d11Device.Reset();
            rt::g_session.d3d11Context.Reset();
            rt::g_session.d3d12Device.Reset();
            rt::g_session.d3d12Queue.Reset();
            rt::g_session.previewSwapchain.Reset();
            rt::g_session.handle = (XrSession)(uintptr_t)(0x1000 + sessionCount);
            *session = rt::g_session.handle;
            Logf("[SimXR] xrCreateSession: SUCCESS (OpenGL, handle=%llu, hDC=%p, hGLRC=%p)",
                 (unsigned long long)rt::g_session.handle, bGL->hDC, bGL->hGLRC);
            rt::PushState(rt::g_session.handle, XR_SESSION_STATE_READY);
            return XR_SUCCESS;
        } else if (entry->type == XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR) {
            const auto* bVK = reinterpret_cast<const XrGraphicsBindingVulkanKHR*>(entry);
            rt::g_session.usesVulkan = true;
            rt::g_session.usesD3D12 = false;
            rt::g_session.usesOpenGL = false;
            rt::g_session.vkInstance = bVK->instance;
            rt::g_session.vkPhysicalDevice = bVK->physicalDevice;
            rt::g_session.vkDevice = bVK->device;
            rt::g_session.vkQueueFamilyIndex = bVK->queueFamilyIndex;
            rt::g_session.vkQueueIndex = bVK->queueIndex;
            rt::g_session.d3d11Device.Reset();
            rt::g_session.d3d11Context.Reset();
            rt::g_session.d3d12Device.Reset();
            rt::g_session.d3d12Queue.Reset();
            rt::g_session.previewSwapchain.Reset();
            // Retrieve VkQueue
            vkGetDeviceQueue(bVK->device, bVK->queueFamilyIndex, bVK->queueIndex, &rt::g_session.vkQueue);
            // Create a command pool for readback operations
            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.queueFamilyIndex = bVK->queueFamilyIndex;
            poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            vkCreateCommandPool(bVK->device, &poolInfo, nullptr, &rt::g_session.vkCmdPool);
            rt::g_session.handle = (XrSession)(uintptr_t)(0x1000 + sessionCount);
            *session = rt::g_session.handle;
            Logf("[SimXR] xrCreateSession: SUCCESS (Vulkan, handle=%llu)", (unsigned long long)rt::g_session.handle);
            rt::PushState(rt::g_session.handle, XR_SESSION_STATE_READY);
            return XR_SUCCESS;
        }
        entry = entry->next;
    }
    Log("[SimXR] xrCreateSession: ERROR - No supported graphics binding found (D3D11/D3D12/OpenGL)");
    return XR_ERROR_GRAPHICS_DEVICE_INVALID;
}


static XrResult XRAPI_PTR xrDestroySession_runtime(XrSession s) {
    Logf("[SimXR] xrDestroySession called (handle=%llu)", (unsigned long long)s);
    if (s != rt::g_session.handle) {
        Logf("[SimXR] xrDestroySession: ERROR - Invalid handle (expected %llu)", 
             (unsigned long long)rt::g_session.handle);
        return XR_ERROR_HANDLE_INVALID;
    }
    
    // Transfer window and swapchain to global persistent storage
    // Unity likes to create/destroy sessions rapidly for compatibility checks
    {
        std::lock_guard<std::mutex> lock(rt::g_windowMutex);
        if (rt::g_session.hwnd && !rt::g_persistentWindow) {
            rt::g_persistentWindow = rt::g_session.hwnd;
            rt::g_persistentSwapchain = rt::g_session.previewSwapchain;
            Log("[SimXR] xrDestroySession: Preserving window and swapchain for next session");
        } else if (rt::g_session.hwnd == rt::g_persistentWindow) {
            // Already using persistent window, just update the swapchain
            rt::g_persistentSwapchain = rt::g_session.previewSwapchain;
            Log("[SimXR] xrDestroySession: Updating persistent swapchain");
        }
    }
    
    // Reset session but don't destroy the window
    rt::g_session.handle = XR_NULL_HANDLE;
    rt::g_session.state = XR_SESSION_STATE_IDLE;
    rt::g_session.d3d11Device.Reset();
    rt::g_session.d3d11Context.Reset();
    rt::g_session.usesD3D12 = false;
    rt::g_session.d3d12Device.Reset();
    rt::g_session.d3d12Queue.Reset();
    rt::ResetD3D12PreviewResources(rt::g_session);
    // Reset OpenGL state
    rt::g_session.usesOpenGL = false;
    rt::g_session.glDC = nullptr;
    rt::g_session.glRC = nullptr;
    // Reset Vulkan state
    if (rt::g_session.usesVulkan) {
        if (rt::g_session.vkCmdPool != VK_NULL_HANDLE && rt::g_session.vkDevice != VK_NULL_HANDLE) {
            vkDestroyCommandPool(rt::g_session.vkDevice, rt::g_session.vkCmdPool, nullptr);
        }
        rt::g_session.vkCmdPool = VK_NULL_HANDLE;
        rt::g_session.vkQueue = VK_NULL_HANDLE;
        rt::g_session.vkDevice = VK_NULL_HANDLE;
        rt::g_session.vkPhysicalDevice = VK_NULL_HANDLE;
        rt::g_session.vkInstance = VK_NULL_HANDLE;
        rt::g_session.vkQueueFamilyIndex = 0;
        rt::g_session.vkQueueIndex = 0;
        rt::g_session.usesVulkan = false;
    }
    rt::g_session.hwnd = nullptr;  // Clear from session but window still exists
    rt::g_session.previewWidth = ViewportWidth;
    rt::g_session.previewHeight = ViewportHeight;
    rt::g_session.isFocused = false;
    rt::g_referenceSpacePose.clear();
    rt::g_referenceSpaceType.clear();
    {
        std::lock_guard<std::mutex> lock(rt::g_controllerMutex);
        rt::g_controllerSpaces.clear();
        rt::g_controllerGrips.clear();
        rt::g_controllerPoses.clear();
        rt::g_controllerInfo.clear();
    }
    rt::g_locateSpaces.clear();
    SpacePoses.clear();
    SpaceVelocities.clear();
    rt::g_spacesChanged = true;
    rt::g_sessionWasVR = false;
    Log("[SimXR] xrDestroySession: SUCCESS");
    return XR_SUCCESS;
}

static XrResult XRAPI_PTR xrEnumerateSwapchainFormats_runtime(XrSession, uint32_t capacity, uint32_t* count, int64_t* formats) {
    // OpenGL path - return GL internal formats
    if (rt::g_session.usesOpenGL) {
        const int64_t supportedFormats[] = {
            GL_SRGB8_ALPHA8,          // sRGB
            GL_RGBA8,                 // Standard RGBA
            GL_RGBA16F,               // HDR format
            GL_RGBA32F,               // High precision
            GL_RGB10_A2,              // HDR10 format
            GL_DEPTH_COMPONENT32F,    // Depth buffer
            GL_DEPTH24_STENCIL8,      // Depth + stencil
            GL_DEPTH_COMPONENT16      // 16-bit depth
        };
        const uint32_t formatCount = sizeof(supportedFormats) / sizeof(supportedFormats[0]);

        if (count) *count = formatCount;
        if (capacity > 0 && formats) {
            uint32_t copyCount = (capacity < formatCount) ? capacity : formatCount;
            for (uint32_t i = 0; i < copyCount; ++i) {
                formats[i] = supportedFormats[i];
            }
            Logf("[SimXR] xrEnumerateSwapchainFormats(OpenGL): Returned %u formats (first: 0x%X)", copyCount, (int)formats[0]);
        }
        return XR_SUCCESS;
    }

    // Vulkan path - return VkFormat values
    if (rt::g_session.usesVulkan) {
        const int64_t supportedFormats[] = {
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_FORMAT_B8G8R8A8_UNORM,
            VK_FORMAT_B8G8R8A8_SRGB,
            VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_FORMAT_A2B10G10R10_UNORM_PACK32,
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D24_UNORM_S8_UINT,
            VK_FORMAT_D16_UNORM,
        };
        const uint32_t formatCount = sizeof(supportedFormats) / sizeof(supportedFormats[0]);
        if (count) *count = formatCount;
        if (capacity > 0 && formats) {
            uint32_t copyCount = (capacity < formatCount) ? capacity : formatCount;
            for (uint32_t i = 0; i < copyCount; ++i) formats[i] = supportedFormats[i];
            Logf("[SimXR] xrEnumerateSwapchainFormats(Vulkan): Returned %u formats", copyCount);
        }
        return XR_SUCCESS;
    }

    // D3D11/D3D12 path - return DXGI formats
    const int64_t supportedFormats[] = {
        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,  // Unity often prefers sRGB
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,  // UEVR uses this
        DXGI_FORMAT_B8G8R8A8_UNORM,
        DXGI_FORMAT_R16G16B16A16_FLOAT,   // HDR format
        DXGI_FORMAT_R32G32B32A32_FLOAT,   // High precision
        DXGI_FORMAT_R10G10B10A2_UNORM,    // HDR10 format
        DXGI_FORMAT_D32_FLOAT_S8X24_UINT, // UEVR's preferred depth format
        DXGI_FORMAT_D32_FLOAT,            // Depth buffer
        DXGI_FORMAT_D24_UNORM_S8_UINT,    // Depth + stencil
        DXGI_FORMAT_D16_UNORM             // 16-bit depth
    };
    const uint32_t formatCount = sizeof(supportedFormats) / sizeof(supportedFormats[0]);

    if (count) *count = formatCount;
    if (capacity > 0 && formats) {
        uint32_t copyCount = (capacity < formatCount) ? capacity : formatCount;
        for (uint32_t i = 0; i < copyCount; ++i) {
            formats[i] = supportedFormats[i];
        }
        Logf("[SimXR] xrEnumerateSwapchainFormats: Returned %u formats (first: %d)", copyCount, (int)formats[0]);
    }
    return XR_SUCCESS;
}

// Direct transport: D3D11 swapchain images are shared with wxr_bridge, a Wine unixlib that
// copies them into AHardwareBuffers for the host, instead of reaching it through the preview window.
typedef LONG (WINAPI* PFN_WxrBridgeCall)(unsigned int, void*);
static PFN_WxrBridgeCall g_bridgeCall = nullptr;
static std::mutex g_bridgeMutex;  // an import and a present both write to the bridge's host socket

static bool BridgeReady() {
    static bool tried = false;
    if (tried) return g_bridgeCall != nullptr;
    tried = true;

    HMODULE module = LoadLibraryA("wxr_bridge.dll");
    PFN_WxrBridgeCall call = module ? (PFN_WxrBridgeCall)GetProcAddress(module, "WxrBridgeCall") : nullptr;
    if (!call) {
        Logf("[SimXR] direct_transport: wxr_bridge.dll unavailable (error %lu)", GetLastError());
        return false;
    }
    wxr_bridge_init_args init{};
    init.abi = WXR_BRIDGE_ABI;
    LONG status = call(WXR_BRIDGE_INIT, &init);
    if (status || init.result) {
        Logf("[SimXR] direct_transport: bridge init failed, status 0x%08lX: %s", (unsigned long)status, init.message);
        return false;
    }
    Logf("[SimXR] direct_transport: relay device %s", init.message);
    g_bridgeCall = call;
    return true;
}

// The VkFormat DXVK gives a texture of this family (typeless maps to the UNORM member), or 0 if the bridge can't take it.
static uint32_t BridgeVkFormat(DXGI_FORMAT format) {
    switch (format) {
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return VK_FORMAT_R8G8B8A8_UNORM;
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return VK_FORMAT_B8G8R8A8_UNORM;
        default: return 0;
    }
}

static uint32_t BridgeImport(ID3D11Texture2D* texture, const D3D11_TEXTURE2D_DESC& td, uint32_t vkFormat, uint32_t flags) {
    ComPtr<IDXGIResource1> resource;
    HANDLE shared = nullptr;
    HRESULT hr = texture->QueryInterface(IID_PPV_ARGS(&resource));
    if (SUCCEEDED(hr)) hr = resource->CreateSharedHandle(nullptr, DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE, nullptr, &shared);
    if (FAILED(hr) || !shared) {
        Logf("[SimXR] direct_transport: CreateSharedHandle failed, hr=0x%08X", (unsigned)hr);
        return UINT32_MAX;
    }
    wxr_bridge_import_args import{};
    import.nt_handle = (uint64_t)(uintptr_t)shared;
    import.width = td.Width;
    import.height = td.Height;
    import.layers = td.ArraySize;
    import.vk_format = vkFormat;
    import.flags = flags;
    {
        std::lock_guard<std::mutex> lock(g_bridgeMutex);
        g_bridgeCall(WXR_BRIDGE_IMPORT, &import);
    }
    CloseHandle(shared);  // the bridge holds its own fd now
    Logf("[SimXR] direct_transport: import %s: %s", import.result ? "FAILED" : "ok", import.message);
    return import.result ? UINT32_MAX : import.id;
}

static XrResult XRAPI_PTR xrCreateSwapchain_runtime(XrSession, const XrSwapchainCreateInfo* ci, XrSwapchain* sc) {
    Log("[SimXR] ============================================");
    Logf("[SimXR] xrCreateSwapchain called: format=%d, size=%ux%u, arraySize=%u, mipCount=%u, sampleCount=%u, usageFlags=0x%X",
         ci ? (int)ci->format : -1, 
         ci ? ci->width : 0, 
         ci ? ci->height : 0, 
         ci ? ci->arraySize : 0, 
         ci ? ci->mipCount : 0, 
         ci ? ci->sampleCount : 0,
         ci ? ci->usageFlags : 0);
    
    // Log specific usage flags
    if (ci && ci->usageFlags) {
        if (ci->usageFlags & XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT) 
            Log("[SimXR]   - COLOR_ATTACHMENT");
        if (ci->usageFlags & XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) 
            Log("[SimXR]   - DEPTH_STENCIL_ATTACHMENT");
        if (ci->usageFlags & XR_SWAPCHAIN_USAGE_UNORDERED_ACCESS_BIT) 
            Log("[SimXR]   - UNORDERED_ACCESS");
        if (ci->usageFlags & XR_SWAPCHAIN_USAGE_TRANSFER_SRC_BIT) 
            Log("[SimXR]   - TRANSFER_SRC");
        if (ci->usageFlags & XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT) 
            Log("[SimXR]   - TRANSFER_DST");
        if (ci->usageFlags & XR_SWAPCHAIN_USAGE_SAMPLED_BIT) 
            Log("[SimXR]   - SAMPLED");
        if (ci->usageFlags & XR_SWAPCHAIN_USAGE_MUTABLE_FORMAT_BIT) 
            Log("[SimXR]   - MUTABLE_FORMAT");
    }
    
    Log("[SimXR] ============================================");
    if (!ci || !sc) return XR_ERROR_VALIDATION_FAILURE;
    rt::Swapchain chain{}; 
    chain.handle = (XrSwapchain)(rt::g_nextSwapchainHandle++);
    chain.format = (DXGI_FORMAT)ci->format;  // Store the original requested format
    chain.width = ci->width; 
    chain.height = ci->height; 
    chain.arraySize = ci->arraySize ? ci->arraySize : 1;
    chain.lastAcquired = UINT32_MAX;  // No image acquired yet
    chain.lastReleased = UINT32_MAX;  // No image released yet
    // Only D3D11 images reach the bridge, so a game rendering through anything else keeps the
    // preview window, and has to be told the window's eye size instead of the headset's, or it
    // renders at a size the window never matches. OpenComposite re-reads the view configuration
    // when it rebuilds its session, so the size corrects itself on the next one.
    if (directTransport && (rt::g_session.usesOpenGL || rt::g_session.usesD3D12)) {
        directTransport = false;
        Log("[SimXR] direct_transport: the game does not render through D3D11, using the preview window");
    }

    // Create textures on appropriate backend
    if (rt::g_session.usesD3D12) {
        chain.backend = rt::Swapchain::Backend::D3D12;
        chain.imageCount = 3;
        for (uint32_t i = 0; i < chain.imageCount; ++i) {
            D3D12_RESOURCE_DESC rd = {};
            rd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            rd.Alignment = 0;
            rd.Width = chain.width;
            rd.Height = chain.height;
            rd.DepthOrArraySize = (UINT)chain.arraySize;
            rd.MipLevels = (UINT)(ci->mipCount ? ci->mipCount : 1);
            chain.mipCount = rd.MipLevels;
            rd.Format = chain.format;
            rd.SampleDesc.Count = (UINT)(ci->sampleCount ? ci->sampleCount : 1);
            rd.SampleDesc.Quality = 0;
            rd.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            rd.Flags = D3D12_RESOURCE_FLAG_NONE;
            if (!(chain.format == DXGI_FORMAT_D32_FLOAT || chain.format == DXGI_FORMAT_D24_UNORM_S8_UINT || chain.format == DXGI_FORMAT_D16_UNORM)) {
                if (ci->usageFlags & XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT)
                    rd.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
                if (ci->usageFlags & XR_SWAPCHAIN_USAGE_UNORDERED_ACCESS_BIT)
                    rd.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            }
            D3D12_HEAP_PROPERTIES hp = {}; hp.Type = D3D12_HEAP_TYPE_DEFAULT;
            D3D12_RESOURCE_STATES init =
                (rd.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
                    ? D3D12_RESOURCE_STATE_RENDER_TARGET
                    : D3D12_RESOURCE_STATE_COMMON;
            ComPtr<ID3D12Resource> res;
            HRESULT hr = rt::g_session.d3d12Device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd, init, nullptr, IID_PPV_ARGS(res.GetAddressOf()));
            if (FAILED(hr)) {
                Logf("[SimXR] CreateCommittedResource(D3D12)[%u] FAILED: 0x%08X", i, (unsigned)hr);
                return XR_ERROR_RUNTIME_FAILURE;
            }
            chain.images12.push_back(res);
            chain.imageStates12.push_back(init);
        }
        rt::g_swapchains.emplace(chain.handle, std::move(chain));
        *sc = chain.handle;
        Logf("[SimXR] xrCreateSwapchain(D3D12): sc=%p fmt=%d %ux%u array=%u samples=%u", *sc, (int)ci->format, ci->width, ci->height, ci->arraySize, ci->sampleCount);
        return XR_SUCCESS;
    }
    // Vulkan path
    if (rt::g_session.usesVulkan) {
        chain.backend = rt::Swapchain::Backend::Vulkan;
        chain.imageCount = 3;
        chain.vkFormat = (VkFormat)ci->format;

        bool isDepthVK = (chain.vkFormat == VK_FORMAT_D32_SFLOAT ||
                          chain.vkFormat == VK_FORMAT_D24_UNORM_S8_UINT ||
                          chain.vkFormat == VK_FORMAT_D16_UNORM ||
                          chain.vkFormat == VK_FORMAT_D32_SFLOAT_S8_UINT);

        VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        if (!isDepthVK) {
            usageFlags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            if (ci->usageFlags & XR_SWAPCHAIN_USAGE_UNORDERED_ACCESS_BIT)
                usageFlags |= VK_IMAGE_USAGE_STORAGE_BIT;
        } else {
            usageFlags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }
        if (ci->usageFlags & XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT) usageFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        auto findMemoryType = [&](uint32_t typeBits, VkMemoryPropertyFlags props) -> uint32_t {
            VkPhysicalDeviceMemoryProperties memProps{};
            vkGetPhysicalDeviceMemoryProperties(rt::g_session.vkPhysicalDevice, &memProps);
            for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
                if ((typeBits & (1u << i)) && (memProps.memoryTypes[i].propertyFlags & props) == props)
                    return i;
            }
            return UINT32_MAX;
        };

        for (uint32_t i = 0; i < chain.imageCount; ++i) {
            VkImageCreateInfo imgInfo{};
            imgInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imgInfo.imageType     = VK_IMAGE_TYPE_2D;
            imgInfo.format        = chain.vkFormat;
            imgInfo.extent        = { chain.width, chain.height, 1 };
            imgInfo.mipLevels     = ci->mipCount ? ci->mipCount : 1;
            imgInfo.arrayLayers   = chain.arraySize ? chain.arraySize : 1;
            imgInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
            imgInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
            imgInfo.usage         = usageFlags;
            imgInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
            imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            VkImage img = VK_NULL_HANDLE;
            VkResult vr = vkCreateImage(rt::g_session.vkDevice, &imgInfo, nullptr, &img);
            if (vr != VK_SUCCESS) {
                Logf("[SimXR] xrCreateSwapchain(Vulkan): vkCreateImage[%u] failed: %d", i, (int)vr);
                return XR_ERROR_RUNTIME_FAILURE;
            }

            VkMemoryRequirements memReq{};
            vkGetImageMemoryRequirements(rt::g_session.vkDevice, img, &memReq);
            uint32_t memTypeIdx = findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            if (memTypeIdx == UINT32_MAX) {
                Logf("[SimXR] xrCreateSwapchain(Vulkan): no suitable memory type for image %u", i);
                vkDestroyImage(rt::g_session.vkDevice, img, nullptr);
                return XR_ERROR_RUNTIME_FAILURE;
            }

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize  = memReq.size;
            allocInfo.memoryTypeIndex = memTypeIdx;

            VkDeviceMemory mem = VK_NULL_HANDLE;
            vr = vkAllocateMemory(rt::g_session.vkDevice, &allocInfo, nullptr, &mem);
            if (vr != VK_SUCCESS) {
                Logf("[SimXR] xrCreateSwapchain(Vulkan): vkAllocateMemory[%u] failed: %d", i, (int)vr);
                vkDestroyImage(rt::g_session.vkDevice, img, nullptr);
                return XR_ERROR_RUNTIME_FAILURE;
            }

            vkBindImageMemory(rt::g_session.vkDevice, img, mem, 0);
            chain.imagesVK.push_back(img);
            chain.imageMemoriesVK.push_back(mem);
        }

        rt::g_swapchains.emplace(chain.handle, std::move(chain));
        *sc = chain.handle;
        Logf("[SimXR] xrCreateSwapchain(Vulkan): sc=%p fmt=%d %ux%u array=%u",
             *sc, (int)ci->format, ci->width, ci->height, ci->arraySize);
        return XR_SUCCESS;
    }
    // OpenGL path
    if (rt::g_session.usesOpenGL) {
        chain.backend = rt::Swapchain::Backend::OpenGL;
        chain.imageCount = 3;

        // Check the current GL context state
        HGLRC currentRC = wglGetCurrentContext();
        HDC currentDC = wglGetCurrentDC();
        Logf("[SimXR] OpenGL swapchain: currentRC=%p, currentDC=%p, sessionRC=%p, sessionDC=%p",
             currentRC, currentDC, rt::g_session.glRC, rt::g_session.glDC);

        // If the app's context is already current, use it directly
        // Otherwise, switch to the app's context
        HGLRC prevRC = currentRC;
        HDC prevDC = currentDC;
        bool contextSwitched = false;

        if (currentRC != rt::g_session.glRC) {
            Log("[SimXR] Context mismatch - switching to app's context");
            if (!wglMakeCurrent(rt::g_session.glDC, rt::g_session.glRC)) {
                Logf("[SimXR] xrCreateSwapchain(OpenGL): wglMakeCurrent failed (error=%lu)", GetLastError());
                return XR_ERROR_RUNTIME_FAILURE;
            }
            contextSwitched = true;
        } else {
            Log("[SimXR] App's GL context is already current - good!");
        }

        // Log the GL version and renderer for debugging
        const char* glVersion = (const char*)glGetString(GL_VERSION);
        const char* glRenderer = (const char*)glGetString(GL_RENDERER);
        Logf("[SimXR] GL context: version=%s, renderer=%s",
             glVersion ? glVersion : "(null)", glRenderer ? glRenderer : "(null)");

        // Map format to OpenGL internal format and pixel format
        // The format from createInfo is a GL internal format (e.g. GL_RGBA8) when using OpenGL path
        auto GLFormatToPixelFormat = [](GLenum internalFmt) -> GLenum {
            switch (internalFmt) {
                case GL_SRGB8_ALPHA8:
                case GL_RGBA8:
                    return GL_RGBA;
                case GL_RGBA16F:
                case GL_RGBA32F:
                    return GL_RGBA;
                case GL_RGB10_A2:
                    return GL_RGBA;
                case GL_DEPTH_COMPONENT32F:
                case GL_DEPTH_COMPONENT16:
                    return GL_DEPTH_COMPONENT;
                case GL_DEPTH24_STENCIL8:
                    return GL_DEPTH_STENCIL;
                default:
                    return GL_RGBA;  // Default fallback
            }
        };

        // ci->format already contains the GL internal format
        GLenum glInternalFormat = (GLenum)ci->format;
        GLenum glFormat = GLFormatToPixelFormat(glInternalFormat);
        chain.glInternalFormat = glInternalFormat;

        bool isDepthFormat = (glInternalFormat == GL_DEPTH_COMPONENT32F ||
                              glInternalFormat == GL_DEPTH24_STENCIL8 ||
                              glInternalFormat == GL_DEPTH_COMPONENT16);

        // Load glTexImage3D if needed for array textures
        if (chain.arraySize > 1 && !EnsureGLTexImage3D()) {
            wglMakeCurrent(prevDC, prevRC);
            return XR_ERROR_RUNTIME_FAILURE;
        }

        // Create OpenGL textures
        for (uint32_t i = 0; i < chain.imageCount; ++i) {
            GLuint tex;
            glGenTextures(1, &tex);

            if (chain.arraySize > 1) {
                // Use texture array
                glBindTexture(GL_TEXTURE_2D_ARRAY, tex);
                g_glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, glInternalFormat,
                               chain.width, chain.height, chain.arraySize,
                               0, glFormat, GL_UNSIGNED_BYTE, nullptr);
                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
            } else {
                // Use regular 2D texture
                glBindTexture(GL_TEXTURE_2D, tex);
                if (isDepthFormat) {
                    GLenum type = (glInternalFormat == GL_DEPTH24_STENCIL8) ? GL_UNSIGNED_INT_24_8 : GL_FLOAT;
                    glTexImage2D(GL_TEXTURE_2D, 0, glInternalFormat,
                                 chain.width, chain.height, 0,
                                 glFormat, type, nullptr);
                } else {
                    // DEBUG: Initialize with bright green to verify texture pipeline
                    std::vector<uint8_t> initData(chain.width * chain.height * 4);
                    for (size_t p = 0; p < initData.size(); p += 4) {
                        initData[p + 0] = 0;    // R
                        initData[p + 1] = 255;  // G - bright green
                        initData[p + 2] = 0;    // B
                        initData[p + 3] = 255;  // A
                    }
                    glTexImage2D(GL_TEXTURE_2D, 0, glInternalFormat,
                                 chain.width, chain.height, 0,
                                 GL_RGBA, GL_UNSIGNED_BYTE, initData.data());
                    Logf("[SimXR] Initialized tex %u with GREEN data", tex);
                }
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glBindTexture(GL_TEXTURE_2D, 0);
            }

            GLenum err = glGetError();
            if (err != GL_NO_ERROR) {
                Logf("[SimXR] OpenGL texture creation error[%u]: 0x%X", i, err);
                if (contextSwitched) wglMakeCurrent(prevDC, prevRC);
                return XR_ERROR_RUNTIME_FAILURE;
            }

            // Verify the texture is valid
            GLboolean isValid = glIsTexture(tex);
            chain.imagesGL.push_back(tex);
            Logf("[SimXR] Created GL texture[%u]: %u (format=0x%X, valid=%d)", i, tex, glInternalFormat, isValid);
        }

        // Restore previous context only if we switched
        if (contextSwitched) {
            Log("[SimXR] Restoring previous GL context");
            wglMakeCurrent(prevDC, prevRC);
        }

        rt::g_swapchains.emplace(chain.handle, std::move(chain));
        *sc = chain.handle;
        Logf("[SimXR] xrCreateSwapchain(OpenGL): sc=%p fmt=%d %ux%u array=%u imageCount=%u",
             *sc, (int)ci->format, ci->width, ci->height, ci->arraySize, chain.imageCount);
        return XR_SUCCESS;
    }
    // D3D11 path
    D3D11_TEXTURE2D_DESC td{};

    // Determine if this is a depth format
    bool isDepthFormat = (chain.format == DXGI_FORMAT_D32_FLOAT ||
                          chain.format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT ||
                          chain.format == DXGI_FORMAT_D24_UNORM_S8_UINT ||
                          chain.format == DXGI_FORMAT_D16_UNORM);

    // For depth formats that need to be sampled, we must use typeless format
    bool needsTypelessDepth = isDepthFormat && (ci->usageFlags & XR_SWAPCHAIN_USAGE_SAMPLED_BIT);

    // Convert depth format to typeless when sampling is needed
    auto ToTypelessDepth = [](DXGI_FORMAT fmt) -> DXGI_FORMAT {
        switch (fmt) {
            case DXGI_FORMAT_D32_FLOAT:            return DXGI_FORMAT_R32_TYPELESS;
            case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: return DXGI_FORMAT_R32G8X24_TYPELESS;
            case DXGI_FORMAT_D24_UNORM_S8_UINT:    return DXGI_FORMAT_R24G8_TYPELESS;
            case DXGI_FORMAT_D16_UNORM:            return DXGI_FORMAT_R16_TYPELESS;
            default: return fmt;
        }
    };

    // Use typeless format for color textures to allow both UNORM and SRGB views
    // Use typeless format for depth textures when sampling is needed
    if (isDepthFormat) {
        td.Format = needsTypelessDepth ? ToTypelessDepth(chain.format) : chain.format;
    } else {
        td.Format = ToTypeless(chain.format);
    }

    td.Width = chain.width;
    td.Height = chain.height;
    td.ArraySize = chain.arraySize ? chain.arraySize : 1;  // Ensure at least 1
    td.MipLevels = ci->mipCount ? ci->mipCount : 1;  // Ensure at least 1
    chain.mipCount = td.MipLevels;
    td.SampleDesc.Count = ci->sampleCount ? ci->sampleCount : 1;
    td.SampleDesc.Quality = 0;  // Must be 0 for non-MSAA
    // Set bind flags based on format type

    if (isDepthFormat) {
        td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        // Typeless depth formats can also be shader resources
        if (needsTypelessDepth) {
            td.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
        }
    } else {
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        
        // Add unordered access if requested
        if (ci->usageFlags & XR_SWAPCHAIN_USAGE_UNORDERED_ACCESS_BIT) {
            td.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
        }
    }
    
    td.Usage = D3D11_USAGE_DEFAULT;
    td.CPUAccessFlags = 0;
    
    // Set MiscFlags based on what Unity might need
    td.MiscFlags = 0;
    
    // Direct transport shares single-sample, single-mip colour images; anything else keeps the preview path
    uint32_t bridgeFormat = directTransport && !isDepthFormat && td.SampleDesc.Count == 1 && td.MipLevels == 1
                            ? BridgeVkFormat(td.Format) : 0;
    if (bridgeFormat && BridgeReady()) td.MiscFlags = D3D11_RESOURCE_MISC_SHARED | D3D11_RESOURCE_MISC_SHARED_NTHANDLE;

    // Log the texture description for debugging
    Logf("[SimXR] Creating swapchain textures: Format=%d, %ux%u, Array=%u, Mips=%u, Samples=%u",
         td.Format, td.Width, td.Height, td.ArraySize, td.MipLevels, td.SampleDesc.Count);
    chain.imageCount = 3;
    for (uint32_t i = 0; i < chain.imageCount; ++i) {
        ComPtr<ID3D11Texture2D> tex;
        HRESULT hr = rt::g_session.d3d11Device->CreateTexture2D(&td, nullptr, tex.GetAddressOf());
        if (FAILED(hr) && td.MiscFlags) {
            // DXVK refused to share this texture: fall back to plain ones for the whole chain
            Logf("[SimXR] direct_transport: shared CreateTexture2D failed (hr=0x%08X), not sharing this swapchain", (unsigned)hr);
            td.MiscFlags = 0;
            chain.images.clear();
            i = UINT32_MAX;  // restart at 0
            continue;
        }
        if (FAILED(hr)) {
            Logf("[SimXR] CreateTexture2D[%u] FAILED: hr=0x%08X", i, (unsigned)hr);
            Logf("[SimXR]   Format=%d, Size=%ux%u, Array=%u, Mips=%u, Samples=%u, BindFlags=0x%X",
                 td.Format, td.Width, td.Height, td.ArraySize, td.MipLevels, td.SampleDesc.Count, td.BindFlags);
            
            // Try to provide more specific error info
            if (hr == E_INVALIDARG) {
                Log("[SimXR]   ERROR: E_INVALIDARG - Invalid texture parameters");
                // Check common issues
                if (td.ArraySize == 0) Log("[SimXR]   - ArraySize is 0");
                if (td.Width == 0 || td.Height == 0) Log("[SimXR]   - Invalid dimensions");
                if (td.MipLevels == 0) Log("[SimXR]   - MipLevels is 0");
            }
            return XR_ERROR_RUNTIME_FAILURE; 
        }
        Logf("[SimXR] Created swapchain texture[%u]: %p", i, tex.Get());
        chain.images.push_back(std::move(tex));
    }
    if (td.MiscFlags) {
        bool typeless = td.Format == DXGI_FORMAT_R8G8B8A8_TYPELESS || td.Format == DXGI_FORMAT_B8G8R8A8_TYPELESS;
        uint32_t flags = (typeless ? WXR_BRIDGE_IMPORT_MUTABLE_FORMAT : 0) |
                         ((td.BindFlags & D3D11_BIND_UNORDERED_ACCESS) ? WXR_BRIDGE_IMPORT_STORAGE : 0);
        for (auto& image : chain.images) chain.bridgeIds.push_back(BridgeImport(image.Get(), td, bridgeFormat, flags));
    }
    rt::g_swapchains.emplace(chain.handle, std::move(chain));
    *sc = chain.handle;
    Logf("[SimXR] xrCreateSwapchain: sc=%p fmt=%d %ux%u array=%u samples=%u", *sc, (int)ci->format, ci->width, ci->height, ci->arraySize, ci->sampleCount);
    return XR_SUCCESS;
}

static XrResult XRAPI_PTR xrEnumerateSwapchainImages_runtime(XrSwapchain sc, uint32_t capacity, uint32_t* count, XrSwapchainImageBaseHeader* images) {
    auto it = rt::g_swapchains.find(sc); if (it == rt::g_swapchains.end()) return XR_ERROR_HANDLE_INVALID;
    if (it->second.backend == rt::Swapchain::Backend::D3D12) {
        const uint32_t n = (uint32_t)it->second.images12.size();
        if (count) *count = n;
        if (capacity >= n && images) {
            auto* arr = reinterpret_cast<XrSwapchainImageD3D12KHR*>(images);
            for (uint32_t i = 0; i < n; ++i) { arr[i].type = XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR; arr[i].texture = it->second.images12[i].Get(); }
        }
        if (verboseLogging) Logf("[SimXR] xrEnumerateSwapchainImages(D3D12): sc=%p count=%u", sc, n);
        return XR_SUCCESS;
    } else if (it->second.backend == rt::Swapchain::Backend::OpenGL) {
        const uint32_t n = (uint32_t)it->second.imagesGL.size();
        if (count) *count = n;
        if (capacity >= n && images) {
            auto* arr = reinterpret_cast<XrSwapchainImageOpenGLKHR*>(images);
            for (uint32_t i = 0; i < n; ++i) {
                arr[i].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;
                arr[i].image = it->second.imagesGL[i];
            }
        } else {
            if (verboseLogging) Logf("[SimXR] xrEnumerateSwapchainImages(OpenGL): sc=%p count=%u (query only)", sc, n);
        }
        return XR_SUCCESS;
    } else if (it->second.backend == rt::Swapchain::Backend::Vulkan) {
        const uint32_t n = (uint32_t)it->second.imagesVK.size();
        if (count) *count = n;
        if (capacity >= n && images) {
            auto* arr = reinterpret_cast<XrSwapchainImageVulkanKHR*>(images);
            for (uint32_t i = 0; i < n; ++i) {
                arr[i].type  = XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR;
                arr[i].next  = nullptr;
                arr[i].image = it->second.imagesVK[i];
            }
        }
        if (verboseLogging) Logf("[SimXR] xrEnumerateSwapchainImages(Vulkan): sc=%p count=%u", sc, n);
        return XR_SUCCESS;
    } else {
        const uint32_t n = (uint32_t)it->second.images.size();
        if (count) *count = n;
        if (capacity >= n && images) {
            auto* arr = reinterpret_cast<XrSwapchainImageD3D11KHR*>(images);
            for (uint32_t i = 0; i < n; ++i) { arr[i].type = XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR; arr[i].texture = it->second.images[i].Get(); }
        }
        if (verboseLogging) Logf("[SimXR] xrEnumerateSwapchainImages(D3D11): sc=%p count=%u", sc, n);
        return XR_SUCCESS;
    }
}

static XrResult XRAPI_PTR xrAcquireSwapchainImage_runtime(XrSwapchain sc, const XrSwapchainImageAcquireInfo*, uint32_t* index) {
    auto it = rt::g_swapchains.find(sc); if (it == rt::g_swapchains.end()) return XR_ERROR_HANDLE_INVALID;
    auto& ch = it->second;
    uint32_t i = ch.nextIndex;
    ch.nextIndex = (ch.nextIndex + 1) % ch.imageCount;
    ch.lastAcquired = i;  // Track what we just gave to the app
    if (index) *index = i; 
    
    static int acquireCount = 0;
    if ((++acquireCount % 60 == 1) && verboseLogging) {  // Log every 60 calls
        Logf("[SimXR] xrAcquireSwapchainImage: sc=%p idx=%u (format=%d, %ux%u)", 
             sc, i, (int)ch.format, ch.width, ch.height);
    }
    return XR_SUCCESS;
}
static XrResult XRAPI_PTR xrWaitSwapchainImage_runtime(XrSwapchain, const XrSwapchainImageWaitInfo*) { return XR_SUCCESS; }
static XrResult XRAPI_PTR xrReleaseSwapchainImage_runtime(XrSwapchain sc, const XrSwapchainImageReleaseInfo*) {
    auto it = rt::g_swapchains.find(sc);
    if (it == rt::g_swapchains.end()) return XR_ERROR_HANDLE_INVALID;
    auto& ch = it->second;
    // The app just released the image it acquired earlier
    ch.lastReleased = ch.lastAcquired;

    static int releaseCount = 0;
    bool shouldLog = (++releaseCount <= 10);
    if ((shouldLog || releaseCount % 60 == 1) && verboseLogging) {
        Logf("[SimXR] xrReleaseSwapchainImage: sc=%p released=%u", sc, ch.lastReleased);
    }
    return XR_SUCCESS;
}

// Helper struct to save and restore D3D11 context state using RAII
struct D3D11StateBackup {
    D3D11StateBackup(ID3D11DeviceContext* ctx) : ctx_(ctx) {
        // IA
        ctx_->IAGetInputLayout(&ia_input_layout);
        ctx_->IAGetPrimitiveTopology(&ia_primitive_topology);
        // RS
        rs_num_viewports = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        ctx_->RSGetViewports(&rs_num_viewports, rs_viewports);
        rs_num_scissor_rects = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        ctx_->RSGetScissorRects(&rs_num_scissor_rects, rs_scissor_rects);
        ctx_->RSGetState(&rs_state);
        // OM
        ctx_->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, om_rtvs, &om_dsv);
        ctx_->OMGetBlendState(&om_blend_state, om_blend_factor, &om_sample_mask);
        ctx_->OMGetDepthStencilState(&om_depth_stencil_state, &om_stencil_ref);
        // Shaders - MUST initialize class instance counts before calling GetShader
        ps_num_class_instances = 256;  // Initialize to array capacity
        ctx_->PSGetShader(&ps_shader, ps_class_instances, &ps_num_class_instances);
        // blitViewToHalf binds slot 0 of each of these, so slot 0 is all that has to survive it.
        // Saving every slot cost ~145 Get calls plus an AddRef and a Release on each bound object,
        // every frame, on the thread whose job is getting the frame out.
        ctx_->PSGetSamplers(0, 1, ps_samplers);
        ctx_->PSGetShaderResources(0, 1, ps_srvs);
        // The blit binds its own PS constant buffer in slot 0 and nothing restored the game's
        ctx_->PSGetConstantBuffers(0, 1, ps_constant_buffers);
        vs_num_class_instances = 256;  // Initialize to array capacity
        ctx_->VSGetShader(&vs_shader, vs_class_instances, &vs_num_class_instances);
    }

    ~D3D11StateBackup() {
        // Restore state
        ctx_->IASetInputLayout(ia_input_layout);
        ctx_->IASetPrimitiveTopology(ia_primitive_topology);
        ctx_->RSSetViewports(rs_num_viewports, rs_viewports);
        ctx_->RSSetScissorRects(rs_num_scissor_rects, rs_scissor_rects);
        ctx_->RSSetState(rs_state);
        ctx_->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, om_rtvs, om_dsv);
        ctx_->OMSetBlendState(om_blend_state, om_blend_factor, om_sample_mask);
        ctx_->OMSetDepthStencilState(om_depth_stencil_state, om_stencil_ref);
        ctx_->PSSetShader(ps_shader, ps_class_instances, ps_num_class_instances);
        ctx_->PSSetSamplers(0, 1, ps_samplers);
        ctx_->PSSetShaderResources(0, 1, ps_srvs);
        ctx_->PSSetConstantBuffers(0, 1, ps_constant_buffers);
        ctx_->VSSetShader(vs_shader, vs_class_instances, vs_num_class_instances);

        // Release COM references
        if (ia_input_layout) ia_input_layout->Release();
        if (rs_state) rs_state->Release();
        for (UINT i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) if (om_rtvs[i]) om_rtvs[i]->Release();
        if (om_dsv) om_dsv->Release();
        if (om_blend_state) om_blend_state->Release();
        if (om_depth_stencil_state) om_depth_stencil_state->Release();
        if (ps_shader) ps_shader->Release();
        for (UINT i = 0; i < ps_num_class_instances; ++i) if (ps_class_instances[i]) ps_class_instances[i]->Release();
        if (ps_samplers[0]) ps_samplers[0]->Release();
        if (ps_srvs[0]) ps_srvs[0]->Release();
        if (ps_constant_buffers[0]) ps_constant_buffers[0]->Release();
        if (vs_shader) vs_shader->Release();
        for (UINT i = 0; i < vs_num_class_instances; ++i) if (vs_class_instances[i]) vs_class_instances[i]->Release();
    }

private:
    ID3D11DeviceContext* ctx_;
    // IA State
    ID3D11InputLayout* ia_input_layout = nullptr;
    D3D11_PRIMITIVE_TOPOLOGY ia_primitive_topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    // RS State  
    UINT rs_num_viewports = 0, rs_num_scissor_rects = 0;
    D3D11_VIEWPORT rs_viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
    D3D11_RECT rs_scissor_rects[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
    ID3D11RasterizerState* rs_state = nullptr;
    // OM State
    ID3D11RenderTargetView* om_rtvs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = { nullptr };
    ID3D11DepthStencilView* om_dsv = nullptr;
    ID3D11BlendState* om_blend_state = nullptr;
    FLOAT om_blend_factor[4] = { 0.0f };
    UINT om_sample_mask = 0;
    ID3D11DepthStencilState* om_depth_stencil_state = nullptr;
    UINT om_stencil_ref = 0;
    // PS State
    ID3D11PixelShader* ps_shader = nullptr;
    ID3D11ClassInstance* ps_class_instances[256] = { nullptr };
    UINT ps_num_class_instances = 0;
    ID3D11SamplerState* ps_samplers[1] = { nullptr };
    ID3D11ShaderResourceView* ps_srvs[1] = { nullptr };
    ID3D11Buffer* ps_constant_buffers[1] = { nullptr };
    // VS State
    ID3D11VertexShader* vs_shader = nullptr;
    ID3D11ClassInstance* vs_class_instances[256] = { nullptr };
    UINT vs_num_class_instances = 0;
};

namespace rt {
    static XrSessionState g_state = XR_SESSION_STATE_IDLE;
    static std::vector<XrEventDataBuffer> g_eventQueue;
    void PushState(XrSession s, XrSessionState ns) {
        g_state = ns;
        g_session.state = ns;
        const char* stateName = "UNKNOWN";
        switch(ns) {
            case XR_SESSION_STATE_IDLE: stateName = "IDLE"; break;
            case XR_SESSION_STATE_READY: stateName = "READY"; break;
            case XR_SESSION_STATE_SYNCHRONIZED: stateName = "SYNCHRONIZED"; break;
            case XR_SESSION_STATE_VISIBLE: stateName = "VISIBLE"; break;
            case XR_SESSION_STATE_FOCUSED: stateName = "FOCUSED"; break;
            case XR_SESSION_STATE_STOPPING: stateName = "STOPPING"; break;
            case XR_SESSION_STATE_LOSS_PENDING: stateName = "LOSS_PENDING"; break;
            case XR_SESSION_STATE_EXITING: stateName = "EXITING"; break;
        }
        if (verboseLogging) Logf("[SimXR] PushState: Session %llu -> %s", (unsigned long long)s, stateName);
        
        XrEventDataSessionStateChanged e{XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED};
        e.session = s; e.state = ns; e.time = 0;
        
        XrEventDataBuffer buf{};
        buf.type = XR_TYPE_EVENT_DATA_BUFFER;  // Set the base type
        std::memcpy(&buf, &e, sizeof(e));
        g_eventQueue.push_back(buf);
        if (verboseLogging) Logf("[SimXR] Event queue now has %zu events", g_eventQueue.size());
    }
}
static XrResult XRAPI_PTR xrPollEvent_runtime(XrInstance, XrEventDataBuffer* b) {
    static int pollCount = 0;
    pollCount++;
    
    if (pollCount <= 5 && verboseLogging) {  // Log first few polls
        Logf("[SimXR] xrPollEvent called (#%d), queue size=%zu", pollCount, rt::g_eventQueue.size());
    }
    
    if (!b) return XR_ERROR_VALIDATION_FAILURE;
    if (rt::g_eventQueue.empty()) {
        if (pollCount <= 5) {
            Log("[SimXR] xrPollEvent: No events available (XR_EVENT_UNAVAILABLE)");
        }
        return XR_EVENT_UNAVAILABLE;
    }
    *b = rt::g_eventQueue.front();
    rt::g_eventQueue.erase(rt::g_eventQueue.begin());
    
    // Log what event we're delivering
    const XrEventDataBaseHeader* header = reinterpret_cast<const XrEventDataBaseHeader*>(b);
    if (header->type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
        const XrEventDataSessionStateChanged* stateEvent = reinterpret_cast<const XrEventDataSessionStateChanged*>(b);
        const char* stateName = "UNKNOWN";
        switch(stateEvent->state) {
            case XR_SESSION_STATE_IDLE: stateName = "IDLE"; break;
            case XR_SESSION_STATE_READY: stateName = "READY"; break;
            case XR_SESSION_STATE_SYNCHRONIZED: stateName = "SYNCHRONIZED"; break;
            case XR_SESSION_STATE_VISIBLE: stateName = "VISIBLE"; break;
            case XR_SESSION_STATE_FOCUSED: stateName = "FOCUSED"; break;
            case XR_SESSION_STATE_STOPPING: stateName = "STOPPING"; break;
            case XR_SESSION_STATE_LOSS_PENDING: stateName = "LOSS_PENDING"; break;
            case XR_SESSION_STATE_EXITING: stateName = "EXITING"; break;
        }
        if (verboseLogging) Logf("[SimXR] xrPollEvent: Delivering SESSION_STATE_CHANGED -> %s (session=%llu, %zu events left)",
            stateName, (unsigned long long)stateEvent->session, rt::g_eventQueue.size());
    } else {
        if (verboseLogging) Logf("[SimXR] xrPollEvent: Delivering event type %d (%zu events left)", header->type, rt::g_eventQueue.size());
    }
    return XR_SUCCESS;
}
static XrResult XRAPI_PTR xrBeginSession_runtime(XrSession s, const XrSessionBeginInfo*) { 
    Log("[SimXR] ============================================");
    Logf("[SimXR] xrBeginSession called (session=%llu)", (unsigned long long)s);
    Log("[SimXR] Session started - moving to SYNCHRONIZED/VISIBLE states");
    Log("[SimXR] ============================================");
    rt::PushState(s, XR_SESSION_STATE_SYNCHRONIZED); 
    rt::PushState(s, XR_SESSION_STATE_VISIBLE);
    rt::PushState(s, XR_SESSION_STATE_FOCUSED);
    return XR_SUCCESS; 
}
static XrResult XRAPI_PTR xrEndSession_runtime(XrSession s) { Log("[SimXR] xrEndSession"); rt::PushState(s, XR_SESSION_STATE_STOPPING); rt::PushState(s, XR_SESSION_STATE_IDLE); return XR_SUCCESS; }
static XrResult XRAPI_PTR xrRequestExitSession_runtime(XrSession s) { rt::PushState(s, XR_SESSION_STATE_EXITING); return XR_SUCCESS; }
static XrResult XRAPI_PTR xrWaitFrame_runtime(XrSession, const XrFrameWaitInfo*, XrFrameState* s) {
    if (!s) return XR_ERROR_VALIDATION_FAILURE;
    // Message pump so the preview window stays responsive
    MSG msg; while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    static LARGE_INTEGER freq = [](){ LARGE_INTEGER f; QueryPerformanceFrequency(&f); return f; }();
    static double periodSec = 1.0 / 90.0;
    static long long periodNs = (long long)(periodSec * 1e9);
    static double nextTick = []() { LARGE_INTEGER t; QueryPerformanceCounter(&t); return (double)t.QuadPart; }();

    // Paced before the host state is read, so the game gets the poses as they are when it starts the frame
    for (;;) {
        LARGE_INTEGER now; QueryPerformanceCounter(&now);
        double dt = (nextTick - (double)now.QuadPart) / (double)freq.QuadPart;
        if (dt <= 0.0) {
            // A game slower than the display must not bank ticks, or it runs uncapped until they are used up
            if (-dt > periodSec) nextTick = (double)now.QuadPart;
            break;
        }
        double ms = dt * 1000.0;
        if (ms > 5.0) ms = 5.0;
        if (ms < 0.0) ms = 0.0;
        Sleep((DWORD)ms);
    }
    nextTick += periodSec * (double)freq.QuadPart;

    //----------------
    //OXRWXR CHANGE:
    //----------------
    // Now we pass 6DOF data always
    // The packet was parsed when it arrived; static so the pose list keeps its buffer between frames
    static WxrHostState host;
    if (!udpReader->GetState(host)) {
        static bool logged = false;
        if (!logged) Log("[SimXR] xrWaitFrame: ERROR - the UDP port is not bound so no data can come from the host, failing the call");
        logged = true;
        return XR_ERROR_RUNTIME_FAILURE;
    }

    const float* floats = host.floats;
    const bool* buttonBools = host.buttons;
    int openXRFrameID = host.frameId;
    int recenterID = host.recenterId;

    // Poses for XrLocateSpace
    for (const WxrHostPose& hostPose : host.poses) {
        XrPosef pose;
        pose.position = { hostPose.pose[0], hostPose.pose[1], hostPose.pose[2] };
        pose.orientation = { hostPose.pose[3], hostPose.pose[4], hostPose.pose[5], hostPose.pose[6] };
        XrSpaceVelocity velocity{XR_TYPE_SPACE_VELOCITY};
        velocity.velocityFlags = (XrSpaceVelocityFlags)hostPose.velocity[0];
        velocity.linearVelocity = { hostPose.velocity[1], hostPose.velocity[2], hostPose.velocity[3] };
        velocity.angularVelocity = { hostPose.velocity[4], hostPose.velocity[5], hostPose.velocity[6] };

        std::pair<XrSpace, XrSpace> key;
        key.first = (XrSpace)(intptr_t)hostPose.space;
        key.second = (XrSpace)(intptr_t)hostPose.baseSpace;
        if (pose.orientation.x == 0 && pose.orientation.y == 0 && pose.orientation.z == 0 && pose.orientation.w == 0) {
            // Pair not located by the host
            SpaceVelocities.erase(key);
            SpacePoses.erase(key);
            continue;
        }
        SpacePoses[key] = pose;
        SpaceVelocities[key] = velocity;
    }

    OpenXRFrameID = openXRFrameID;

    udpReader->LastOpenXRFrameID = OpenXRFrameID;

    // Check if recenter should be called
    static int lastRecenterID = 0;
    if (lastRecenterID != recenterID) {
        XrEventDataReferenceSpaceChangePending e{XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING};
        e.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
        XrEventDataBuffer buf{};
        buf.type = XR_TYPE_EVENT_DATA_BUFFER;  // Set the base type
        std::memcpy(&buf, &e, sizeof(e));
        rt::g_eventQueue.push_back(buf);
        lastRecenterID = recenterID;
    }

    IPDVal = floats[8];
    if ((FOVH < 0) || (FOVV < 0)) {
       FOVH = floats[9] * toRadians;
       FOVV = floats[10] * toRadians;
    }

    float hz = floats[11];
    if (hz >= 30.0f && hz <= 240.0f) {
        periodSec = 1.0 / hz;
        periodNs = (long long)(periodSec * 1e9);
    }

    lastPosFrame += 1;
    if (lastPosFrame > 4) {
        lastPosFrame = 0;
    }

    float deltaTime = (float)periodSec;

    rt::g_leftController.gripPressed = buttonBools[0];
    rt::g_leftController.menuPressed = buttonBools[1];
    rt::g_leftController.thumbstickPressed = buttonBools[2];
    rt::g_leftController.triggerPressed = buttonBools[7];
    rt::g_leftController.primaryPressed = buttonBools[8];
    rt::g_leftController.secondaryPressed = buttonBools[9];
    rt::g_rightController.primaryPressed = buttonBools[10];
    rt::g_rightController.secondaryPressed = buttonBools[11];
    rt::g_rightController.gripPressed = buttonBools[12];
    rt::g_rightController.thumbstickPressed = buttonBools[13];
    rt::g_rightController.triggerPressed = buttonBools[18];
    rt::g_rightController.menuPressed = (buttonBools[12] && buttonBools[1]); //Right grip + L Menu to trigger the OpenXR menu

    rt::g_leftController.thumbstick = makeXrVector2f(floats[0], floats[1]);
    rt::g_rightController.thumbstick = makeXrVector2f(floats[2], floats[3]);
    rt::g_leftController.triggerValue = floats[4];
    rt::g_leftController.gripValue = floats[5];
    rt::g_rightController.triggerValue = floats[6];
    rt::g_rightController.gripValue = floats[7];

    LARGE_INTEGER now; QueryPerformanceCounter(&now);
    // Convert QPC to nanoseconds using double to avoid overflow on MSVC
    XrTime nowTime = (XrTime)((double)now.QuadPart * 1000000000.0 / (double)freq.QuadPart);
    s->type = XR_TYPE_FRAME_STATE; s->shouldRender = XR_TRUE; s->predictedDisplayPeriod = periodNs; s->predictedDisplayTime = nowTime + periodNs;
    return XR_SUCCESS;
}

static std::string PoseToString(const XrPosef& pose) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(3);
    ss << pose.position.x << " "
       << pose.position.y << " "
       << pose.position.z << " "
       << pose.orientation.x << " "
       << pose.orientation.y << " "
       << pose.orientation.z << " "
       << pose.orientation.w;

    size_t pos = 0;
    std::string str = ss.str();
    const std::string target = ".000";
    while ((pos = str.find(target, pos)) != std::string::npos) {
        str.erase(pos, target.length());
    }
    return str;
}

static void sendUdpData() {
    // XrAPI v0.5 data
    std::string msg = "";
    msg += "0 0 ";                                //haptics
    msg += isVR ? "1 " : "2 ";                    //VR
    msg += monoRendering || !isVR ? "-1 " : "1 "; //3D

    // Custom FOV
    msg += std::to_string(FOVH / toRadians) + " ";
    msg += std::to_string(FOVV / toRadians) + " ";

    // XrAPI v0.6 reference spaces
    msg += std::to_string(rt::g_referenceSpaceType.size()) + " ";
    for (const auto& [space, referenceSpaceType] : rt::g_referenceSpaceType) {
        msg += std::to_string((int)space) + " ";
        // The host has no LOCAL_FLOOR; its STAGE is on the floor and follows the recenter, which is the same thing
        msg += std::to_string(referenceSpaceType == XR_REFERENCE_SPACE_TYPE_LOCAL_FLOOR ? XR_REFERENCE_SPACE_TYPE_STAGE : referenceSpaceType) + " ";
        msg += PoseToString(rt::g_referenceSpacePose[space]) + " ";
    }

    // XrAPI v0.6 action spaces
    {
        std::lock_guard<std::mutex> lock(rt::g_controllerMutex);
        msg += std::to_string(rt::g_controllerSpaces.size()) + " ";
        for (const auto& [space, controllerType] : rt::g_controllerSpaces) {
            msg += std::to_string((int)space) + " ";
            msg += std::to_string(controllerType) + " ";
            msg += rt::g_controllerGrips[space] ? "1 " : "0 ";
            msg += PoseToString(rt::g_controllerPoses[space]) + " ";
        }
    }

    // XrAPI v0.6 locate spaces
    msg += std::to_string(rt::g_locateSpaces.size()) + " ";
    for (auto it = rt::g_locateSpaces.begin(); it != rt::g_locateSpaces.end(); it++) {
        const auto& [spaceA, spaceB] = it->first;
        msg += std::to_string((int)spaceA) + " ";
        msg += std::to_string((int)spaceB) + " ";
    }

    udpReader->SendData(msg);
}

static XrResult XRAPI_PTR xrBeginFrame_runtime(XrSession, const XrFrameBeginInfo*) { return XR_SUCCESS; }

static void ensurePreviewSized(rt::Session& s, UINT width, UINT height, int format) {
    if (s.usesVulkan) {
        if (s.vkPreviewSwapchain && s.previewWidth == width && s.previewHeight == height && s.previewFormat == format) return;
    } else if (!s.usesD3D12) {
        if (s.previewSwapchain && s.previewWidth == width && s.previewHeight == height && s.previewFormat == format) return;
    } else {
        if (s.previewSwapchain12 && s.previewWidth == width && s.previewHeight == height && s.previewFormat == format) return;
    }

    // IMPORTANT: Release ALL swapchain references before creating a new one
    // DXGI only allows one swapchain per window
    s.previewSwapchain.Reset();
    rt::ResetD3D12PreviewResources(s);
    {
        std::lock_guard<std::mutex> lock(rt::g_windowMutex);
        rt::g_persistentSwapchain.Reset();
    }
    s.previewWidth = width;
    s.previewHeight = height;
    s.previewFormat = format;

    // Register window class if not done (use global flag)
    if (!rt::g_windowClassRegistered) {
        WNDCLASSW wc{}; 
        wc.lpfnWndProc = rt::WndProc; 
        wc.hInstance = GetModuleHandleW(nullptr); 
        wc.lpszClassName = L"OpenXR Simulator";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        RegisterClassW(&wc);
        rt::g_windowClassRegistered = true;
    }

    if (!s.hwnd) {
        // Check if we have a persistent window from a previous session
        {
            std::lock_guard<std::mutex> lock(rt::g_windowMutex);
            if (rt::g_persistentWindow && IsWindow(rt::g_persistentWindow)) {
                s.hwnd = rt::g_persistentWindow;
                // DON'T clear g_persistentWindow - keep it for reference

                // CRITICAL: Update WndProc to point to this DLL's function
                // After DLL reload, the old WndProc pointer is invalid!
                SetWindowLongPtrW(s.hwnd, GWLP_WNDPROC, (LONG_PTR)rt::WndProc);
                Log("[SimXR] Updated window WndProc to new DLL address");

                // Reuse swapchain if compatible
                if (rt::g_persistentSwapchain && ViewportWidth == width && 
                    ViewportHeight == height && s.previewFormat == format) {
                    s.previewSwapchain = rt::g_persistentSwapchain;
                    s.previewWidth = ViewportWidth;
                    s.previewHeight = ViewportHeight;
                    s.previewFormat = format;
                    Log("[SimXR] Reusing existing window AND swapchain from previous session");
                    return;  // Everything is already set up
                }
                
                Log("[SimXR] Reusing existing window from previous session (recreating swapchain)");

                // IMPORTANT: Release the old persistent swapchain before creating a new one
                // DXGI only allows one swapchain per window
                rt::g_persistentSwapchain.Reset();

                if (!disableWindow) {
                    // Keep it fullscreen and let the swapchain scale: side-by-side eye buffers can be wider than the screen
                    SetWindowPos(s.hwnd, nullptr, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
                
                    // Make sure it's visible
                    ShowWindow(s.hwnd, SW_SHOWNA);
                    UpdateWindow(s.hwnd);
                }
            }
        }
        
        // Create new window if we don't have one
        if (!s.hwnd) {
            RECT rc = { 0, 0, (LONG)width, (LONG)height };
            AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
            s.hwnd = CreateWindowExW(WS_EX_NOACTIVATE, L"OpenXR Simulator", L"OpenXR Simulator (Mouse Look + WASD)", WS_OVERLAPPEDWINDOW,
                                     100, 100, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
            if (!s.hwnd) {
                Log("[SimXR] Failed to create preview window!");
                return;
            }

            if (!disableWindow) {
                ShowWindow(s.hwnd, SW_SHOWNA);
                UpdateWindow(s.hwnd);
                SetWindowPos(s.hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
            
                if (verboseLogging) Logf("[SimXR] Created new preview window: hwnd=%p size=%ux%u", s.hwnd, width, height);

                //----------------
                //OXRWXR CHANGE:
                //----------------
                //  Make the window fullscreen borderless
                RECT rc2;
                GetWindowRect(s.hwnd, &rc2);

                bool fullScreenDisplay = true;
                if (fullScreenDisplay) {
                    rc2.left = 0;
                    rc2.top = 0;
                    rc2.right = GetSystemMetrics(SM_CXSCREEN);
                    rc2.bottom = GetSystemMetrics(SM_CYSCREEN);
                }

                // Modify window style
                LONG_PTR style = GetWindowLongPtr(s.hwnd, GWL_STYLE);
                style &= ~(WS_THICKFRAME | WS_BORDER | WS_CAPTION);
                SetWindowLongPtr(s.hwnd, GWL_STYLE, style);

                // Update window position and size
                SetWindowPos(s.hwnd, HWND_TOP,
                    rc2.left, rc2.top,
                    rc2.right - rc2.left, rc2.bottom - rc2.top,
                    SWP_FRAMECHANGED | SWP_NOACTIVATE);

                ShowCursor(FALSE);
            }

            // Also save to persistent storage right away
            {
                std::lock_guard<std::mutex> lock(rt::g_windowMutex);
                rt::g_persistentWindow = s.hwnd;
                if (verboseLogging) Log("[SimXR] Saved new window to persistent storage");
            }
        }
    } else if (!disableWindow) {
        // Keep it fullscreen and let the swapchain scale: side-by-side eye buffers can be wider than the screen
        SetWindowPos(s.hwnd, nullptr, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        Logf("[SimXR] Resized preview window: hwnd=%p size=%ux%u", s.hwnd, width, height);
    }

    if (s.usesD3D12) {
        // DX12 preview swapchain
        ComPtr<IDXGIFactory4> factory;
        if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(factory.GetAddressOf())))) {
            Log("[SimXR] DX12 preview: CreateDXGIFactory1 failed");
            return;
        }
        DXGI_SWAP_CHAIN_DESC1 desc{};
        desc.Width = width;
        desc.Height = height;
        desc.Format = (DXGI_FORMAT)format;
        desc.SampleDesc.Count = 1;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = 2;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        ComPtr<IDXGISwapChain1> sc1;
        HRESULT hr = factory->CreateSwapChainForHwnd(s.d3d12Queue.Get(), s.hwnd, &desc, nullptr, nullptr, sc1.GetAddressOf());
        if (FAILED(hr)) {
            Logf("[SimXR] DX12 preview: CreateSwapChainForHwnd failed 0x%08X", (unsigned)hr);
            return;
        }
        sc1.As(&s.previewSwapchain12);
        s.previewBackbufferCount = desc.BufferCount;
        s.previewBackbuffers.clear();
        s.previewBackbuffers.resize(desc.BufferCount);

        // Create RTV heap
        D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{}; rtvDesc.NumDescriptors = desc.BufferCount; rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        if (FAILED(s.d3d12Device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(s.previewRTVHeap.GetAddressOf())))) {
            Log("[SimXR] DX12 preview: CreateDescriptorHeap RTV failed"); return;
        }
        s.previewRTVDescriptorSize = s.d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = s.previewRTVHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < desc.BufferCount; ++i) {
            if (FAILED(s.previewSwapchain12->GetBuffer(i, IID_PPV_ARGS(s.previewBackbuffers[i].GetAddressOf())))) {
                Logf("[SimXR] DX12 preview: GetBuffer %u failed", i); return;
            }
            s.d3d12Device->CreateRenderTargetView(s.previewBackbuffers[i].Get(), nullptr, rtvHandle);
            rtvHandle.ptr += s.previewRTVDescriptorSize;
        }
        // Command allocator/list
        s.d3d12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(s.previewCmdAlloc.GetAddressOf()));
        s.d3d12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, s.previewCmdAlloc.Get(), nullptr, IID_PPV_ARGS(s.previewCmdList.GetAddressOf()));
        s.previewCmdList->Close();
        // Fence
        s.d3d12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(s.previewFence.GetAddressOf()));
        s.previewFenceValue = 1;
        s.previewFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        Log("[SimXR] DX12 preview swapchain initialized");
    } else if (s.usesVulkan) {
        // Destroy existing preview swapchain if needed.
        if (s.vkPreviewSwapchain != VK_NULL_HANDLE) {
            // The per-image blit resources below belong to that swapchain's images, so they go with
            // it. Anything still in flight has to retire before its command buffer is freed.
            if (!s.vkBlitFences.empty()) {
                for (size_t i = 0; i < s.vkBlitFences.size(); ++i) {
                    if (s.vkBlitSubmitted[i]) {
                        vkWaitForFences(s.vkDevice, 1, &s.vkBlitFences[i], VK_TRUE, UINT64_MAX);
                    }
                }
            }
            if (!s.vkBlitCmds.empty()) {
                vkFreeCommandBuffers(s.vkDevice, s.vkCmdPool, (uint32_t)s.vkBlitCmds.size(), s.vkBlitCmds.data());
            }
            for (VkSemaphore sem : s.vkBlitDone) {
                if (sem != VK_NULL_HANDLE) vkDestroySemaphore(s.vkDevice, sem, nullptr);
            }
            for (VkFence fence : s.vkBlitFences) {
                if (fence != VK_NULL_HANDLE) vkDestroyFence(s.vkDevice, fence, nullptr);
            }
            s.vkPreviewImages.clear();
            s.vkBlitCmds.clear();
            s.vkBlitDone.clear();
            s.vkBlitFences.clear();
            s.vkBlitSubmitted.clear();

            vkDestroySwapchainKHR(s.vkDevice, s.vkPreviewSwapchain, nullptr);
            s.vkPreviewSwapchain = VK_NULL_HANDLE;
        }

        VkWin32SurfaceCreateInfoKHR surfaceInfo{};
        surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surfaceInfo.hinstance = GetModuleHandle(nullptr);
        surfaceInfo.hwnd = s.hwnd;

        VkResult result = vkCreateWin32SurfaceKHR(s.vkInstance, &surfaceInfo,  nullptr, &s.vkPreviewSurface);
        if (result != VK_SUCCESS) {
            Logf("[SimXR] ERROR: Failed to create Win32 surface (%d)", result);
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = s.vkPreviewSurface;            // VkSurfaceKHR for hwnd
        createInfo.minImageCount = 2;
        createInfo.imageFormat = (VkFormat)format;          // XR swapchain format
        createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        createInfo.imageExtent = { width, height };
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        result = vkCreateSwapchainKHR(s.vkDevice, &createInfo, nullptr, &s.vkPreviewSwapchain);
        if (result != VK_SUCCESS) {
            Logf("[SimXR] ERROR: Failed to create Vulkan preview swapchain with format %d", format);
        }
    } else {
        ComPtr<IDXGIDevice> dxgiDev; s.d3d11Device.As(&dxgiDev);
        ComPtr<IDXGIAdapter> adapter; dxgiDev->GetAdapter(adapter.GetAddressOf());
        ComPtr<IDXGIFactory2> factory; adapter->GetParent(IID_PPV_ARGS(factory.GetAddressOf()));
        DXGI_SWAP_CHAIN_DESC1 desc{}; 
        desc.Format = (DXGI_FORMAT)format;  // Use the format from the XR swapchain
        desc.Width = width; 
        desc.Height = height;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; 
        desc.BufferCount = 2; 
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; 
        desc.SampleDesc.Count = 1;
        HRESULT hr = factory->CreateSwapChainForHwnd(s.d3d11Device.Get(), s.hwnd, &desc, nullptr, nullptr, s.previewSwapchain.GetAddressOf());
        if (verboseLogging) Logf("[SimXR] ensurePreviewSized(DX11): hr=0x%08X swapchain=%p format=%d", (unsigned)hr, s.previewSwapchain.Get(), format);
        if (FAILED(hr)) {
            Logf("[SimXR] ERROR: Failed to create DX11 preview swapchain with format %d", format);
        }
    }
}

static void blitViewToHalf(rt::Session& s, rt::Swapchain& chain, uint32_t srcIndex, uint32_t arraySlice,
                           const XrRect2Di& rect, ID3D11RenderTargetView* rtv,
                           const D3D11_VIEWPORT& vp, ID3D11BlendState* blendState, bool isSyncEye = false) {

    //----------------
    //OXRWXR CHANGE:
    //----------------
    // Red sync for (DX11)
    int redIntensity = OpenXRFrameID;
    int blueIntensity = 0;

    if (!rtv) {
        Log("[SimXR] blitViewToHalf: rtv is null!");
        return;
    }

    if (!rt::InitBlitResources(s)) {
        Log("[SimXR] Cannot blit, blit resources failed to initialize.");
        return;
    }

    // Check if we have a valid image
    if (srcIndex >= chain.images.size()) {
        Logf("[SimXR] blitViewToHalf: srcIndex %u >= images.size() %zu", srcIndex, chain.images.size());
        return;
    }
    if (!chain.images[srcIndex]) {
        Logf("[SimXR] blitViewToHalf: images[%u] is null", srcIndex);
        return;
    }

    // Prepare source texture
    ComPtr<ID3D11Texture2D> sourceTexture = chain.images[srcIndex];
    D3D11_TEXTURE2D_DESC srcDesc;
    sourceTexture->GetDesc(&srcDesc);

    // A zero-sized rect is invalid in OpenXR, but Maquette submits one; use the whole image
    XrRect2Di srcRect = rect;
    if (srcRect.extent.width == 0 || srcRect.extent.height == 0) {
        srcRect.offset = {0, 0};
        srcRect.extent = {(int32_t)srcDesc.Width, (int32_t)srcDesc.Height};
    }
    
    // Skip depth formats - they can't be rendered to the preview window
    bool isDepthFormat = (srcDesc.Format == DXGI_FORMAT_D32_FLOAT ||
                          srcDesc.Format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT ||
                          srcDesc.Format == DXGI_FORMAT_D24_UNORM_S8_UINT ||
                          srcDesc.Format == DXGI_FORMAT_D16_UNORM ||
                          srcDesc.Format == DXGI_FORMAT_R32_TYPELESS ||
                          srcDesc.Format == DXGI_FORMAT_R32G8X24_TYPELESS ||
                          srcDesc.Format == DXGI_FORMAT_R24G8_TYPELESS ||
                          srcDesc.Format == DXGI_FORMAT_R16_TYPELESS);
    if (isDepthFormat) {
        // Depth swapchains are for depth testing, not preview rendering
        static int depthSkipCount = 0;
        if (++depthSkipCount % 60 == 1 && verboseLogging) {
            Logf("[SimXR] blitViewToHalf: Skipping depth format %d", srcDesc.Format);
        }
        return;
    }

    //----------------
    //OXRWXR CHANGE:
    //----------------
    // Red sync pixel (DX11) order flipping
    bool flipColorOrder = false;

    // Choose a typed format for SRV and temp texture
    // Preserve sRGB if original was sRGB to enable auto-conversion
    DXGI_FORMAT typedFormat = srcDesc.Format;
    switch (srcDesc.Format) {
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
            typedFormat = (chain.format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB) ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
            break;
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
            typedFormat = (chain.format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB) ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB : DXGI_FORMAT_B8G8R8A8_UNORM;
            flipColorOrder = true;
            break;
        case DXGI_FORMAT_R16G16B16A16_TYPELESS: 
            typedFormat = DXGI_FORMAT_R16G16B16A16_FLOAT; 
            break;
        case DXGI_FORMAT_R32G32B32A32_TYPELESS: 
            typedFormat = DXGI_FORMAT_R32G32B32A32_FLOAT; 
            break;
        case DXGI_FORMAT_R10G10B10A2_TYPELESS: 
            typedFormat = DXGI_FORMAT_R10G10B10A2_UNORM; 
            break;
        default: 
            break; // Already typed or unknown
    }

    //----------------
    //OXRWXR CHANGE:
    //----------------
    // Red sync pixel (DX11)
    if (isSyncEye) {
        D3D11_BOX redBox;
        redBox.left = 0;
        redBox.top = srcRect.extent.height < 0 ? abs(srcRect.extent.height) - 10 : 0;
        redBox.right = 10;
        redBox.bottom = srcRect.extent.height < 0 ? abs(srcRect.extent.height) : 10;
        redBox.front = 0;
        redBox.back = 1;

        UINT pitch = 40; // 10 * 4
        BYTE data[10 * 10 * 4];

        if (flipColorOrder) {
            for (int i = 0; i < 10 * 10; i++) {
                data[i * 4 + 0] = 0;   // B
                data[i * 4 + 1] = 0;   // G
                data[i * 4 + 2] = OpenXRFrameID;   // R
                data[i * 4 + 3] = 255; // A
            }
        } else {
            for (int i = 0; i < 10 * 10; i++) {
                data[i * 4 + 0] = OpenXRFrameID; // R
                data[i * 4 + 1] = 0;   // G
                data[i * 4 + 2] = 0;   // B
                data[i * 4 + 3] = 255; // A
            }
        }

        s.d3d11Context->UpdateSubresource(sourceTexture.Get(), 0, &redBox, data, pitch, 0);
    }

    // Create Shader Resource View
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = typedFormat; // Use the proper typed format
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;

    ComPtr<ID3D11ShaderResourceView> srv;
    HRESULT hr = s.d3d11Device->CreateShaderResourceView(sourceTexture.Get(), &srvDesc, srv.GetAddressOf());
    if (FAILED(hr)) {
        Logf("[SimXR] Failed to create SRV: 0x%08X", hr);
        return;
    }

    s.d3d11Context->RSSetViewports(1, &vp);

    // Set shaders and resources
    s.d3d11Context->VSSetShader(s.blitVS.Get(), nullptr, 0);
    s.d3d11Context->PSSetShader(s.blitPS.Get(), nullptr, 0);

    // Update blit constants
    rt::BlitConstants blitConstants = {};
    blitConstants.uvMinX = static_cast<float>(srcRect.offset.x) / static_cast<float>(srcDesc.Width);
    blitConstants.uvMinY = static_cast<float>(srcRect.offset.y) / static_cast<float>(srcDesc.Height);
    blitConstants.uvMaxX = static_cast<float>(srcRect.offset.x + srcRect.extent.width) / static_cast<float>(srcDesc.Width);
    blitConstants.uvMaxY = static_cast<float>(srcRect.offset.y + srcRect.extent.height) / static_cast<float>(srcDesc.Height);
    while (std::min(blitConstants.uvMinY, blitConstants.uvMaxY) < 0) { blitConstants.uvMaxY += 1.0f; blitConstants.uvMinY += 1.0f; }
    while (std::max(blitConstants.uvMinY, blitConstants.uvMaxY) > 1) { blitConstants.uvMaxY -= 1.0f; blitConstants.uvMinY -= 1.0f; }

    s.d3d11Context->UpdateSubresource(s.blitConstantBuffer.Get(), 0, nullptr, &blitConstants, 0, 0);
    ID3D11Buffer* constantBuffers[] = { s.blitConstantBuffer.Get() };
    s.d3d11Context->PSSetConstantBuffers(0, 1, constantBuffers);

    ID3D11ShaderResourceView* srvs[] = { srv.Get() };
    s.d3d11Context->PSSetShaderResources(0, 1, srvs);
    ID3D11SamplerState* samplers[] = { s.samplerState.Get() };
    s.d3d11Context->PSSetSamplers(0, 1, samplers);

    // Set pipeline state
    s.d3d11Context->IASetInputLayout(nullptr);
    s.d3d11Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    s.d3d11Context->OMSetBlendState(blendState, nullptr, 0xFFFFFFFF);
    s.d3d11Context->OMSetDepthStencilState(nullptr, 0);
    s.d3d11Context->RSSetState(s.noCullRS.Get());  // Use no-cull rasterizer state to prevent triangle culling

    // Bind render target
    ID3D11RenderTargetView* rtvs[1] = { rtv };
    s.d3d11Context->OMSetRenderTargets(1, rtvs, nullptr);

    // Draw fullscreen quad
    s.d3d11Context->Draw(4, 0);
    
    // Unbind SRV to avoid conflicts with future RTV usage
    ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
    s.d3d11Context->PSSetShaderResources(0, 1, nullSRV);
    
    static int debugCount = 0;
    if (++debugCount % 120 == 1) {
        Logf("[SimXR] blitViewToHalf: srcIdx=%u slice=%u typedFmt=%d srcFmt=%d",
             srcIndex, arraySlice, typedFormat, srcDesc.Format);
        Logf("[SimXR]   viewport: x=%.0f y=%.0f w=%.0f h=%.0f", vp.TopLeftX, vp.TopLeftY, vp.Width, vp.Height);
        Logf("[SimXR]   viewrect: x1=%.2f y1=%.2f x2=%.2f y2=%.2f", blitConstants.uvMinX, blitConstants.uvMinY, blitConstants.uvMaxX, blitConstants.uvMaxY);
    }
}

// D3D12 blit function - copies swapchain textures to preview backbuffer
static void blitD3D12ToPreview(rt::Session& s,
                                rt::Swapchain& chainL, uint32_t leftIdx, uint32_t leftSlice,
                                rt::Swapchain* chainR, uint32_t rightIdx, uint32_t rightSlice) {
    if (!s.previewSwapchain12 || !s.previewCmdList || !s.previewCmdAlloc) {
        Log("[SimXR] blitD3D12ToPreview: Missing D3D12 preview resources");
        return;
    }

    // Skip depth-only swapchains
    bool isDepthFormat = (chainL.format == DXGI_FORMAT_D32_FLOAT ||
                          chainL.format == DXGI_FORMAT_D24_UNORM_S8_UINT ||
                          chainL.format == DXGI_FORMAT_D16_UNORM);
    if (isDepthFormat) {
        return;
    }

    // Allow multiple frames in flight before stalling the CPU.
    // This reduces CPU/GPU synchronization and improves throughput.
    constexpr UINT64 MaxFramesInFlight = 3;
    if (s.previewFenceValue > MaxFramesInFlight) {
        const UINT64 targetFence = s.previewFenceValue - MaxFramesInFlight;
        const UINT64 completedFence = s.previewFence->GetCompletedValue();

        if (completedFence < targetFence) {
            HRESULT hr = s.previewFence->SetEventOnCompletion(targetFence, s.previewFenceEvent);
            if (SUCCEEDED(hr)) {
                WaitForSingleObject(s.previewFenceEvent, INFINITE);
            }
        }
    }

    // Get current backbuffer index
    UINT bbIndex = s.previewSwapchain12->GetCurrentBackBufferIndex();
    ID3D12Resource* backbuffer = s.previewBackbuffers[bbIndex].Get();
    if (!backbuffer) {
        Log("[SimXR] blitD3D12ToPreview: No backbuffer");
        return;
    }

    // Create a render target view descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = 1;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    s.d3d12Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&s.previewRTVHeap));

    // Create a render target view descriptor for backbuffer
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    s.d3d12Device->CreateRenderTargetView(backbuffer, &rtvDesc, s.previewRTVHeap->GetCPUDescriptorHandleForHeapStart());

    // Reset command allocator and list
    HRESULT hr = s.previewCmdAlloc->Reset();
    if (FAILED(hr)) {
        Logf("[SimXR] blitD3D12ToPreview: CmdAlloc Reset failed 0x%08X", hr);
        return;
    }
    hr = s.previewCmdList->Reset(s.previewCmdAlloc.Get(), nullptr);
    if (FAILED(hr)) {
        Logf("[SimXR] blitD3D12ToPreview: CmdList Reset failed 0x%08X", hr);
        return;
    }

    // Transition backbuffer to copy dest
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = backbuffer;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    s.previewCmdList->ResourceBarrier(1, &barrier);

    auto transition = [&](ID3D12Resource* res, D3D12_RESOURCE_STATES& state, D3D12_RESOURCE_STATES newState) {
        if (!res || state == newState) return;
        D3D12_RESOURCE_BARRIER b = {};
        b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition.pResource = res;
        b.Transition.StateBefore = state;
        b.Transition.StateAfter = newState;
        b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        s.previewCmdList->ResourceBarrier(1, &b);
        state = newState;
    };

    auto calcSubresource = [](const rt::Swapchain& chain, uint32_t slice, const char* label) -> UINT {
        uint32_t arraySize = chain.arraySize ? chain.arraySize : 1;
        uint32_t mipLevels = chain.mipCount ? chain.mipCount : 1;
        if (slice >= arraySize) {
            Logf("[SimXR] blitD3D12ToPreview: %s slice %u out of range (arraySize=%u)", label, slice, arraySize);
            return UINT_MAX;
        }
        return D3D12CalcSubresource(0, slice, 0, mipLevels, arraySize);
    };

    auto copyEye = [&](rt::Swapchain& chain, uint32_t idx, uint32_t slice,
        UINT dstX, UINT dstY, const char* label, bool isSyncEye = false) -> bool {
            //----------------
            //OXRWXR CHANGE:
            //----------------
            // Now with red sync for (DX12)
            float redIntensity = (OpenXRFrameID / 255.0f);
            float blueIntensity = 0.0f;

            if (idx >= chain.images12.size() || !chain.images12[idx]) return false;
            if (chain.imageStates12.size() <= idx) {
                Logf("[SimXR] blitD3D12ToPreview: %s missing state tracking", label);
                return false;
            }
            UINT subresource = calcSubresource(chain, slice, label);
            if (subresource == UINT_MAX) return false;

            ID3D12Resource* srcTex = chain.images12[idx].Get();
            D3D12_RESOURCE_STATES prevState = chain.imageStates12[idx];
            transition(srcTex, chain.imageStates12[idx], D3D12_RESOURCE_STATE_COPY_SOURCE);

            D3D12_TEXTURE_COPY_LOCATION dst = {};
            dst.pResource = backbuffer;
            dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            dst.SubresourceIndex = 0;

            D3D12_TEXTURE_COPY_LOCATION src = {};
            src.pResource = srcTex;
            src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            src.SubresourceIndex = subresource;

            D3D12_BOX srcBox = {};
            srcBox.left = 0;
            srcBox.top = 0;
            srcBox.front = 0;
            srcBox.right = chain.width;
            srcBox.bottom = chain.height;
            srcBox.back = 1;

            s.previewCmdList->CopyTextureRegion(&dst, dstX, dstY, 0, &src, &srcBox);

            if (isSyncEye) {
                D3D12_RESOURCE_STATES backbufferState = D3D12_RESOURCE_STATE_PRESENT;
                transition(backbuffer, backbufferState, D3D12_RESOURCE_STATE_RENDER_TARGET);

                float red[4] = { redIntensity, 0.0f, blueIntensity, 1.0f };
                D3D12_RECT rect = { 0, 0, 10, 10 };
                D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = s.previewRTVHeap->GetCPUDescriptorHandleForHeapStart();
                s.previewCmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
                s.previewCmdList->ClearRenderTargetView(rtvHandle, red, 1, &rect);

                transition(backbuffer, backbufferState, D3D12_RESOURCE_STATE_PRESENT);
            }

            transition(srcTex, chain.imageStates12[idx], prevState);
            return true;
        };

    const bool hasLeft = leftIdx < chainL.images12.size() && chainL.images12[leftIdx];
    const bool hasRight = chainR && rightIdx < chainR->images12.size() && chainR->images12[rightIdx];

    // Single-eye mode: render selected eye full-screen
    UINT rightX = (UINT)(s.previewWidth / 2);
    UINT rightY = 0;

    if (hasLeft) {
        copyEye(chainL, leftIdx, leftSlice, 0, 0, "L", true);
    }
    if (hasRight) {
        copyEye(*chainR, rightIdx, rightSlice, rightX, rightY, "R", false);
    } else if (hasLeft) {
        copyEye(chainL, leftIdx, leftSlice, rightX, rightY, "L", false);
    }

    // Transition backbuffer back to present
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    s.previewCmdList->ResourceBarrier(1, &barrier);

    // Close and execute
    s.previewCmdList->Close();
    ID3D12CommandList* cmdLists[] = { s.previewCmdList.Get() };
    s.d3d12Queue->ExecuteCommandLists(1, cmdLists);

    // Signal fence
    s.d3d12Queue->Signal(s.previewFence.Get(), s.previewFenceValue++);

    static int blitCount = 0;
    if (++blitCount % 60 == 1) {
        Logf("[SimXR] blitD3D12ToPreview: Copied L[%u] R[%u] to backbuffer %u", leftIdx, rightIdx, bbIndex);
    }
}

// Flag to track if Present should be called (deferred until all layers rendered)
static bool g_presentPending = false;

// ---- Vulkan CPU readback helper ----
// Copies a VkImage to a CPU buffer via a host-visible staging VkBuffer.
// Returns false on any Vulkan error.
static bool VulkanReadbackPixels(
    VkDevice device, VkPhysicalDevice physDevice,
    VkCommandPool cmdPool, VkQueue queue,
    VkImage image, VkFormat fmt,
    uint32_t w, uint32_t h,
    std::vector<uint8_t>& out)
{
    // Only RGBA8 and BGRA8 variants handled for the preview display
    // Depth formats are skipped
    bool isDepth = (fmt == VK_FORMAT_D32_SFLOAT || fmt == VK_FORMAT_D24_UNORM_S8_UINT ||
                    fmt == VK_FORMAT_D16_UNORM  || fmt == VK_FORMAT_D32_SFLOAT_S8_UINT);
    if (isDepth) return false;

    const VkDeviceSize bufSize = (VkDeviceSize)w * h * 4;
    out.resize(bufSize, 0);

    // Find host-visible + host-coherent memory type
    auto findHostMem = [&](uint32_t typeBits) -> uint32_t {
        VkPhysicalDeviceMemoryProperties mp{};
        vkGetPhysicalDeviceMemoryProperties(physDevice, &mp);
        VkMemoryPropertyFlags needed = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        for (uint32_t i = 0; i < mp.memoryTypeCount; i++) {
            if ((typeBits & (1u << i)) && (mp.memoryTypes[i].propertyFlags & needed) == needed)
                return i;
        }
        return UINT32_MAX;
    };

    // Create staging buffer
    VkBufferCreateInfo bufInfo{};
    bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufInfo.size  = bufSize;
    bufInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkBuffer stagingBuf = VK_NULL_HANDLE;
    if (vkCreateBuffer(device, &bufInfo, nullptr, &stagingBuf) != VK_SUCCESS) return false;

    VkMemoryRequirements bufMemReq{};
    vkGetBufferMemoryRequirements(device, stagingBuf, &bufMemReq);
    uint32_t memIdx = findHostMem(bufMemReq.memoryTypeBits);
    if (memIdx == UINT32_MAX) { vkDestroyBuffer(device, stagingBuf, nullptr); return false; }

    VkMemoryAllocateInfo stagingAlloc{};
    stagingAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    stagingAlloc.allocationSize = bufMemReq.size;
    stagingAlloc.memoryTypeIndex = memIdx;
    VkDeviceMemory stagingMem = VK_NULL_HANDLE;
    if (vkAllocateMemory(device, &stagingAlloc, nullptr, &stagingMem) != VK_SUCCESS) {
        vkDestroyBuffer(device, stagingBuf, nullptr); return false;
    }
    vkBindBufferMemory(device, stagingBuf, stagingMem, 0);

    // Allocate a one-shot command buffer
    VkCommandBufferAllocateInfo cbAlloc{};
    cbAlloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbAlloc.commandPool = cmdPool;
    cbAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbAlloc.commandBufferCount = 1;
    VkCommandBuffer cb = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(device, &cbAlloc, &cb) != VK_SUCCESS) {
        vkFreeMemory(device, stagingMem, nullptr);
        vkDestroyBuffer(device, stagingBuf, nullptr);
        return false;
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cb, &beginInfo);

    // Transition image: UNDEFINED/COLOR_ATTACHMENT → TRANSFER_SRC
    VkImageMemoryBarrier toSrc{};
    toSrc.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toSrc.srcAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    toSrc.dstAccessMask       = VK_ACCESS_TRANSFER_READ_BIT;
    toSrc.oldLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    toSrc.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    toSrc.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toSrc.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toSrc.image               = image;
    toSrc.subresourceRange    = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    vkCmdPipelineBarrier(cb,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &toSrc);

    // Copy image → buffer
    VkBufferImageCopy region{};
    region.bufferOffset      = 0;
    region.bufferRowLength   = 0;  // tightly packed
    region.bufferImageHeight = 0;
    region.imageSubresource  = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    region.imageOffset       = { 0, 0, 0 };
    region.imageExtent       = { w, h, 1 };
    vkCmdCopyImageToBuffer(cb, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuf, 1, &region);

    // Transition image back to COLOR_ATTACHMENT_OPTIMAL
    VkImageMemoryBarrier toColor{};
    toColor.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toColor.srcAccessMask       = VK_ACCESS_TRANSFER_READ_BIT;
    toColor.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    toColor.oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    toColor.newLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    toColor.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toColor.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toColor.image               = image;
    toColor.subresourceRange    = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    vkCmdPipelineBarrier(cb,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0, 0, nullptr, 0, nullptr, 1, &toColor);

    vkEndCommandBuffer(cb);

    // Submit and wait
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence fence = VK_NULL_HANDLE;
    vkCreateFence(device, &fenceInfo, nullptr, &fence);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cb;
    vkQueueSubmit(queue, 1, &submitInfo, fence);
    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
    vkDestroyFence(device, fence, nullptr);

    vkFreeCommandBuffers(device, cmdPool, 1, &cb);

    // Map and copy pixel data
    void* mapped = nullptr;
    vkMapMemory(device, stagingMem, 0, bufSize, 0, &mapped);
    memcpy(out.data(), mapped, bufSize);
    vkUnmapMemory(device, stagingMem);

    vkFreeMemory(device, stagingMem, nullptr);
    vkDestroyBuffer(device, stagingBuf, nullptr);
    return true;
}

static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1u << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) return i;
    }

    return UINT32_MAX;
}

static bool initVulkanSyncBuffer(rt::Session& s)
{
    constexpr VkDeviceSize size = 10 * 10 * 4;

    if (s.vkSyncBuffer != VK_NULL_HANDLE && s.vkSyncBufferMemory != VK_NULL_HANDLE) return true;
    if (s.vkDevice == VK_NULL_HANDLE || s.vkPhysicalDevice == VK_NULL_HANDLE) {
        Logf("[SimXR] Cannot create sync buffer: Vulkan device/physical device is null");
        return false;
    }

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(s.vkDevice, &bufferInfo, nullptr, &s.vkSyncBuffer);
    if (result != VK_SUCCESS) {
        Logf("[SimXR] vkCreateBuffer(sync buffer) failed: %d", (int)result);
        s.vkSyncBuffer = VK_NULL_HANDLE;
        return false;
    }

    VkMemoryRequirements memReq{};
    vkGetBufferMemoryRequirements(s.vkDevice, s.vkSyncBuffer, &memReq);

    uint32_t memoryType = findMemoryType(s.vkPhysicalDevice, memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (memoryType == UINT32_MAX) {
        Logf("[SimXR] No HOST_VISIBLE | HOST_COHERENT memory type for sync buffer");
        vkDestroyBuffer(s.vkDevice, s.vkSyncBuffer, nullptr);
        s.vkSyncBuffer = VK_NULL_HANDLE;
        return false;
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = memoryType;

    result = vkAllocateMemory(s.vkDevice, &allocInfo, nullptr, &s.vkSyncBufferMemory);
    if (result != VK_SUCCESS) {
        Logf("[SimXR] vkAllocateMemory(sync buffer) failed: %d", (int)result);
        vkDestroyBuffer(s.vkDevice, s.vkSyncBuffer, nullptr);
        s.vkSyncBuffer = VK_NULL_HANDLE;
        s.vkSyncBufferMemory = VK_NULL_HANDLE;
        return false;
    }

    result = vkBindBufferMemory(s.vkDevice, s.vkSyncBuffer, s.vkSyncBufferMemory, 0);
    if (result != VK_SUCCESS) {
        Logf("[SimXR] vkBindBufferMemory(sync buffer) failed: %d", (int)result);
        vkFreeMemory(s.vkDevice, s.vkSyncBufferMemory, nullptr);
        vkDestroyBuffer(s.vkDevice, s.vkSyncBuffer, nullptr);
        s.vkSyncBufferMemory = VK_NULL_HANDLE;
        s.vkSyncBuffer = VK_NULL_HANDLE;
        return false;
    }

    Logf("[SimXR] Vulkan sync buffer initialized");
    return true;
}

static void presentVulkanSwapchain(rt::Session& s, const XrCompositionLayerProjection& proj)
{
    // Get left eye swapchain and image index
    const auto& vL = proj.views[0];
    auto itL = rt::g_swapchains.find(vL.subImage.swapchain);
    if (itL == rt::g_swapchains.end()) return;
    rt::Swapchain chL = itL->second;

    uint32_t leftIdx = (chL.lastReleased != UINT32_MAX) ? chL.lastReleased :
                       (chL.lastAcquired != UINT32_MAX) ? chL.lastAcquired : 0;

    // Get right eye
    uint32_t rightIdx = leftIdx;
    rt::Swapchain* chR = &chL;
    if (proj.viewCount > 1) {
        const auto& vR = proj.views[1];
        auto itR = rt::g_swapchains.find(vR.subImage.swapchain);
        if (itR != rt::g_swapchains.end()) {
            chR = &itR->second;
            rightIdx = (chR->lastReleased != UINT32_MAX) ? chR->lastReleased :
                       (chR->lastAcquired != UINT32_MAX) ? chR->lastAcquired : 0;
        }
    }

    uint32_t eyeWidth = proj.views[0].subImage.imageRect.extent.width;
    uint32_t eyeHeight = proj.views[0].subImage.imageRect.extent.height;
    VkFormat displayFmt = VK_FORMAT_B8G8R8A8_UNORM;
    ensurePreviewSized(s, eyeWidth * 2, eyeHeight, displayFmt);
    if (!s.vkPreviewSwapchain) { Log("[SimXR] VK PREVIEW: No preview swapchain"); return; }


    VkImage leftImage = chL.imagesVK[leftIdx];
    VkImage rightImage = chR->imagesVK[rightIdx];

    // Make sure the required Vulkan objects are valid before doing anything.
    if (s.vkDevice == VK_NULL_HANDLE) {
        Logf("[SimXR] vkDevice is VK_NULL_HANDLE");
        return;
    } else if (s.vkPreviewSwapchain == VK_NULL_HANDLE) {
        Logf("[SimXR] vkPreviewSwapchain is VK_NULL_HANDLE");
        return;
    } else if (s.vkQueue == VK_NULL_HANDLE) {
        Logf("[SimXR] vkQueue is VK_NULL_HANDLE");
        return;
    } else if (s.vkCmdPool == VK_NULL_HANDLE) {
        Logf("[SimXR] vkCmdPool is VK_NULL_HANDLE");
        return;
    } else if (leftImage == VK_NULL_HANDLE) {
        Logf("[SimXR] leftImage is VK_NULL_HANDLE");
        return;
    } else if (eyeWidth == 0 || eyeHeight == 0) {
        Logf("[SimXR] Invalid eye dimensions: %ux%u", eyeWidth, eyeHeight);
        return;
    }

    if (s.vkAcquireFence == VK_NULL_HANDLE) {
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        if (VK_SUCCESS != vkCreateFence(s.vkDevice, &fenceInfo, nullptr, &s.vkAcquireFence)) {
            Logf("[SimXR] Failed to call vkCreateFence");
            s.vkAcquireFence = VK_NULL_HANDLE;
            return;
        }
    }

    // Get preview swapchain images. Enumerated once per swapchain rather than per frame, and the
    // per-image command buffer, semaphore and fence are created alongside them.
    VkResult result;
    if (s.vkPreviewImages.empty()) {
        uint32_t imageCount = 0;
        result = vkGetSwapchainImagesKHR(s.vkDevice, s.vkPreviewSwapchain, &imageCount, nullptr);
        if (result != VK_SUCCESS || imageCount == 0) {
            Logf("[SimXR] vkGetSwapchainImagesKHR failed: %d", (int)result);
            return;
        }

        std::vector<VkImage> previewImages(imageCount);
        result = vkGetSwapchainImagesKHR(s.vkDevice, s.vkPreviewSwapchain, &imageCount, previewImages.data());
        if (result != VK_SUCCESS) {
            Logf("[SimXR] vkGetSwapchainImagesKHR failed: %d", (int)result);
            return;
        }

        VkCommandBufferAllocateInfo cmdAlloc{};
        cmdAlloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdAlloc.commandPool = s.vkCmdPool;
        cmdAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdAlloc.commandBufferCount = imageCount;

        std::vector<VkCommandBuffer> cmds(imageCount, VK_NULL_HANDLE);
        result = vkAllocateCommandBuffers(s.vkDevice, &cmdAlloc, cmds.data());
        if (result != VK_SUCCESS) {
            Logf("[SimXR] vkAllocateCommandBuffers failed: %d", (int)result);
            return;
        }

        std::vector<VkSemaphore> semaphores(imageCount, VK_NULL_HANDLE);
        std::vector<VkFence> fences(imageCount, VK_NULL_HANDLE);
        for (uint32_t i = 0; i < imageCount; ++i) {
            VkSemaphoreCreateInfo semInfo{};
            semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            VkFenceCreateInfo fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            if (vkCreateSemaphore(s.vkDevice, &semInfo, nullptr, &semaphores[i]) != VK_SUCCESS ||
                vkCreateFence(s.vkDevice, &fenceInfo, nullptr, &fences[i]) != VK_SUCCESS) {
                Logf("[SimXR] Failed to create preview blit sync objects");
                for (uint32_t j = 0; j <= i; ++j) {
                    if (semaphores[j] != VK_NULL_HANDLE) vkDestroySemaphore(s.vkDevice, semaphores[j], nullptr);
                    if (fences[j] != VK_NULL_HANDLE) vkDestroyFence(s.vkDevice, fences[j], nullptr);
                }
                vkFreeCommandBuffers(s.vkDevice, s.vkCmdPool, imageCount, cmds.data());
                return;
            }
        }

        s.vkPreviewImages = std::move(previewImages);
        s.vkBlitCmds = std::move(cmds);
        s.vkBlitDone = std::move(semaphores);
        s.vkBlitFences = std::move(fences);
        s.vkBlitSubmitted.assign(imageCount, false);
    }
    const std::vector<VkImage>& previewImages = s.vkPreviewImages;

    // Acquire preview swapchain image
    result = vkResetFences(s.vkDevice, 1, &s.vkAcquireFence);
    if (result != VK_SUCCESS) {
        Logf("[SimXR] vkResetFences failed: %d", (int)result);
        return;
    }

    uint32_t imageIndex = 0;
    result = vkAcquireNextImageKHR(s.vkDevice, s.vkPreviewSwapchain, UINT64_MAX, VK_NULL_HANDLE, s.vkAcquireFence, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        Logf("[SimXR] vkAcquireNextImageKHR: swapchain is out of date.");
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        Logf("[SimXR] vkAcquireNextImageKHR failed: %d", (int)result);
        return;
    }

    result = vkWaitForFences(s.vkDevice, 1, &s.vkAcquireFence, VK_TRUE, UINT64_MAX);
    if (result != VK_SUCCESS) {
        Logf("[SimXR] vkWaitForFences failed: %d", (int)result);
        return;
    } else if (imageIndex >= previewImages.size()) {
        Logf("[SimXR] Invalid preview image index: %u", imageIndex);
        return;
    }

    VkImage previewImage = previewImages[imageIndex];

    // This image's command buffer is reused, so wait for its previous submission to retire before
    // recording into it again. That is a wait on one frame's blit rather than on the whole queue.
    VkCommandBuffer cmd = s.vkBlitCmds[imageIndex];
    if (s.vkBlitSubmitted[imageIndex]) {
        result = vkWaitForFences(s.vkDevice, 1, &s.vkBlitFences[imageIndex], VK_TRUE, UINT64_MAX);
        if (result != VK_SUCCESS) {
            Logf("[SimXR] vkWaitForFences (blit) failed: %d", (int)result);
            return;
        }
    }
    result = vkResetFences(s.vkDevice, 1, &s.vkBlitFences[imageIndex]);
    if (result != VK_SUCCESS) {
        Logf("[SimXR] vkResetFences (blit) failed: %d", (int)result);
        return;
    }
    s.vkBlitSubmitted[imageIndex] = false;

    result = vkResetCommandBuffer(cmd, 0);
    if (result != VK_SUCCESS) {
        Logf("[SimXR] vkResetCommandBuffer failed: %d", (int)result);
        return;
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    result = vkBeginCommandBuffer(cmd, &beginInfo);
    if (result != VK_SUCCESS) {
        Logf("[SimXR] vkBeginCommandBuffer failed: %d", (int)result);
        return;
    }

    // Preview image -> transfer destination
    VkImageMemoryBarrier previewBarrier{};
    previewBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    previewBarrier.srcAccessMask = 0;
    previewBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    previewBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    previewBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    previewBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    previewBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    previewBarrier.image = previewImage;
    previewBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    previewBarrier.subresourceRange.baseMipLevel = 0;
    previewBarrier.subresourceRange.levelCount = 1;
    previewBarrier.subresourceRange.baseArrayLayer = 0;
    previewBarrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &previewBarrier);

    // -----------------------------------------------------------------------------
    // Sync pixel: write 10x10 red pixels directly into leftImage.
    // -----------------------------------------------------------------------------

    float redIntensity = static_cast<float>(OpenXRFrameID & 0xFF) / 255.0f;
    if (s.vkSyncBuffer == VK_NULL_HANDLE) {
        initVulkanSyncBuffer(s);
    }
    if (s.vkSyncBuffer == VK_NULL_HANDLE || s.vkSyncBufferMemory == VK_NULL_HANDLE) {
        Logf("[SimXR] VK sync buffer is not initialized");
        vkEndCommandBuffer(cmd);
        return;
    }

    void* syncMapped = nullptr;
    result = vkMapMemory(s.vkDevice, s.vkSyncBufferMemory, 0, 10 * 10 * 4, 0, &syncMapped);
    if (result != VK_SUCCESS || !syncMapped) {
        Logf("[SimXR] vkMapMemory(sync buffer) failed: %d", (int)result);
        vkEndCommandBuffer(cmd);
        return;
    }

    uint8_t r = static_cast<uint8_t>(redIntensity * 255.0f);
    uint8_t* pixels = static_cast<uint8_t*>(syncMapped);

    for (uint32_t y = 0; y < 10; ++y) {
        for (uint32_t x = 0; x < 10; ++x) {
            pixels[(y * 10 + x) * 4 + 0] = r;
            pixels[(y * 10 + x) * 4 + 1] = 0;
            pixels[(y * 10 + x) * 4 + 2] = 0;
            pixels[(y * 10 + x) * 4 + 3] = 255;
        }
    }
    vkUnmapMemory(s.vkDevice, s.vkSyncBufferMemory);

    VkImageMemoryBarrier syncBegin{};
    syncBegin.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    syncBegin.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    syncBegin.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    syncBegin.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    syncBegin.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    syncBegin.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    syncBegin.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    syncBegin.image = leftImage;
    syncBegin.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    syncBegin.subresourceRange.baseMipLevel = 0;
    syncBegin.subresourceRange.levelCount = 1;
    syncBegin.subresourceRange.baseArrayLayer = 0;
    syncBegin.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &syncBegin);

    VkBufferImageCopy syncCopy{};
    syncCopy.bufferOffset = 0;
    syncCopy.bufferRowLength = 10;
    syncCopy.bufferImageHeight = 10;
    syncCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    syncCopy.imageSubresource.mipLevel = 0;
    syncCopy.imageSubresource.baseArrayLayer = 0;
    syncCopy.imageSubresource.layerCount = 1;
    syncCopy.imageOffset = { 0, 0, 0 };
    syncCopy.imageExtent = { 10, 10, 1 };

    vkCmdCopyBufferToImage(cmd, s.vkSyncBuffer, leftImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &syncCopy);

    VkImageMemoryBarrier syncEnd{};
    syncEnd.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    syncEnd.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    syncEnd.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    syncEnd.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    syncEnd.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    syncEnd.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    syncEnd.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    syncEnd.image = leftImage;
    syncEnd.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    syncEnd.subresourceRange.baseMipLevel = 0;
    syncEnd.subresourceRange.levelCount = 1;
    syncEnd.subresourceRange.baseArrayLayer = 0;
    syncEnd.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &syncEnd);

    // Blit left eye
    VkImageBlit leftBlit{};
    leftBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    leftBlit.srcSubresource.mipLevel = 0;
    leftBlit.srcSubresource.baseArrayLayer = 0;
    leftBlit.srcSubresource.layerCount = 1;
    leftBlit.srcOffsets[0] = { 0, 0, 0 };
    leftBlit.srcOffsets[1] = { (int32_t)eyeWidth, (int32_t)eyeHeight, 1 };
    leftBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    leftBlit.dstSubresource.mipLevel = 0;
    leftBlit.dstSubresource.baseArrayLayer = 0;
    leftBlit.dstSubresource.layerCount = 1;
    leftBlit.dstOffsets[0] = { 0, 0, 0 };
    leftBlit.dstOffsets[1] = { monoRendering ? (int32_t)ViewportWidth : (int32_t)ViewportWidth / 2, (int32_t)ViewportHeight, 1 };

    vkCmdBlitImage(cmd, leftImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, previewImage,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &leftBlit, VK_FILTER_LINEAR);

    // Right eye
    if ((rightImage != VK_NULL_HANDLE) && !monoRendering) {

        VkImageMemoryBarrier rightBarrier{};
        rightBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        rightBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        rightBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        rightBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        rightBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        rightBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        rightBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        rightBarrier.image = rightImage;
        rightBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        rightBarrier.subresourceRange.baseMipLevel = 0;
        rightBarrier.subresourceRange.levelCount = 1;
        rightBarrier.subresourceRange.baseArrayLayer = 0;
        rightBarrier.subresourceRange.layerCount = 1;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &rightBarrier);

        // Blit right eye
        VkImageBlit rightBlit{};
        rightBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        rightBlit.srcSubresource.mipLevel = 0;
        rightBlit.srcSubresource.baseArrayLayer = 0;
        rightBlit.srcSubresource.layerCount = 1;
        rightBlit.srcOffsets[0] = { 0, 0, 0 };
        rightBlit.srcOffsets[1] = { (int32_t)eyeWidth, (int32_t)eyeHeight, 1 };
        rightBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        rightBlit.dstSubresource.mipLevel = 0;
        rightBlit.dstSubresource.baseArrayLayer = 0;
        rightBlit.dstSubresource.layerCount = 1;
        rightBlit.dstOffsets[0] = {(int32_t)ViewportWidth / 2, 0, 0 };
        rightBlit.dstOffsets[1] = { (int32_t)ViewportWidth, (int32_t)ViewportHeight, 1 };

        vkCmdBlitImage(cmd, rightImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, previewImage,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &rightBlit, VK_FILTER_LINEAR);
    }

    // Preview image -> present
    VkImageMemoryBarrier presentBarrier{};
    presentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    presentBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    presentBarrier.dstAccessMask = 0;
    presentBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    presentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    presentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    presentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    presentBarrier.image = previewImage;
    presentBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    presentBarrier.subresourceRange.baseMipLevel = 0;
    presentBarrier.subresourceRange.levelCount = 1;
    presentBarrier.subresourceRange.baseArrayLayer = 0;
    presentBarrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &presentBarrier);

    result = vkEndCommandBuffer(cmd);
    if (result != VK_SUCCESS) {
        Logf("[SimXR] vkEndCommandBuffer failed: %d", (int)result);
        return;
    }

    // Submit blit, signalling the semaphore the present waits on and the fence that says this
    // image's command buffer can be recorded into again.
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &s.vkBlitDone[imageIndex];

    result = vkQueueSubmit(s.vkQueue, 1, &submitInfo, s.vkBlitFences[imageIndex]);
    if (result != VK_SUCCESS) {
        Logf("[SimXR] vkQueueSubmit failed: %d", (int)result);
        return;
    }
    s.vkBlitSubmitted[imageIndex] = true;

    // No vkQueueWaitIdle here: the blit is ordered before the present by the semaphore, so the CPU
    // does not have to stall on the whole queue (which also drains the game's own work) each frame.

    // Present the same image that we just blitted.
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &s.vkBlitDone[imageIndex];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &s.vkPreviewSwapchain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(s.vkQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        Logf("[SimXR] vkQueuePresentKHR: swapchain is out of date.");
    } else if ((result != VK_SUCCESS) && (result != VK_SUBOPTIMAL_KHR)) {
        Logf("[SimXR] vkQueuePresentKHR failed: %d", (int)result);
    }
}
// ---- End Vulkan CPU readback helper ----

static void presentProjection(rt::Session& s, const XrCompositionLayerProjection& proj, bool skipPresent = false) {
    ShowCursor(FALSE);

    if (verboseLogging) Log("[SimXR] ============================================");
    if (verboseLogging) Logf("[SimXR] presentProjection called: viewCount=%u, skipPresent=%d", proj.viewCount, (int)skipPresent);
    if (verboseLogging) Log("[SimXR] RENDERING FRAME TO PREVIEW WINDOW");
    if (verboseLogging) Log("[SimXR] ============================================");
    if (proj.viewCount < 1) {
        Log("[SimXR] presentProjection: No views, returning");
        return;
    }
    const auto& vL = proj.views[0];
    auto itL = rt::g_swapchains.find(vL.subImage.swapchain); 
    if (itL == rt::g_swapchains.end()) {
        Log("[SimXR] presentProjection: Left swapchain not found");
        return;
    }
    auto& chL = itL->second;
    uint32_t width = chL.width, height = chL.height;
    const rt::Swapchain* chRPtr = &chL;
    const auto& vR = proj.views[1];
    auto itR = rt::g_swapchains.find(vR.subImage.swapchain);
    if (itR != rt::g_swapchains.end()) {
        chRPtr = &itR->second;
        if (itR->second.width > width) width = itR->second.width;
        if (itR->second.height > height) height = itR->second.height;
    }
    // Single-pass stereo submits one double-wide image per view, so the eye is the view's
    // imageRect: sizing the preview from the whole image gives twice the screen width
    uint32_t eyeWidth = width, eyeHeight = height;
    if (vL.subImage.imageRect.extent.width > 0 && (uint32_t)vL.subImage.imageRect.extent.width < eyeWidth)
        eyeWidth = vL.subImage.imageRect.extent.width;
    if (vL.subImage.imageRect.extent.height > 0 && (uint32_t)vL.subImage.imageRect.extent.height < eyeHeight)
        eyeHeight = vL.subImage.imageRect.extent.height;
    {
        std::lock_guard<std::mutex> lock(s.previewMutex);

        // Vulkan preview path
        if (s.usesVulkan) {
            static int vkFrameCount = 0;
            vkFrameCount++;

            if (!skipPresent) {
                MSG msg; while (PeekMessageW(&msg, s.hwnd, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
                presentVulkanSwapchain(s, proj);
            } else {
                rt::g_last_proj = proj;
                g_presentPending = true;
            }
            return;
        }

        // OpenGL preview path - read pixels from GL textures and display via D3D11
        if (s.usesOpenGL) {
            static int glFrameCount = 0;
            glFrameCount++;

            if (glFrameCount % 60 == 1) {
                Logf("[SimXR] GL PREVIEW: frame=%d, width=%u, height=%u", glFrameCount, width, height);
            }

            // Make the app's GL context current
            HGLRC savedRC = wglGetCurrentContext();
            HDC savedDC = wglGetCurrentDC();

            if (glFrameCount % 60 == 1) {
                Logf("[SimXR] GL PREVIEW: savedRC=%p, savedDC=%p, s.glRC=%p, s.glDC=%p",
                     savedRC, savedDC, s.glRC, s.glDC);
            }

            if (s.glRC && s.glDC) {
                BOOL result = wglMakeCurrent(s.glDC, s.glRC);
                if (glFrameCount % 60 == 1) {
                    Logf("[SimXR] GL PREVIEW: wglMakeCurrent result=%d", result);
                }
            } else {
                if (glFrameCount % 60 == 1) {
                    Log("[SimXR] GL PREVIEW: WARNING - s.glRC or s.glDC is null!");
                }
            }

            // Only the rows the eye rects cover are read back: each readback stalls on the GPU and copies through
            // the CPU, and a game may keep other things in the image (swingmania has a 1080-row desktop mirror under
            // its eyes). Rows are GL rows (bottom-up); rect.offset.y counts from the top, see blitTexture.
            if (!g_glGetTextureSubImageLoaded) {
                g_glGetTextureSubImage = (PFNGLGETTEXTURESUBIMAGEPROC)wglGetProcAddress("glGetTextureSubImage");
                g_glGetTextureSubImageLoaded = true;
                Logf("[SimXR] GL PREVIEW: glGetTextureSubImage %s", g_glGetTextureSubImage ? "available" : "missing, reading whole images");
            }
            int bandLo = 0, bandHi = (int)height;
            if (g_glGetTextureSubImage && vL.subImage.imageRect.extent.height != 0 && vR.subImage.imageRect.extent.height != 0) {
                int lo = INT_MAX, hi = INT_MIN;
                for (const XrRect2Di* r : { &vL.subImage.imageRect, &vR.subImage.imageRect }) {
                    int top = (int)height - r->offset.y, bottom = top - r->extent.height;
                    lo = std::min(lo, std::min(top, bottom));
                    hi = std::max(hi, std::max(top, bottom));
                }
                lo = std::max(lo, 0);
                hi = std::min(hi, (int)height);
                if (lo < hi) { bandLo = lo; bandHi = hi; }
            }
            uint32_t bandHeight = (uint32_t)(bandHi - bandLo);
            auto readRows = [&](GLuint tex, std::vector<uint8_t>& pixels) {
                pixels.resize((size_t)width * bandHeight * 4);
                if (bandHeight < height) {
                    g_glGetTextureSubImage(tex, 0, 0, bandLo, 0, (GLsizei)width, (GLsizei)bandHeight, 1, GL_RGBA, GL_UNSIGNED_BYTE,
                                           (GLsizei)pixels.size(), pixels.data());
                } else {
                    glBindTexture(GL_TEXTURE_2D, tex);
                    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
                }
            };
            LARGE_INTEGER qpcFreq, qpcStart, qpcRead, qpcUpload;
            QueryPerformanceFrequency(&qpcFreq);
            QueryPerformanceCounter(&qpcStart);

            // Read pixel data from GL textures into CPU buffers
            std::vector<uint8_t> leftPixels((size_t)width * bandHeight * 4);
            std::vector<uint8_t> rightPixels;

            // Get left eye texture
            GLuint leftTex = 0;
            if (chL.imagesGL.size() > 0) {
                uint32_t idx = chL.lastReleased;
                if (idx == UINT32_MAX || idx >= chL.imageCount) idx = chL.lastAcquired;
                if (idx != UINT32_MAX && idx < chL.imagesGL.size()) {
                    leftTex = chL.imagesGL[idx];
                }
            }

            // Get right eye texture
            GLuint rightTex = 0;
            if (chRPtr && chRPtr->imagesGL.size() > 0) {
                uint32_t idx = chRPtr->lastReleased;
                if (idx == UINT32_MAX || idx >= chRPtr->imageCount) idx = chRPtr->lastAcquired;
                if (idx != UINT32_MAX && idx < chRPtr->imagesGL.size()) {
                    rightTex = chRPtr->imagesGL[idx];
                }
            }

            bool sharedTex = rightTex != 0 && rightTex == leftTex;

            // Read left eye pixels
            if (leftTex != 0) {
                readRows(leftTex, leftPixels);
            }

            // Read right eye pixels; when both views share one image (side by side) the left readback serves both,
            // since each readback is a full-image CPU copy
            if (rightTex != 0 && !sharedTex) {
                readRows(rightTex, rightPixels);
            }
            QueryPerformanceCounter(&qpcRead);

            // Restore original GL context
            if (savedRC) {
                wglMakeCurrent(savedDC, savedRC);
            }

            // Now create/use D3D11 preview if we don't have one yet
            if (!s.d3d11Device) {
                // Create D3D11 device for preview window
                D3D_FEATURE_LEVEL featureLevel;
                UINT flags = 0;
                #ifdef _DEBUG
                flags |= D3D11_CREATE_DEVICE_DEBUG;
                #endif
                HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
                                               nullptr, 0, D3D11_SDK_VERSION,
                                               &s.d3d11Device, &featureLevel, &s.d3d11Context);
                if (FAILED(hr)) {
                    Logf("[SimXR] Failed to create D3D11 device for GL preview: 0x%08X", hr);
                    return;
                }
                Log("[SimXR] Created D3D11 device for OpenGL preview");

                // Force shader recompilation by resetting blit resources
                s.blitVS.Reset();
                s.blitPS.Reset();
                s.samplerState.Reset();
                s.noCullRS.Reset();
                for (auto& blend : s.quadBlend) blend.Reset();
                Log("[SimXR] Reset blit resources for fresh shader compilation");
            }

            // Use the standard preview path now that we have a D3D11 device
            DXGI_FORMAT displayFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
            int targetWidth = (int)eyeWidth * 2;
            int targetHeight = (int)eyeHeight;

            if (glFrameCount % 60 == 1 && verboseLogging) {
                Logf("[SimXR] GL PREVIEW: targetSize=%dx%d, calling ensurePreviewSized", targetWidth, targetHeight);
            }

            ensurePreviewSized(s, (UINT)targetWidth, (UINT)targetHeight, displayFormat);

            if (!s.previewSwapchain) {
                Log("[SimXR] GL PREVIEW: ERROR - previewSwapchain is NULL after ensurePreviewSized!");
                return;
            }

            // Get the backbuffer
            ComPtr<ID3D11Texture2D> bb;
            if (FAILED(s.previewSwapchain->GetBuffer(0, IID_PPV_ARGS(bb.GetAddressOf())))) {
                Log("[SimXR] Failed to get preview swapchain buffer for GL preview");
                return;
            }

            // Create staging textures to upload GL pixel data
            D3D11_TEXTURE2D_DESC texDesc = {};
            texDesc.Width = width;
            texDesc.Height = bandHeight;
            texDesc.MipLevels = 1;
            texDesc.ArraySize = 1;
            texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            texDesc.SampleDesc.Count = 1;
            texDesc.Usage = D3D11_USAGE_DEFAULT;
            texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

            D3D11_SUBRESOURCE_DATA initData = {};
            initData.pSysMem = leftPixels.data();
            initData.SysMemPitch = width * 4;

            ComPtr<ID3D11Texture2D> leftTex2D;
            s.d3d11Device->CreateTexture2D(&texDesc, &initData, &leftTex2D);

            ComPtr<ID3D11Texture2D> rightTex2D;
            if (sharedTex) {
                rightTex2D = leftTex2D;
            } else {
                rightPixels.resize((size_t)width * bandHeight * 4);
                initData.pSysMem = rightPixels.data();
                s.d3d11Device->CreateTexture2D(&texDesc, &initData, &rightTex2D);
            }
            QueryPerformanceCounter(&qpcUpload);
            if (glFrameCount % 60 == 1) {
                Logf("[SimXR] GL PREVIEW: read rows %d-%d of %u (%s) %.1f ms, upload %.1f ms", bandLo, bandHi, height,
                     sharedTex ? "one image" : "two images",
                     (qpcRead.QuadPart - qpcStart.QuadPart) * 1000.0 / qpcFreq.QuadPart,
                     (qpcUpload.QuadPart - qpcRead.QuadPart) * 1000.0 / qpcFreq.QuadPart);
            }

            // Initialize blit resources if not already done
            if (!rt::InitBlitResources(s)) {
                Log("[SimXR] OpenGL preview: Failed to init blit resources");
                return;
            }

            // Create render target view for the backbuffer
            ComPtr<ID3D11RenderTargetView> rtv;
            if (FAILED(s.d3d11Device->CreateRenderTargetView(bb.Get(), nullptr, rtv.GetAddressOf()))) {
                Log("[SimXR] OpenGL preview: Failed to create RTV");
                return;
            }

            // Create SRVs for the uploaded textures
            ComPtr<ID3D11ShaderResourceView> leftSRV, rightSRV;
            if (leftTex2D) {
                HRESULT hr = s.d3d11Device->CreateShaderResourceView(leftTex2D.Get(), nullptr, leftSRV.GetAddressOf());
                if (FAILED(hr) && glFrameCount % 60 == 1) {
                    Logf("[SimXR] GL PREVIEW: CreateSRV for left failed: 0x%08X", hr);
                }
            }
            if (rightTex2D) {
                HRESULT hr = s.d3d11Device->CreateShaderResourceView(rightTex2D.Get(), nullptr, rightSRV.GetAddressOf());
                if (FAILED(hr) && glFrameCount % 60 == 1) {
                    Logf("[SimXR] GL PREVIEW: CreateSRV for right failed: 0x%08X", hr);
                }
            }

            if (glFrameCount % 60 == 1) {
                Logf("[SimXR] GL PREVIEW: leftTex2D=%p rightTex2D=%p leftSRV=%p rightSRV=%p",
                     leftTex2D.Get(), rightTex2D.Get(), leftSRV.Get(), rightSRV.Get());
            }

            // Clear the render target
            ID3D11RenderTargetView* rtvs[1] = { rtv.Get() };
            s.d3d11Context->OMSetRenderTargets(1, rtvs, nullptr);
            const float clearColor[4] = {0.1f, 0.1f, 0.2f, 1.0f};  // Dark blue
            s.d3d11Context->ClearRenderTargetView(rtv.Get(), clearColor);

            // Setup viewports for left and right eyes
            D3D11_VIEWPORT fullVp = {};
            fullVp.TopLeftX = 0.0f;
            fullVp.TopLeftY = 0.0f;
            fullVp.Width = (float)s.previewWidth;
            fullVp.Height = (float)s.previewHeight;
            fullVp.MinDepth = 0.0f;
            fullVp.MaxDepth = 1.0f;

            D3D11_VIEWPORT leftVp = fullVp;
            D3D11_VIEWPORT rightVp = fullVp;
            float half = monoRendering ? 1.0f : 2.0f;
            leftVp.Width = (float)s.previewWidth / half;
            rightVp.Width = (float)s.previewWidth / half;
            rightVp.TopLeftX = (float)s.previewWidth / half;

            // Helper lambda to blit a texture to a viewport
            auto blitTexture = [&](ID3D11ShaderResourceView* srv, const D3D11_VIEWPORT& vp, const XrRect2Di& rect) {
                if (!srv) {
                    if (glFrameCount % 60 == 1) Log("[SimXR] GL PREVIEW: blitTexture - SRV is null!");
                    return;
                }

                if (glFrameCount % 60 == 1) {
                    Logf("[SimXR] GL PREVIEW: blitTexture - vp=(%.0f,%.0f,%.0f,%.0f) srv=%p",
                         vp.TopLeftX, vp.TopLeftY, vp.Width, vp.Height, srv);
                }

                // Ensure render target is bound
                ID3D11RenderTargetView* currentRTVs[1] = { rtv.Get() };
                s.d3d11Context->OMSetRenderTargets(1, currentRTVs, nullptr);

                s.d3d11Context->RSSetViewports(1, &vp);
                s.d3d11Context->VSSetShader(s.blitVS.Get(), nullptr, 0);
                s.d3d11Context->PSSetShader(s.blitPS.Get(), nullptr, 0);

                // Update blit constants
                rt::BlitConstants blitConstants = {};
                // The rect is a window onto the source texture, so it scales by that texture's size and not
                // by the viewport it is drawn into -- a game submitting one double-wide image per view has
                // an eye rect half the texture's width, which the viewport's own width would read as all of it.
                // The readback rows are bottom-up GL rows, but games measure rect.offset.y from the top of the image
                // as SteamVR does (swingmania puts its eyes in the top 960 rows and a desktop mirror below, at y=0).
                // A full-height rect maps the same either way. Only rows bandLo..bandHi were uploaded.
                blitConstants.uvMinX = static_cast<float>(rect.offset.x) / static_cast<float>(width);
                blitConstants.uvMinY = static_cast<float>((int)height - rect.offset.y - bandLo) / static_cast<float>(bandHeight);
                blitConstants.uvMaxX = static_cast<float>(rect.offset.x + rect.extent.width) / static_cast<float>(width);
                blitConstants.uvMaxY = static_cast<float>((int)height - rect.offset.y - rect.extent.height - bandLo) / static_cast<float>(bandHeight);
                while (std::min(blitConstants.uvMinY, blitConstants.uvMaxY) < 0) { blitConstants.uvMaxY += 1.0f; blitConstants.uvMinY += 1.0f; }
                while (std::max(blitConstants.uvMinY, blitConstants.uvMaxY) > 1) { blitConstants.uvMaxY -= 1.0f; blitConstants.uvMinY -= 1.0f; }

                s.d3d11Context->UpdateSubresource(s.blitConstantBuffer.Get(), 0, nullptr, &blitConstants, 0, 0);
                ID3D11Buffer* constantBuffers[] = { s.blitConstantBuffer.Get() };
                s.d3d11Context->PSSetConstantBuffers(0, 1, constantBuffers);

                ID3D11ShaderResourceView* srvs[] = { srv };
                s.d3d11Context->PSSetShaderResources(0, 1, srvs);
                ID3D11SamplerState* samplers[] = { s.samplerState.Get() };
                s.d3d11Context->PSSetSamplers(0, 1, samplers);

                s.d3d11Context->IASetInputLayout(nullptr);
                s.d3d11Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
                s.d3d11Context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
                s.d3d11Context->OMSetDepthStencilState(nullptr, 0);
                s.d3d11Context->RSSetState(s.noCullRS.Get());

                s.d3d11Context->Draw(4, 0);

                // Unbind SRV
                ID3D11ShaderResourceView* nullSRV[] = { nullptr };
                s.d3d11Context->PSSetShaderResources(0, 1, nullSRV);
            };

            //----------------
            //OXRWXR CHANGE:
            //----------------
            // Helper lambda to blit a solid red quad (OPENGL)
            auto blitRedQuad = [&](const D3D11_VIEWPORT& vp, int redIntensity = 1) {
                // Define a simple red pixel shader
                struct Vertex {
                    float x, y, z;
                };
                Vertex vertices[] = {
                    { 0.0f, 0.0f, 0.0f },
                    { 10.0f, 0.0f, 0.0f },
                    { 0.0f, 10.0f, 0.0f },
                    { 10.0f, 10.0f, 0.0f }
                };

                // Create a vertex buffer
                ID3D11Buffer* vertexBuffer = nullptr;
                D3D11_BUFFER_DESC bd = {};
                bd.ByteWidth = sizeof(vertices);
                bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
                bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                bd.Usage = D3D11_USAGE_DYNAMIC;
                D3D11_SUBRESOURCE_DATA initData = { vertices, 0, 0 };
                s.d3d11Device->CreateBuffer(&bd, &initData, &vertexBuffer);

                // Map vertex buffer
                D3D11_MAPPED_SUBRESOURCE mappedResource;
                s.d3d11Context->Map(vertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
                memcpy(mappedResource.pData, vertices, sizeof(vertices));
                s.d3d11Context->Unmap(vertexBuffer, 0);

                // Ensure render target is bound
                ID3D11RenderTargetView* currentRTVs[1] = { rtv.Get() };
                s.d3d11Context->OMSetRenderTargets(1, currentRTVs, nullptr);

                s.d3d11Context->RSSetViewports(1, &vp);
                s.d3d11Context->VSSetShader(s.blitVS.Get(), nullptr, 0);
                s.d3d11Context->PSSetShader(s.solidColorPS.Get(), nullptr, 0);

                if (!s.colorConstantBuffer) {
                    D3D11_BUFFER_DESC cbDesc = {};
                    cbDesc.ByteWidth = sizeof(float) * 4;
                    cbDesc.Usage = D3D11_USAGE_DEFAULT;
                    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
                    HRESULT hr = s.d3d11Device->CreateBuffer(&cbDesc, nullptr, s.colorConstantBuffer.GetAddressOf());
                    if (FAILED(hr)) {
                        Logf("[WinXrApi] Failed to create color constant buffer: 0x%08X", hr);
                        // Release resources
                        vertexBuffer->Release();
                        return;
                    }
                }

                // Update color constant buffer
                struct ColorConstantBuffer {
                    float color[4];
                } colorBuffer;
                colorBuffer.color[0] = redIntensity / 255.0f;
                colorBuffer.color[1] = 0;
                colorBuffer.color[2] = 0;
                colorBuffer.color[3] = 255;
                s.d3d11Context->UpdateSubresource(s.colorConstantBuffer.Get(), 0, nullptr, &colorBuffer, 0, 0);
                s.d3d11Context->PSSetConstantBuffers(0, 1, s.colorConstantBuffer.GetAddressOf());

                // Set vertex buffer
                UINT stride = sizeof(Vertex);
                UINT offset = 0;
                s.d3d11Context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
                s.d3d11Context->IASetInputLayout(s.simpleVertexLayout.Get());
                s.d3d11Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
                s.d3d11Context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
                s.d3d11Context->OMSetDepthStencilState(nullptr, 0);
                s.d3d11Context->RSSetState(s.noCullRS.Get());

                // Draw the quad
                s.d3d11Context->Draw(4, 0);

                // Clean up
                vertexBuffer->Release();
            };

            // Render the eyes
            if (leftSRV) {
                blitTexture(leftSRV.Get(), leftVp, vL.subImage.imageRect);
            }
            if (rightSRV && !monoRendering) {
                blitTexture(rightSRV.Get(), rightVp, vR.subImage.imageRect);
            }

            //----------------
            //OXRWXR CHANGE:
            //----------------
            // Render the OpenXR Frame Sync pixels (OPENGL)
            // Once per frame, and the log is flushed to D: on every line, so it costs a
            // write to shared storage per frame unless it is asked for
            if (verboseLogging) Logf("[WinXrApi] Drawing Red Sync Pixel (OPENGL)");
            D3D11_VIEWPORT vpOXR = {};
            vpOXR.TopLeftX = 0;
            vpOXR.TopLeftY = 0;
            vpOXR.Width = 10;
            vpOXR.Height = 10;
            vpOXR.MinDepth = 0;
            vpOXR.MaxDepth = 1;

            blitRedQuad(vpOXR, OpenXRFrameID);

            // Present (may be deferred if overlays are pending)
            if (!skipPresent) {
                MSG msg;
                while (PeekMessageW(&msg, s.hwnd, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessageW(&msg); }

                if (glFrameCount % 60 == 1) {
                    Logf("[SimXR] GL PREVIEW: About to Present - hwnd=%p, swapchain=%p", s.hwnd, s.previewSwapchain.Get());
                }

                HRESULT presentHr = s.previewSwapchain->Present(1, 0);
                if (FAILED(presentHr) && glFrameCount % 60 == 1) {
                    Logf("[SimXR] GL PREVIEW: Present FAILED with hr=0x%08X", presentHr);
                }
            } else {
                g_presentPending = true;
            }

            return;
        }

        // Use UNORM format for swapchain (SRGB not valid for FLIP_DISCARD)
        // We create SRGB RTVs for proper gamma when rendering
        DXGI_FORMAT displayFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        int targetWidth = monoRendering ? (int)eyeWidth : (int)eyeWidth * 2;
        int targetHeight = (int)eyeHeight;
        ensurePreviewSized(s, (UINT)targetWidth, (UINT)targetHeight, displayFormat);

        // Get left image index
        uint32_t leftIdx = 0;
        if (chL.lastReleased != UINT32_MAX && chL.lastReleased < chL.imageCount) {
            leftIdx = chL.lastReleased;
        } else if (chL.lastAcquired != UINT32_MAX && chL.lastAcquired < chL.imageCount) {
            leftIdx = chL.lastAcquired;
        }

        static int blitCount = 0;
        if (++blitCount % 60 == 1 && verboseLogging) {  // Log every 60 frames
            Logf("[SimXR] Blitting left eye: idx=%u (lastReleased=%u, lastAcquired=%u, imageCount=%u)",
                 leftIdx, chL.lastReleased, chL.lastAcquired, chL.imageCount);
        }

        if (!s.usesD3D12) {
            // ===== D3D11 PATH =====
            if (!s.previewSwapchain) return;

            // Save D3D11 context state - will auto-restore when stateBackup goes out of scope
            D3D11StateBackup stateBackup(s.d3d11Context.Get());

            // Get the backbuffer and create RTV
            ComPtr<ID3D11Texture2D> bb;
            if (FAILED(s.previewSwapchain->GetBuffer(0, IID_PPV_ARGS(bb.GetAddressOf())))) {
                Log("[SimXR] Failed to get preview swapchain buffer.");
                return;
            }

            // Create explicit sRGB RTV for proper gamma encoding
            DXGI_FORMAT bbFmt = (DXGI_FORMAT)s.previewFormat;
            DXGI_FORMAT rtvFmt = bbFmt;
            if (bbFmt == DXGI_FORMAT_R8G8B8A8_UNORM)       rtvFmt = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            else if (bbFmt == DXGI_FORMAT_B8G8R8A8_UNORM)  rtvFmt = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

            D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
            rtvDesc.Format = rtvFmt;
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            rtvDesc.Texture2D.MipSlice = 0;

            ComPtr<ID3D11RenderTargetView> rtv;
            HRESULT hr = s.d3d11Device->CreateRenderTargetView(bb.Get(), &rtvDesc, rtv.GetAddressOf());
            if (FAILED(hr)) {
                Logf("[SimXR] Explicit sRGB RTV failed (0x%08X), falling back to auto format", hr);
                if (FAILED(s.d3d11Device->CreateRenderTargetView(bb.Get(), nullptr, rtv.GetAddressOf()))) {
                    Log("[SimXR] Failed to create RTV for preview.");
                    return;
                }
            }

            // Bind RTV and clear
            ID3D11RenderTargetView* rtvs[1] = { rtv.Get() };
            s.d3d11Context->OMSetRenderTargets(1, rtvs, nullptr);
            const float clearColor[4] = {0.1f, 0.1f, 0.2f, 1.0f};
            s.d3d11Context->ClearRenderTargetView(rtv.Get(), clearColor);

            D3D11_VIEWPORT fullVp = {};
            fullVp.TopLeftX = 0.0f;
            fullVp.TopLeftY = 0.0f;
            fullVp.Width = (float)s.previewWidth;
            fullVp.Height = (float)s.previewHeight;
            fullVp.MinDepth = 0.0f;
            fullVp.MaxDepth = 1.0f;

            D3D11_VIEWPORT leftVp = fullVp;
            D3D11_VIEWPORT rightVp = fullVp;
            float half = monoRendering ? 1.0f : 2.0f;
            leftVp.Width = (float)s.previewWidth / half;
            rightVp.Width = (float)s.previewWidth / half;
            rightVp.TopLeftX = (float)s.previewWidth / half;

            blitViewToHalf(s, chL, leftIdx, vL.subImage.imageArrayIndex, vL.subImage.imageRect,
                           rtv.Get(), leftVp, nullptr, true);

            // Blit right eye
            if (proj.viewCount > 1) {
                const auto& vR = proj.views[1];
                auto& chR = const_cast<rt::Swapchain&>(*chRPtr);
                uint32_t rightIdx = 0;
                if (chR.lastReleased != UINT32_MAX && chR.lastReleased < chR.imageCount) {
                    rightIdx = chR.lastReleased;
                } else if (chR.lastAcquired != UINT32_MAX && chR.lastAcquired < chR.imageCount) {
                    rightIdx = chR.lastAcquired;
                }
                blitViewToHalf(s, chR, rightIdx, vR.subImage.imageArrayIndex, vR.subImage.imageRect,
                               rtv.Get(), rightVp, nullptr, false);
            }

            // Present D3D11 (may be deferred if overlays are pending)
            if (!skipPresent) {
                MSG msg;
                while (PeekMessageW(&msg, s.hwnd, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
                s.previewSwapchain->Present(1, 0);
            } else {
                g_presentPending = true;
            }
        } else {
            // ===== D3D12 PATH =====
            if (!s.previewSwapchain12) return;

            // Blit using D3D12 copy commands
            if (proj.viewCount > 1) {
                const auto& vR = proj.views[1];
                auto& chR = const_cast<rt::Swapchain&>(*chRPtr);
                uint32_t rightIdx = 0;
                if (chR.lastReleased != UINT32_MAX && chR.lastReleased < chR.imageCount) {
                    rightIdx = chR.lastReleased;
                } else if (chR.lastAcquired != UINT32_MAX && chR.lastAcquired < chR.imageCount) {
                    rightIdx = chR.lastAcquired;
                }
                blitD3D12ToPreview(s, chL, leftIdx, vL.subImage.imageArrayIndex,
                                   &chR, rightIdx, vR.subImage.imageArrayIndex);
            } else {
                blitD3D12ToPreview(s, chL, leftIdx, vL.subImage.imageArrayIndex,
                                   nullptr, 0, 0);
            }

            // Present D3D12 (may be deferred if overlays are pending)
            if (!skipPresent) {
                MSG msg;
                while (PeekMessageW(&msg, s.hwnd, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
                s.previewSwapchain12->Present(1, 0);
            } else {
                g_presentPending = true;
            }
        }
    }
}

// Render a quad layer as 2D overlay (supports both D3D11 and OpenGL)
static void renderQuadLayer(rt::Session& s, const XrCompositionLayerQuad* quad) {
    if (!quad || !s.previewSwapchain || disable2DLayer) return;

    if (s.usesD3D12) {
        static bool warnedD3D12 = false;
        if (!warnedD3D12) {
            Log("[SimXR] WARNING: Quad layer rendering not implemented for D3D12 sessions");
            warnedD3D12 = true;
        }
        return;
    }

    auto it = rt::g_swapchains.find(quad->subImage.swapchain);
    if (it == rt::g_swapchains.end()) return;

    auto& chain = it->second;

    // Get texture dimensions from the quad subImage
    uint32_t texWidth = quad->subImage.imageRect.extent.width;
    uint32_t texHeight = quad->subImage.imageRect.extent.height;
    if (texWidth == 0) texWidth = chain.width;
    if (texHeight == 0) texHeight = chain.height;

    // Get texture index
    uint32_t texIdx = (chain.lastReleased != UINT32_MAX) ? chain.lastReleased :
                      (chain.lastAcquired != UINT32_MAX) ? chain.lastAcquired : 0;

    static int quadLogCount = 0;
    bool shouldLog = (++quadLogCount % 60 == 1);

    if (shouldLog && verboseLogging) {
        Logf("[SimXR] Quad swapchain: handle=%llu, lastReleased=%u, lastAcquired=%u, texIdx=%u, imageCount=%u",
             (unsigned long long)quad->subImage.swapchain, chain.lastReleased, chain.lastAcquired,
             texIdx, chain.imageCount);
    }

    ComPtr<ID3D11Texture2D> quadTex;

    // Check if using OpenGL
    if (chain.backend == rt::Swapchain::Backend::OpenGL && !chain.imagesGL.empty()) {
        if (texIdx >= chain.imagesGL.size()) return;
        GLuint glTex = chain.imagesGL[texIdx];
        if (glTex == 0) return;

        // Save and switch to app's GL context
        HGLRC savedRC = wglGetCurrentContext();
        HDC savedDC = wglGetCurrentDC();

        // DEBUG: Log current context BEFORE switch
        if (shouldLog && verboseLogging) {
            Logf("[SimXR] Quad GL context: current={RC=%p,DC=%p}, stored={RC=%p,DC=%p}",
                 savedRC, savedDC, s.glRC, s.glDC);
        }

        if (s.glRC && s.glDC) {
            BOOL switchResult = wglMakeCurrent(s.glDC, s.glRC);
            if (shouldLog && verboseLogging) {
                HGLRC afterRC = wglGetCurrentContext();
                Logf("[SimXR] Quad GL context switch: result=%d, afterRC=%p (expected %p)",
                     switchResult, afterRC, s.glRC);
            }
        }

        // Ensure all GL commands are finished before reading
        glFinish();

        // DEBUG: Verify texture exists and is valid
        if (shouldLog && verboseLogging) {
            GLboolean isValid = glIsTexture(glTex);
            Logf("[SimXR] Quad texture check: glTex=%u, glIsTexture=%d", glTex, isValid);
        }

        // Read pixels from GL texture using FBO (more reliable than glGetTexImage)
        std::vector<uint8_t> pixels(texWidth * texHeight * 4);

        // Try FBO method first if available
        bool usedFBO = false;
        if (EnsureGLFramebufferFuncs()) {
            // Create a temporary FBO to read the texture
            GLuint readFBO = 0;
            g_glGenFramebuffers(1, &readFBO);
            g_glBindFramebuffer(GL_FRAMEBUFFER, readFBO);
            g_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, glTex, 0);

            GLenum fboStatus = g_glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (fboStatus == GL_FRAMEBUFFER_COMPLETE) {
                glReadPixels(0, 0, texWidth, texHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
                usedFBO = true;
            } else {
                if (shouldLog) {
                    Logf("[SimXR] Quad FBO not complete: status=0x%X, falling back to glGetTexImage", fboStatus);
                }
            }

            // Cleanup FBO
            g_glBindFramebuffer(GL_FRAMEBUFFER, 0);
            g_glDeleteFramebuffers(1, &readFBO);
        }

        // Fallback to glGetTexImage
        if (!usedFBO) {
            glBindTexture(GL_TEXTURE_2D, glTex);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        }

        // Flip vertically (OpenGL has Y=0 at bottom)
        const uint32_t rowSize = texWidth * 4;
        std::vector<uint8_t> tempRow(rowSize);
        for (uint32_t y = 0; y < texHeight / 2; y++) {
            uint8_t* topRow = pixels.data() + y * rowSize;
            uint8_t* bottomRow = pixels.data() + (texHeight - 1 - y) * rowSize;
            memcpy(tempRow.data(), topRow, rowSize);
            memcpy(topRow, bottomRow, rowSize);
            memcpy(bottomRow, tempRow.data(), rowSize);
        }

        // Restore GL context
        if (savedRC) wglMakeCurrent(savedDC, savedRC);

        // Create D3D11 texture from pixel data
        D3D11_TEXTURE2D_DESC texDesc = {};
        texDesc.Width = texWidth;
        texDesc.Height = texHeight;
        texDesc.MipLevels = 1;
        texDesc.ArraySize = 1;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Usage = D3D11_USAGE_DEFAULT;
        texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = pixels.data();
        initData.SysMemPitch = texWidth * 4;

        if (FAILED(s.d3d11Device->CreateTexture2D(&texDesc, &initData, quadTex.GetAddressOf()))) {
            if (shouldLog) Log("[SimXR] renderQuadLayer: Failed to create D3D11 texture from GL pixels");
            return;
        }

        if (shouldLog && verboseLogging) {
            Logf("[SimXR] Rendering quad layer (OpenGL): size=%.2fx%.2f, texSize=%ux%u, glTex=%u",
                 quad->size.width, quad->size.height, texWidth, texHeight, glTex);
        }
    } else if (!chain.images.empty()) {
        // D3D11 path
        if (texIdx >= chain.images.size() || !chain.images[texIdx]) return;

        // Skip depth formats
        D3D11_TEXTURE2D_DESC srcDesc;
        chain.images[texIdx]->GetDesc(&srcDesc);
        if (srcDesc.Format == DXGI_FORMAT_D32_FLOAT || srcDesc.Format == DXGI_FORMAT_D24_UNORM_S8_UINT ||
            srcDesc.Format == DXGI_FORMAT_D16_UNORM || srcDesc.Format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT) {
            return;
        }

        // Convert typeless formats to typed formats for SRV creation
        DXGI_FORMAT typedFormat = srcDesc.Format;
        switch (srcDesc.Format) {
            case DXGI_FORMAT_R8G8B8A8_TYPELESS:
                typedFormat = (chain.format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB) ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
                break;
            case DXGI_FORMAT_B8G8R8A8_TYPELESS:
                typedFormat = (chain.format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB) ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB : DXGI_FORMAT_B8G8R8A8_UNORM;
                break;
            case DXGI_FORMAT_R16G16B16A16_TYPELESS:
                typedFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
                break;
            case DXGI_FORMAT_R32G32B32A32_TYPELESS:
                typedFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
                break;
            case DXGI_FORMAT_R10G10B10A2_TYPELESS:
                typedFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
                break;
            default:
                break; // Already typed or unknown
        }

        // Create temp texture for the quad content with typed format
        D3D11_TEXTURE2D_DESC tempDesc = srcDesc;
        tempDesc.Format = typedFormat;  // Use typed format for the temp texture
        tempDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        tempDesc.MiscFlags = 0;
        tempDesc.SampleDesc.Count = 1;

        if (FAILED(s.d3d11Device->CreateTexture2D(&tempDesc, nullptr, quadTex.GetAddressOf()))) return;

        // Copy the quad texture (handle array index if needed)
        const auto& rect = quad->subImage.imageRect;
        uint32_t arraySlice = quad->subImage.imageArrayIndex;
        D3D11_BOX box = { (UINT)rect.offset.x, (UINT)rect.offset.y, 0,
                          (UINT)(rect.offset.x + abs(rect.extent.width)), (UINT)(rect.offset.y + abs(rect.extent.height)), 1 };
        uint32_t srcSubresource = D3D11CalcSubresource(0, arraySlice, 1);
        s.d3d11Context->CopySubresourceRegion(quadTex.Get(), 0, 0, 0, 0, chain.images[texIdx].Get(), srcSubresource, &box);

        if (shouldLog && verboseLogging) {
            Logf("[SimXR] Rendering quad layer (D3D11): size=%.2fx%.2f, texSize=%ux%u, typedFmt=%d, srcFmt=%d, arraySlice=%u",
                 quad->size.width, quad->size.height, srcDesc.Width, srcDesc.Height, typedFormat, srcDesc.Format, arraySlice);
        }
    } else {
        if (shouldLog) Log("[SimXR] renderQuadLayer: No valid images in swapchain");
        return;
    }

    // Get backbuffer
    ComPtr<ID3D11Texture2D> bb;
    if (FAILED(s.previewSwapchain->GetBuffer(0, IID_PPV_ARGS(bb.GetAddressOf())))) return;

    // Create RTV
    ComPtr<ID3D11RenderTargetView> rtv;
    if (FAILED(s.d3d11Device->CreateRenderTargetView(bb.Get(), nullptr, rtv.GetAddressOf()))) return;

    // Create SRV
    ComPtr<ID3D11ShaderResourceView> srv;
    if (FAILED(s.d3d11Device->CreateShaderResourceView(quadTex.Get(), nullptr, srv.GetAddressOf()))) return;

    // Calculate viewport for quad
    float quadAspect = quad->size.width / quad->size.height;
    float fullEyeW = monoRendering ? (float)ViewportWidth : (float)ViewportWidth / 2.0f;
    float fullEyeH = (float)ViewportHeight;
    float eyeW = fullEyeW * 0.5f;
    float eyeH = eyeW / quadAspect;
    float topY = (fullEyeH - eyeH) * 0.5f;
    float leftX = (fullEyeW - eyeW) * 0.5f;
    D3D11_VIEWPORT leftVp = {leftX, topY, eyeW, eyeH, 0.0f, 1.0f};
    D3D11_VIEWPORT rightVp = {fullEyeW + leftX, topY, eyeW, eyeH, 0.0f, 1.0f};

    // Render the quad
    s.d3d11Context->RSSetViewports(1, &leftVp);
    s.d3d11Context->VSSetShader(s.blitVS.Get(), nullptr, 0);
    s.d3d11Context->PSSetShader(s.blitPS.Get(), nullptr, 0);

    // The blit shader samples through the UV constants, which otherwise still hold the last eye
    // blit's rect. The quad's image sits at the top left of quadTex (all of it on the GL path),
    // and a negative extent is a flip
    if (s.blitConstantBuffer) {
        D3D11_TEXTURE2D_DESC quadDesc;
        quadTex->GetDesc(&quadDesc);
        const auto& extent = quad->subImage.imageRect.extent;
        rt::BlitConstants blitConstants = {};
        blitConstants.uvMaxX = extent.width ? std::min(1.0f, (float)abs(extent.width) / (float)quadDesc.Width) : 1.0f;
        blitConstants.uvMaxY = extent.height ? std::min(1.0f, (float)abs(extent.height) / (float)quadDesc.Height) : 1.0f;
        if (extent.width < 0) std::swap(blitConstants.uvMinX, blitConstants.uvMaxX);
        if (extent.height < 0) std::swap(blitConstants.uvMinY, blitConstants.uvMaxY);
        s.d3d11Context->UpdateSubresource(s.blitConstantBuffer.Get(), 0, nullptr, &blitConstants, 0, 0);
        ID3D11Buffer* constantBuffers[] = { s.blitConstantBuffer.Get() };
        s.d3d11Context->PSSetConstantBuffers(0, 1, constantBuffers);
    }

    ID3D11ShaderResourceView* srvs[] = { srv.Get() };
    s.d3d11Context->PSSetShaderResources(0, 1, srvs);
    ID3D11SamplerState* samplers[] = { s.samplerState.Get() };
    s.d3d11Context->PSSetSamplers(0, 1, samplers);

    s.d3d11Context->IASetInputLayout(nullptr);
    s.d3d11Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // Use alpha blending for overlay. Writing the quad unblended punched its transparent parts, alpha
    // and all, into the eyes - a hidden pause menu left a black rectangle (FlatOut 4 VR)
    int blendMode = !(quad->layerFlags & XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT) ? 0
                  : (quad->layerFlags & XR_COMPOSITION_LAYER_UNPREMULTIPLIED_ALPHA_BIT) ? 2 : 1;
    s.d3d11Context->OMSetBlendState(s.quadBlend[blendMode].Get(), nullptr, 0xFFFFFFFF);
    s.d3d11Context->OMSetDepthStencilState(nullptr, 0);
    s.d3d11Context->RSSetState(s.noCullRS.Get());

    ID3D11RenderTargetView* rtvs[1] = { rtv.Get() };
    s.d3d11Context->OMSetRenderTargets(1, rtvs, nullptr);

    s.d3d11Context->Draw(4, 0);

    if (!monoRendering) {
        s.d3d11Context->RSSetViewports(1, &rightVp);
        s.d3d11Context->Draw(4, 0);
    }

    // Cleanup
    ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
    s.d3d11Context->PSSetShaderResources(0, 1, nullSRV);
}

// Direct transport, per frame: no semaphore can leave DXVK's device, so the CPU waits until the
// game's rendering is done before the bridge copies each eye's image and sends it to the host with
// the pose and fov it was rendered with. The wait is on a sender thread, so the game's render thread
// does not stall; without a fence it is for the previous frame on this thread, and the frame reaches
// the host one frame later. Returns true when the frame before this one was sent;
// overlaysSent tells whether every quad and cylinder layer of this frame went with it.
static bool BridgeSlot(const XrSwapchainSubImage& subImage, uint32_t& id) {
    auto it = rt::g_swapchains.find(subImage.swapchain);
    if (it == rt::g_swapchains.end() || it->second.bridgeIds.empty()) return false;
    uint32_t index = it->second.lastReleased;
    if (index >= it->second.bridgeIds.size() || it->second.bridgeIds[index] == UINT32_MAX) return false;
    id = it->second.bridgeIds[index];
    return true;
}

static bool BridgeQuad(wxr_bridge_present_args& present, XrSpace space, const XrSwapchainSubImage& subImage,
                       const XrPosef& pose, float width, float height, XrEyeVisibility eyes, XrCompositionLayerFlags flags) {
    if (present.quad_count >= WXR_BRIDGE_MAX_QUADS) return false;
    wxr_bridge_present_quad& out = present.quads[present.quad_count];
    if (!BridgeSlot(subImage, out.id)) return false;
    out.space = (uint64_t)(uintptr_t)space;
    out.layer = subImage.imageArrayIndex;
    out.rect[0] = subImage.imageRect.offset.x;
    out.rect[1] = subImage.imageRect.offset.y;
    out.rect[2] = subImage.imageRect.extent.width;
    out.rect[3] = subImage.imageRect.extent.height;
    memcpy(out.orientation, &pose.orientation, sizeof(out.orientation));
    memcpy(out.position, &pose.position, sizeof(out.position));
    out.size[0] = width;
    out.size[1] = height;
    out.eye_visibility = (uint32_t)eyes;
    out.flags = (uint32_t)flags;
    present.quad_count++;
    return true;
}

// Sends each direct frame from its own thread as soon as its fence signals, so the host gets it
// when the rendering is done rather than in the next xrEndFrame
static struct BridgeSender {
    std::mutex mtx;
    std::condition_variable cv;
    wxr_bridge_present_args frame{};
    ComPtr<ID3D11Fence> fence;
    UINT64 value = 0;
    bool started = false, busy = false, quit = false, sent = false;
} g_bridgeSender;

static DWORD WINAPI BridgeSenderThread(LPVOID module) {
    BridgeSender& b = g_bridgeSender;
    HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    std::unique_lock<std::mutex> lock(b.mtx);
    for (;;) {
        b.cv.wait(lock, [&b] { return b.busy || b.quit; });
        if (!b.busy) break;
        wxr_bridge_present_args frame = b.frame;
        ComPtr<ID3D11Fence> fence = std::move(b.fence);
        UINT64 value = b.value;
        lock.unlock();

        LARGE_INTEGER freq, start, end;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&start);
        if (event && fence->GetCompletedValue() < value && SUCCEEDED(fence->SetEventOnCompletion(value, event)))
            WaitForSingleObject(event, 100);  // give up after 100 ms, as the query loop does
        QueryPerformanceCounter(&end);
        fence.Reset();
        {
            std::lock_guard<std::mutex> bridgeLock(g_bridgeMutex);
            g_bridgeCall(WXR_BRIDGE_PRESENT, &frame);
        }
        if (frame.frame <= 3 || frame.frame % 300 == 0) {
            Logf("[SimXR] direct_transport: frame %d, %u view(s), %u quad(s), result %d, %s, GPU wait %.2f ms",
                 (int)frame.frame, frame.view_count, frame.quad_count, frame.result,
                 frame.sent ? "sent to host" : "host not connected",
                 (double)(end.QuadPart - start.QuadPart) * 1000.0 / (double)freq.QuadPart);
        }

        lock.lock();
        b.sent = frame.sent != 0;
        b.busy = false;
        b.cv.notify_all();
    }
    b.started = b.quit = false;
    lock.unlock();
    if (event) CloseHandle(event);
    // The thread holds its own reference on this DLL, so the code cannot be unloaded under it
    if (module) FreeLibraryAndExitThread((HMODULE)module, 0);
    return 0;
}

// Queues a frame for the sender. Waits for the frame before it to have gone first, which keeps the
// game from running more than a frame ahead of the GPU. Returns whether that earlier frame was sent.
static bool BridgeSenderSubmit(const wxr_bridge_present_args& frame, ID3D11Fence* fence, UINT64 value) {
    BridgeSender& b = g_bridgeSender;
    std::unique_lock<std::mutex> lock(b.mtx);
    if (!b.started) {
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR)&g_bridgeSender, &module);
        HANDLE thread = CreateThread(nullptr, 0, BridgeSenderThread, module, 0, nullptr);
        if (!thread) {
            if (module) FreeLibrary(module);
            return false;
        }
        CloseHandle(thread);
        b.started = true;
        b.sent = false;
    }
    b.cv.wait(lock, [&b] { return !b.busy; });
    b.frame = frame;
    b.fence = fence;
    b.value = value;
    b.busy = true;
    b.cv.notify_all();
    return b.sent;
}

static void StopBridgeSender() {
    BridgeSender& b = g_bridgeSender;
    std::lock_guard<std::mutex> lock(b.mtx);
    if (!b.started) return;
    b.quit = true;
    b.cv.notify_all();
}

static bool SubmitProjectionToBridge(const XrFrameEndInfo* info, int frameCount, bool& overlaysSent) {
    auto& s = rt::g_session;
    overlaysSent = false;
    if (!g_bridgeCall || !s.d3d11Context) return false;

    wxr_bridge_present_args present{};
    overlaysSent = true;
    for (uint32_t i = 0; i < info->layerCount; ++i) {
        const XrCompositionLayerBaseHeader* base = info->layers[i];
        if (!base) continue;
        if (base->type == XR_TYPE_COMPOSITION_LAYER_QUAD) {
            const auto* quad = reinterpret_cast<const XrCompositionLayerQuad*>(base);
            overlaysSent &= BridgeQuad(present, quad->space, quad->subImage, quad->pose, quad->size.width,
                                       quad->size.height, quad->eyeVisibility, quad->layerFlags);
            continue;
        }
        if (base->type == XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR) {
            // Sent flat, as the preview draws it
            const auto* cylinder = reinterpret_cast<const XrCompositionLayerCylinderKHR*>(base);
            float width = cylinder->radius * cylinder->centralAngle;
            float height = cylinder->aspectRatio > 0.0f ? width / cylinder->aspectRatio : 1.0f;
            overlaysSent &= BridgeQuad(present, cylinder->space, cylinder->subImage, cylinder->pose, width, height,
                                       cylinder->eyeVisibility, cylinder->layerFlags);
            continue;
        }
        if (base->type != XR_TYPE_COMPOSITION_LAYER_PROJECTION || present.view_count) continue;
        const auto* proj = reinterpret_cast<const XrCompositionLayerProjection*>(base);
        present.space = (uint64_t)(uintptr_t)proj->space;
        for (uint32_t v = 0; v < proj->viewCount && v < WXR_BRIDGE_MAX_VIEWS; ++v) {
            const XrCompositionLayerProjectionView& view = proj->views[v];
            uint32_t id;
            if (!BridgeSlot(view.subImage, id)) break;
            wxr_bridge_present_view& out = present.views[present.view_count++];
            out.id = id;
            out.layer = view.subImage.imageArrayIndex;
            out.rect[0] = view.subImage.imageRect.offset.x;
            out.rect[1] = view.subImage.imageRect.offset.y;
            out.rect[2] = view.subImage.imageRect.extent.width;
            out.rect[3] = view.subImage.imageRect.extent.height;
            // Same zero-sized rect the preview guards against (Maquette): the host sizes its eye
            // swapchains from this, so send the whole image rather than an empty rect. A negative
            // extent is a flip (OpenComposite's inverted texture bounds, Distance) and goes as is
            if (out.rect[2] == 0 || out.rect[3] == 0) {
                auto chainIt = rt::g_swapchains.find(view.subImage.swapchain);
                if (chainIt == rt::g_swapchains.end()) { present.view_count = 0; break; }
                out.rect[0] = out.rect[1] = 0;
                out.rect[2] = (int32_t)chainIt->second.width;
                out.rect[3] = (int32_t)chainIt->second.height;
            }
            memcpy(out.orientation, &view.pose.orientation, sizeof(out.orientation));
            memcpy(out.position, &view.pose.position, sizeof(out.position));
            memcpy(out.fov, &view.fov, sizeof(out.fov));
        }
        if (present.view_count != proj->viewCount) present.view_count = 0;  // all eyes or none
    }
    if (!present.view_count && !present.quad_count) return false;
    present.frame = (uint64_t)frameCount;
    present.display_time = info->displayTime;

    static ComPtr<ID3D11Query> queries[2];
    static wxr_bridge_present_args pending{};
    static int pendingQuery = -1;
    // A fence lets the sender thread sleep on an event until the frame is rendered; the queries remain the fallback
    static ComPtr<ID3D11Fence> fence;
    static ComPtr<ID3D11DeviceContext4> context4;
    static UINT64 fenceValue = 0;
    static ID3D11Device* fenceDevice = nullptr;  // a new session brings a new device
    if (fenceDevice != s.d3d11Device.Get()) {
        fenceDevice = s.d3d11Device.Get();
        fence.Reset();
        context4.Reset();
        fenceValue = 0;
        ComPtr<ID3D11Device5> device5;
        if (FAILED(s.d3d11Device.As(&device5)) || FAILED(s.d3d11Context.As(&context4)) ||
            FAILED(device5->CreateFence(0, D3D11_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)))) {
            fence.Reset();
            context4.Reset();
        }
        Logf("[SimXR] direct_transport: waiting on %s", fence ? "a D3D11 fence" : "D3D11 queries");
    }
    for (auto& query : queries) {
        D3D11_QUERY_DESC desc{D3D11_QUERY_EVENT, 0};
        if (!fence && !query && FAILED(s.d3d11Device->CreateQuery(&desc, query.GetAddressOf()))) return false;
    }
    int current = frameCount & 1;
    if (fence) context4->Signal(fence.Get(), ++fenceValue);
    else s.d3d11Context->End(queries[current].Get());
    s.d3d11Context->Flush();
    if (fence) return BridgeSenderSubmit(present, fence.Get(), fenceValue);

    // Queries can only be read on this thread, so here the wait is for the previous frame, which
    // then reaches the host one frame late
    bool sent = false;
    if (pendingQuery >= 0) {
        LARGE_INTEGER freq, start, end;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&start);
        while (s.d3d11Context->GetData(queries[pendingQuery].Get(), nullptr, 0, 0) == S_FALSE) {
            QueryPerformanceCounter(&end);
            if ((end.QuadPart - start.QuadPart) * 1000 > freq.QuadPart * 100) break;  // give up after 100 ms
            YieldProcessor();
        }
        QueryPerformanceCounter(&end);

        {
            std::lock_guard<std::mutex> lock(g_bridgeMutex);
            g_bridgeCall(WXR_BRIDGE_PRESENT, &pending);
        }
        sent = pending.sent != 0;
        if (frameCount <= 3 || frameCount % 300 == 0) {
            Logf("[SimXR] direct_transport: frame %d, %u view(s), %u quad(s), result %d, %s, CPU wait %.2f ms",
                 (int)pending.frame, pending.view_count, pending.quad_count, pending.result,
                 sent ? "sent to host" : "host not connected",
                 (double)(end.QuadPart - start.QuadPart) * 1000.0 / (double)freq.QuadPart);
        }
    }
    pending = present;
    pendingQuery = current;
    return sent;
}

static XrResult XRAPI_PTR xrEndFrame_runtime(XrSession, const XrFrameEndInfo* info) {
    static int frameCount = 0;
    frameCount++;

    // The preview window dies with the thread that created it (UE4 restarts its render thread after loading), so make a new one on this thread
    if (rt::g_session.hwnd && !IsWindow(rt::g_session.hwnd)) {
        Log("[SimXR] Preview window was destroyed with its thread, recreating it");
        rt::g_session.hwnd = nullptr;
        rt::g_session.previewSwapchain.Reset();
        rt::ResetD3D12PreviewResources(rt::g_session);
    }

    // Log every frame for first 10 frames, then every 60 frames
    bool shouldLog = (frameCount <= 10) || (frameCount % 60 == 1);

    if (shouldLog && verboseLogging) {
        Logf("[SimXR] xrEndFrame called (frame #%d)", frameCount);
    }

    if (!info) {
        Log("[SimXR] xrEndFrame: ERROR - info is null");
        return XR_ERROR_VALIDATION_FAILURE;
    }

    if (shouldLog && verboseLogging) {
        Logf("[SimXR] xrEndFrame: layers=%u", info->layerCount);
    }

    // First pass: count layer types to know if we need to defer Present
    int projectionCount = 0, quadCount = 0, cylinderCount = 0, otherCount = 0;
    for (uint32_t i = 0; i < info->layerCount; ++i) {
        const XrCompositionLayerBaseHeader* base = info->layers[i];
        if (!base) continue;
        switch (base->type) {
            case XR_TYPE_COMPOSITION_LAYER_PROJECTION: projectionCount++; break;
            case XR_TYPE_COMPOSITION_LAYER_QUAD: quadCount++; break;
            case XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR: cylinderCount++; break;
            default: otherCount++; break;
        }
    }

    bool overlaysSent = false;
    bool direct = SubmitProjectionToBridge(info, frameCount, overlaysSent);

    // Determine if we need to defer Present for overlay layers
    bool hasOverlays = (quadCount > 0 || cylinderCount > 0);
    g_presentPending = false;

    // The host shows the direct frames, so the preview window only costs GPU time; it is still
    // drawn when an overlay layer could not go with them
    bool skipPreview = direct && overlaysSent && rt::g_session.hwnd;
    if (skipPreview) {
        MSG msg;
        while (PeekMessageW(&msg, rt::g_session.hwnd, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    }

    // Second pass: render projection layers (background)
    // If there are overlays, skip Present until after they're rendered
    for (uint32_t i = 0; i < info->layerCount; ++i) {
        const XrCompositionLayerBaseHeader* base = info->layers[i];
        if (!base) continue;

        if (base->type == XR_TYPE_COMPOSITION_LAYER_PROJECTION && !skipPreview) {
            const auto* proj = reinterpret_cast<const XrCompositionLayerProjection*>(base);
            presentProjection(rt::g_session, *proj, hasOverlays);  // skipPresent if overlays pending
        }
    }

    // Third pass: render overlay layers (quad, cylinder) on top of the projection
    for (uint32_t i = 0; i < info->layerCount && !skipPreview; ++i) {
        const XrCompositionLayerBaseHeader* base = info->layers[i];
        if (!base) continue;

        switch (base->type) {
            case XR_TYPE_COMPOSITION_LAYER_QUAD: {
                const auto* quad = reinterpret_cast<const XrCompositionLayerQuad*>(base);
                renderQuadLayer(rt::g_session, quad);
                break;
            }
            case XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR: {
                const auto* cylinder = reinterpret_cast<const XrCompositionLayerCylinderKHR*>(base);
                // Render quad instead of cylinder
                XrCompositionLayerQuad quad{};
                quad.type = XR_TYPE_COMPOSITION_LAYER_QUAD;
                quad.layerFlags = cylinder->layerFlags;
                quad.space      = cylinder->space;
                quad.eyeVisibility = cylinder->eyeVisibility;
                quad.pose       = cylinder->pose;
                quad.subImage = cylinder->subImage;
                quad.size.width  = cylinder->radius * cylinder->centralAngle;
                quad.size.height = cylinder->aspectRatio > 0.0f ? quad.size.width / cylinder->aspectRatio : 1.0f;
                renderQuadLayer(rt::g_session, &quad);
                break;
            }
            default:
                break;
        }
    }

    // Now Present after all layers are rendered
    if (g_presentPending) {
        auto& s = rt::g_session;
        MSG msg;
        while (PeekMessageW(&msg, s.hwnd, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessageW(&msg); }

        if (s.usesVulkan) {
            presentVulkanSwapchain(s, rt::g_last_proj);
        } else if (s.usesD3D12 && s.previewSwapchain12) {
            s.previewSwapchain12->Present(1, 0);
        } else if (s.previewSwapchain) {
            s.previewSwapchain->Present(1, 0);
        }
        g_presentPending = false;
    }

    if (shouldLog && (quadCount > 0 || cylinderCount > 0) && verboseLogging) {
        Logf("[SimXR] xrEndFrame: proj=%d quad=%d cyl=%d other=%d",
             projectionCount, quadCount, cylinderCount, otherCount);
    }

    // Once a session has drawn in stereo it stays VR until it ends. While a game loads,
    // OpenComposite submits the skybox override as a quad layer, or no layer at all when its
    // texture cannot be used, and leaving VR mode for those frames breaks the view (Maquette).
    if (projectionCount > 0) rt::g_sessionWasVR = true;
    isVR = rt::g_sessionWasVR || quadCount > 0 || cylinderCount > 0;
    static bool lastIsVR = false;
    static ULONGLONG lastSendMs = 0;
    if (isVR != lastIsVR || GetTickCount64() - lastSendMs >= 500) {
        sendUdpData();
        lastIsVR = isVR;
        lastSendMs = GetTickCount64();
    }

    if (projectionCount == 0 && shouldLog) {
        Log("[SimXR] xrEndFrame: WARNING - No projection layers found!");
    }

    return XR_SUCCESS;
}

// Add missing space/action functions for compatibility
static XrResult XRAPI_PTR xrCreateReferenceSpace_runtime(XrSession, const XrReferenceSpaceCreateInfo* info, XrSpace* space) {
    if (!info || !space) return XR_ERROR_VALIDATION_FAILURE;
    static uintptr_t nextSpace = 100;
    *space = (XrSpace)(nextSpace++);
    rt::g_referenceSpacePose[*space] = info->poseInReferenceSpace;
    rt::g_referenceSpaceType[*space] = info->referenceSpaceType;
    rt::g_spacesChanged = true;
    Logf("[SimXR] xrCreateReferenceSpace: type=%d space=%d", info->referenceSpaceType, *space);
    return XR_SUCCESS;
}

static XrResult XRAPI_PTR xrDestroySpace_runtime(XrSpace space) {
    Logf("[SimXR] xrDestroySpace: space=%p", space);
    if (rt::g_referenceSpacePose.find(space) != rt::g_referenceSpacePose.end()) {
        rt::g_referenceSpacePose.erase(space);
        rt::g_referenceSpaceType.erase(space);
    }
    {
        std::lock_guard<std::mutex> lock(rt::g_controllerMutex);
        if (rt::g_controllerSpaces.find(space) != rt::g_controllerSpaces.end()) {
            rt::g_controllerSpaces.erase(space);
            rt::g_controllerGrips.erase(space);
        }
    }
    for (auto it = rt::g_locateSpaces.begin(); it != rt::g_locateSpaces.end(); ) {
        const auto& [spaceA, spaceB] = it->first;
        if (spaceA == space || spaceB == space)
            it = rt::g_locateSpaces.erase(it);
        else
            ++it;
    }
    rt::g_spacesChanged = true;
    return XR_SUCCESS;
}

static XrResult XRAPI_PTR xrLocateSpace_runtime(XrSpace space, XrSpace baseSpace, XrTime time, XrSpaceLocation* location) {
    if (!location) return XR_ERROR_VALIDATION_FAILURE;
    location->type = XR_TYPE_SPACE_LOCATION;

    rt::DrainPendingControllers();

    // Check if the space is being reported to the wrapper
    std::pair<XrSpace, XrSpace> key;
    key.first = space;
    key.second = baseSpace;
    if (rt::g_locateSpaces.find(key) == rt::g_locateSpaces.end()) {
        rt::g_locateSpaces[key] = true;
        rt::g_spacesChanged = true;
    }

    if (verboseLogging) Logf("[SimXR] xrLocateSpace: Looking for space=%d in baseSpace=%d", (uint64_t)space, (uint64_t)baseSpace);

    if (SpacePoses.find(key) != SpacePoses.end()) {
        location->locationFlags = XR_SPACE_LOCATION_POSITION_VALID_BIT |
                                  XR_SPACE_LOCATION_ORIENTATION_VALID_BIT |
                                  XR_SPACE_LOCATION_POSITION_TRACKED_BIT |
                                  XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT;
        location->pose = SpacePoses[key];
    } else {
        // Default for invalid data
        location->locationFlags = 0;
        location->pose.orientation = {0, 0, 0, 1};
        location->pose.position = {0, 0, 0};
    }

    for (auto* next = reinterpret_cast<XrBaseOutStructure*>(location->next); next; next = next->next) {
        if (next->type == XR_TYPE_SPACE_VELOCITY) {
            auto* velocity = reinterpret_cast<XrSpaceVelocity*>(next);
            auto it = SpaceVelocities.find(key);
            if (it != SpaceVelocities.end() && location->locationFlags) {
                velocity->velocityFlags = it->second.velocityFlags;
                velocity->linearVelocity = it->second.linearVelocity;
                velocity->angularVelocity = it->second.angularVelocity;
            } else {
                velocity->velocityFlags = 0;
            }
        }
    }

    return XR_SUCCESS;
}

// OpenXR 1.1: xrLocateSpace for several spaces in one call
static XrResult XRAPI_PTR xrLocateSpaces_runtime(XrSession, const XrSpacesLocateInfo* locateInfo, XrSpaceLocations* spaceLocations) {
    if (!locateInfo || !spaceLocations) return XR_ERROR_VALIDATION_FAILURE;
    if (locateInfo->spaceCount == 0 || spaceLocations->locationCount != locateInfo->spaceCount ||
        !locateInfo->spaces || !spaceLocations->locations) return XR_ERROR_VALIDATION_FAILURE;

    XrSpaceVelocities* velocities = nullptr;
    for (auto* next = reinterpret_cast<XrBaseOutStructure*>(spaceLocations->next); next; next = next->next) {
        if (next->type == XR_TYPE_SPACE_VELOCITIES) velocities = reinterpret_cast<XrSpaceVelocities*>(next);
    }
    if (velocities && (velocities->velocityCount != locateInfo->spaceCount || !velocities->velocities)) return XR_ERROR_VALIDATION_FAILURE;

    for (uint32_t i = 0; i < locateInfo->spaceCount; ++i) {
        XrSpaceVelocity velocity{XR_TYPE_SPACE_VELOCITY};
        XrSpaceLocation location{XR_TYPE_SPACE_LOCATION};
        location.next = velocities ? &velocity : nullptr;
        XrResult result = xrLocateSpace_runtime(locateInfo->spaces[i], locateInfo->baseSpace, locateInfo->time, &location);
        if (XR_FAILED(result)) return result;
        spaceLocations->locations[i].locationFlags = location.locationFlags;
        spaceLocations->locations[i].pose = location.pose;
        if (velocities) {
            velocities->velocities[i].velocityFlags = velocity.velocityFlags;
            velocities->velocities[i].linearVelocity = velocity.linearVelocity;
            velocities->velocities[i].angularVelocity = velocity.angularVelocity;
        }
    }
    return XR_SUCCESS;
}

// The part of each eye's image the lenses hide. Only the headset knows its shape, so the host asks its own
// runtime and leaves the answer in a file: one record per eye and mask type, "eye type vertexCount indexCount"
// followed by the vertices (x y pairs in view space at z=-1) and the indices. No file means no mask.
static XrResult XRAPI_PTR xrGetVisibilityMaskKHR_runtime(XrSession, XrViewConfigurationType, uint32_t viewIndex,
                                                         XrVisibilityMaskTypeKHR visibilityMaskType, XrVisibilityMaskKHR* visibilityMask) {
    if (!visibilityMask || viewIndex > 1) return XR_ERROR_VALIDATION_FAILURE;

    std::vector<XrVector2f> vertices;
    std::vector<uint32_t> indices;
    // One image is shown to both eyes then, and each eye's lens hides a different part of it
    if (!monoRendering && !disableRightEye) {
        std::ifstream maskFile("Z:/tmp/xr/mask");
        maskFile.imbue(std::locale::classic());
        uint32_t eye, type, vertexCount, indexCount;
        while (maskFile >> eye >> type >> vertexCount >> indexCount) {
            if (vertexCount > 65536 || indexCount > 65536) break;
            std::vector<XrVector2f> v(vertexCount);
            std::vector<uint32_t> idx(indexCount);
            for (auto& vertex : v) maskFile >> vertex.x >> vertex.y;
            for (auto& index : idx) maskFile >> index;
            if (!maskFile) break;
            if (eye == viewIndex && type == (uint32_t)visibilityMaskType) {
                vertices = std::move(v);
                indices = std::move(idx);
                break;
            }
        }
    }

    static bool logged = false;
    if (!logged) Logf("[SimXR] xrGetVisibilityMaskKHR: eye=%u type=%d -> %zu vertices, %zu indices",
                      viewIndex, (int)visibilityMaskType, vertices.size(), indices.size());
    logged = true;

    visibilityMask->vertexCountOutput = (uint32_t)vertices.size();
    visibilityMask->indexCountOutput = (uint32_t)indices.size();
    if (visibilityMask->vertexCapacityInput == 0 && visibilityMask->indexCapacityInput == 0) return XR_SUCCESS;
    if (visibilityMask->vertexCapacityInput < vertices.size() || visibilityMask->indexCapacityInput < indices.size()) {
        return XR_ERROR_SIZE_INSUFFICIENT;
    }
    if (!vertices.empty() && visibilityMask->vertices) std::memcpy(visibilityMask->vertices, vertices.data(), vertices.size() * sizeof(XrVector2f));
    if (!indices.empty() && visibilityMask->indices) std::memcpy(visibilityMask->indices, indices.data(), indices.size() * sizeof(uint32_t));
    return XR_SUCCESS;
}

static XrResult XRAPI_PTR xrLocateViews_runtime(XrSession, const XrViewLocateInfo* li, XrViewState* vs, uint32_t cap, uint32_t* outCount, XrView* views) {
    if (outCount) *outCount = 2;
    if (vs) { 
        vs->type = XR_TYPE_VIEW_STATE; 
        // Set both VALID and TRACKED bits so Unity knows this is a real tracked HMD
        vs->viewStateFlags = XR_VIEW_STATE_ORIENTATION_VALID_BIT | 
                            XR_VIEW_STATE_POSITION_VALID_BIT | 
                            XR_VIEW_STATE_ORIENTATION_TRACKED_BIT | 
                            XR_VIEW_STATE_POSITION_TRACKED_BIT; 
    }
    if (cap < 2 || !views) return XR_SUCCESS;

    if (rt::g_spacesChanged) {
        //Now we send the VR mode enable and target FOV of WinlatorXR
        sendUdpData();
        rt::g_spacesChanged = false;
    }

    // Before the first xrWaitFrame the FOV is still the -1 placeholder, which swaps left/right and up/down;
    // games that cache their projection at startup (Project CARS) then render upside down, so ask the host now
    if (FOVH < 0 || FOVV < 0) {
        WxrHostState host;
        if (!udpReader->GetState(host)) {
            static bool logged = false;
            if (!logged) Log("[SimXR] xrLocateViews: ERROR - the UDP port is not bound so no data can come from the host, failing the call");
            logged = true;
            return XR_ERROR_RUNTIME_FAILURE;
        }
        IPDVal = host.floats[8];
        FOVH = host.floats[9] * toRadians;
        FOVV = host.floats[10] * toRadians;
    }

    XrSpaceLocation loc{XR_TYPE_SPACE_LOCATION};
    xrLocateSpace_runtime((XrSpace)0, li->space, (XrTime)0, &loc);

    for (uint32_t i = 0; i < 2; ++i) {
        views[i].type = XR_TYPE_VIEW;
        views[i].pose.orientation = loc.pose.orientation;

        // Apply IPD offset in full head orientation space (yaw+pitch)
        // This fixes stereo geometry and eliminates warping when pitching
        float eyeOffset = (i == 0 ? -IPDVal * 0.5f : IPDVal * 0.5f);

        XrVector3f rotatedOffset;
        XrVector3f localEyeOffset{ disableRightEye ? i * 1000000000.0f : eyeOffset, 0.0f, 0.0f };
        XrQuaternionf_RotateVector3f(&rotatedOffset, &loc.pose.orientation, &localEyeOffset);
        XrVector3f_Add(&views[i].pose.position, &loc.pose.position, &rotatedOffset);

        // FOV from API
        float fovX = FOVH / 2.0f;
        float fovY = FOVV / 2.0f;
        views[i].fov = { -fovX, fovX, fovY, -fovY };
    }
    return XR_SUCCESS;
}

static XrResult XRAPI_PTR xrEnumerateReferenceSpaces_runtime(XrSession, uint32_t capacity, uint32_t* count, XrReferenceSpaceType* spaces) {
    // LOCAL_FLOOR is part of OpenXR 1.1, so only an app that asked for 1.1 is told about it
    const uint32_t spaceCount = XR_VERSION_MINOR(rt::g_instance.apiVersion) >= 1 ? 4 : 3;
    if (count) *count = spaceCount;
    if (capacity >= spaceCount && spaces) {
        spaces[0] = XR_REFERENCE_SPACE_TYPE_VIEW;
        spaces[1] = XR_REFERENCE_SPACE_TYPE_LOCAL;
        spaces[2] = XR_REFERENCE_SPACE_TYPE_STAGE;
        if (spaceCount > 3) spaces[3] = XR_REFERENCE_SPACE_TYPE_LOCAL_FLOOR;
    }
    return XR_SUCCESS;
}

static XrResult XRAPI_PTR xrCreateActionSpace_runtime(XrSession, const XrActionSpaceCreateInfo* info, XrSpace* space) {
    if (!info) return XR_ERROR_VALIDATION_FAILURE;
    static uintptr_t nextSpace = 200;
    *space = (XrSpace)(nextSpace++);

    {
        std::lock_guard<std::mutex> lock(rt::g_controllerMutex);
        rt::g_controllerInfo[*space] = XrActionSpaceCreateInfo();
        memcpy(&rt::g_controllerInfo[*space], info, sizeof(XrActionSpaceCreateInfo));
    }

    Logf("[SimXR] xrCreateActionSpace: space=%llu", (unsigned long long) * space);

    return XR_SUCCESS;
}

static XrResult XRAPI_PTR xrCreateActionSet_runtime(XrInstance, const XrActionSetCreateInfo* info, XrActionSet* set) {
    if (!info || !set) return XR_ERROR_VALIDATION_FAILURE;
    static uintptr_t nextSet = 300;
    *set = (XrActionSet)(nextSet++);
    // actionSetName may not be null-terminated
    char setName[XR_MAX_ACTION_SET_NAME_SIZE + 1] = {0};
    memcpy(setName, info->actionSetName, XR_MAX_ACTION_SET_NAME_SIZE);
    Logf("[SimXR] xrCreateActionSet: name=%s", setName);
    return XR_SUCCESS;
}

static XrResult XRAPI_PTR xrDestroyActionSet_runtime(XrActionSet set) {
    Log("[SimXR] xrDestroyActionSet");
    return XR_SUCCESS;
}

static XrResult XRAPI_PTR xrCreateAction_runtime(XrActionSet, const XrActionCreateInfo* info, XrAction* action) {
    if (!info || !action) return XR_ERROR_VALIDATION_FAILURE;
    static uintptr_t nextAction = 400;
    *action = (XrAction)(nextAction++);
    // actionName may not be null-terminated
    char actName[XR_MAX_ACTION_NAME_SIZE + 1] = {0};
    memcpy(actName, info->actionName, XR_MAX_ACTION_NAME_SIZE);

    // Store action name for input mapping
    rt::g_actionNames[*action] = actName;

    Logf("[SimXR] xrCreateAction: name=%s, type=%d", actName, info->actionType);

    return XR_SUCCESS;
}

static XrResult XRAPI_PTR xrDestroyAction_runtime(XrAction action) {
    Log("[SimXR] xrDestroyAction");
    return XR_SUCCESS;
}

static XrResult XRAPI_PTR xrSuggestInteractionProfileBindings_runtime(XrInstance, const XrInteractionProfileSuggestedBinding* bindings) {
    if (!bindings) return XR_ERROR_VALIDATION_FAILURE;

    for (int i = 0; i < bindings->countSuggestedBindings; i++) {
        auto& binding = bindings->suggestedBindings[i];
        if (rt::g_actionNames.find(binding.action) == rt::g_actionNames.end()) {
            Logf("[SimXR] xrSuggestInteractionProfileBindings: unknown action %d", binding.action);
            continue;
        }
        if (rt::g_pathStrings.find(binding.binding) == rt::g_pathStrings.end()) {
            Logf("[SimXR] xrSuggestInteractionProfileBindings: unknown binding %d", binding.binding);
            continue;
        }
        Logf("[SimXR] xrSuggestInteractionProfileBindings: action=%s, binding=%s",
             rt::g_actionNames[binding.action].c_str(), rt::g_pathStrings[binding.binding].c_str());
        rt::g_actionPaths[binding.action] = binding.binding;
        auto& bindingList = rt::g_actionBindings[binding.action];
        auto sameBinding = [&](const rt::ActionBinding& b) {
            return b.profile == bindings->interactionProfile && b.binding == binding.binding;
        };
        if (std::find_if(bindingList.begin(), bindingList.end(), sameBinding) == bindingList.end()) {
            bindingList.push_back({bindings->interactionProfile, binding.binding});
        }
    }
    if (!rt::g_interactionProfiles[bindings->interactionProfile]) {
        rt::g_interactionProfiles[bindings->interactionProfile] = true;
        rt::g_profileOrder.push_back(bindings->interactionProfile);
    }
    return XR_SUCCESS;
}

// How well a profile maps onto the emulated controller, which has a thumbstick, two face
// buttons, a trigger, a grip and a menu button. A trackpad-only wand does not, so it is
// only picked when the app suggested nothing better. Lower is better.
static const int kProfileRankUnknown = 5;

static int InteractionProfileRank(const std::string& profile) {
    if (!preferredProfile.empty() && profile.find(preferredProfile) != std::string::npos) return -1;
    if (profile.find("/valve/index_controller") != std::string::npos) return 0;
    if (profile.find("/oculus/touch_controller") != std::string::npos) return 1;
    if (profile.find("/microsoft/motion_controller") != std::string::npos) return 2;
    if (profile.find("/khr/simple_controller") != std::string::npos) return 3;
    if (profile.find("/htc/vive_controller") != std::string::npos) return 4;
    return kProfileRankUnknown;
}

// The profile the app's bindings are read through. Index ranks first, so this picks the
// same profile xrGetCurrentInteractionProfile used to report.
static XrPath SelectInteractionProfile() {
    XrPath best = XR_NULL_PATH;
    int bestRank = kProfileRankUnknown + 1;
    for (XrPath profile : rt::g_profileOrder) {
        auto pathIt = rt::g_pathStrings.find(profile);
        if (pathIt == rt::g_pathStrings.end()) continue;
        int rank = InteractionProfileRank(pathIt->second);
        if (rank < bestRank) {
            bestRank = rank;
            best = profile;
        }
    }
    return best;
}

static XrResult XRAPI_PTR xrAttachSessionActionSets_runtime(XrSession, const XrSessionActionSetsAttachInfo* info) {
    if (!info) return XR_ERROR_VALIDATION_FAILURE;
    Logf("[SimXR] xrAttachSessionActionSets: count=%u", info->countActionSets);

    // Attaching freezes the bindings, so this is where the profile is chosen
    rt::g_activeProfile = SelectInteractionProfile();
    auto profileIt = rt::g_pathStrings.find(rt::g_activeProfile);
    Logf("[SimXR] xrAttachSessionActionSets: active interaction profile=%s",
         profileIt != rt::g_pathStrings.end() ? profileIt->second.c_str() : "none");

    XrEventDataSessionStateChanged e{XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED};
    XrEventDataBuffer buf{};
    buf.type = XR_TYPE_EVENT_DATA_BUFFER;  // Set the base type
    std::memcpy(&buf, &e, sizeof(e));
    rt::g_eventQueue.push_back(buf);

    return XR_SUCCESS;
}

static std::string GetActionName(XrAction action);

// Hand bitmask: 0 = unknown, HAND_LEFT = left, HAND_RIGHT = right, both bits = either hand
enum { HAND_NONE = 0, HAND_LEFT = 1, HAND_RIGHT = 2 };

static int HandFromPathString(const std::string& path) {
    if (path.find("left") != std::string::npos) return HAND_LEFT;
    if (path.find("right") != std::string::npos) return HAND_RIGHT;
    return HAND_NONE;
}

// Which hand(s) an action query refers to. Returns HAND_LEFT or HAND_RIGHT when the
// answer is unambiguous, otherwise HAND_NONE - meaning the action covers both hands
// (or has no usable binding) and the caller has to combine them rather than guess.
static int ResolveActionHand(XrAction action, XrPath subactionPath) {
    // A subaction path is authoritative per the OpenXR spec: the app is explicitly asking
    // for one hand, so honour it regardless of what the suggested bindings look like.
    if (subactionPath != XR_NULL_PATH) {
        auto pathIt = rt::g_pathStrings.find(subactionPath);
        if (pathIt != rt::g_pathStrings.end()) {
            int hand = HandFromPathString(pathIt->second);
            if (hand != HAND_NONE) {
                if (verboseLogging) Logf("[SimXR] ResolveActionHand: subactionPath='%s' -> %s",
                    pathIt->second.c_str(), hand == HAND_LEFT ? "LEFT" : "RIGHT");
                return hand;
            }
        }
    }

    // Otherwise fall back to the suggested bindings. Look at all of them: an action
    // suggested on both hands used to keep only whichever binding arrived last.
    int hands = HAND_NONE;
    auto bindIt = rt::g_actionBindings.find(action);
    if (bindIt != rt::g_actionBindings.end()) {
        for (const rt::ActionBinding& bound : bindIt->second) {
            auto pathIt = rt::g_pathStrings.find(bound.binding);
            if (pathIt == rt::g_pathStrings.end()) continue;
            hands |= HandFromPathString(pathIt->second);
        }
    }
    if (hands == HAND_LEFT || hands == HAND_RIGHT) {
        if (verboseLogging) Logf("[SimXR] ResolveActionHand: action-bound -> %s",
            hands == HAND_LEFT ? "LEFT" : "RIGHT");
        return hands;
    }

    if (verboseLogging) {
        std::string actionName = GetActionName(action);
        Logf("[SimXR] ResolveActionHand: AMBIGUOUS for action='%s' (subactionPath valid=%d, bound hands=%d) - combining both",
            actionName.c_str(), subactionPath != XR_NULL_PATH, hands);
    }
    return HAND_NONE;
}

static rt::ControllerState* ControllerForHand(int hand) {
    return hand == HAND_LEFT ? &rt::g_leftController : &rt::g_rightController;
}

// The controller component a binding points at. COMP_NONE means the action has no source on
// the queried hand; COMP_LEGACY means it has no suggested binding at all, so the name matching
// below still answers it.
enum ActionComponent {
    COMP_NONE = 0, COMP_TRIGGER, COMP_TRIGGER_TOUCH, COMP_SQUEEZE, COMP_MENU, COMP_SYSTEM,
    COMP_BUTTON_A, COMP_BUTTON_B, COMP_THUMBREST,
    COMP_THUMBSTICK, COMP_THUMBSTICK_X, COMP_THUMBSTICK_Y, COMP_THUMBSTICK_CLICK, COMP_THUMBSTICK_TOUCH,
    COMP_TRACKPAD, COMP_TRACKPAD_X, COMP_TRACKPAD_Y, COMP_TRACKPAD_CLICK, COMP_TRACKPAD_TOUCH, COMP_TRACKPAD_FORCE,
    COMP_POSE, COMP_HAPTIC, COMP_LEGACY
};

// Splits a binding path such as /user/hand/left/input/thumbstick/x into its component, rather
// than searching the whole path for a substring that may belong to another part of it
static ActionComponent ComponentFromPathString(const std::string& path) {
    size_t inputPos = path.find("/input/");
    if (inputPos == std::string::npos) {
        return path.find("/output/haptic") != std::string::npos ? COMP_HAPTIC : COMP_NONE;
    }
    std::string tail = path.substr(inputPos + 7);
    std::string component = tail, sub;
    if (size_t slash = tail.find('/'); slash != std::string::npos) {
        component = tail.substr(0, slash);
        sub = tail.substr(slash + 1);
    }
    // The simple controller's only button is select, which every profile above maps to the trigger
    if (component == "trigger" || component == "select") return sub == "touch" ? COMP_TRIGGER_TOUCH : COMP_TRIGGER;
    if (component == "squeeze") return COMP_SQUEEZE;
    if (component == "menu" || component == "back") return COMP_MENU;
    if (component == "system") return COMP_SYSTEM;
    if (component == "a" || component == "x") return COMP_BUTTON_A;
    if (component == "b" || component == "y") return COMP_BUTTON_B;
    if (component == "thumbrest") return COMP_THUMBREST;
    if (component == "thumbstick" || component == "joystick") {
        if (sub == "x") return COMP_THUMBSTICK_X;
        if (sub == "y") return COMP_THUMBSTICK_Y;
        if (sub == "click") return COMP_THUMBSTICK_CLICK;
        if (sub == "touch") return COMP_THUMBSTICK_TOUCH;
        return COMP_THUMBSTICK;
    }
    if (component == "trackpad") {
        if (sub == "x") return COMP_TRACKPAD_X;
        if (sub == "y") return COMP_TRACKPAD_Y;
        if (sub == "click") return COMP_TRACKPAD_CLICK;
        if (sub == "touch") return COMP_TRACKPAD_TOUCH;
        if (sub == "force") return COMP_TRACKPAD_FORCE;
        return COMP_TRACKPAD;
    }
    // grip and aim are poses, not the squeeze button the old name matching read them as
    if (component == "grip" || component == "aim" || component == "palm_ext" || component == "grip_surface") return COMP_POSE;
    return COMP_NONE;
}

static bool IsEmulatedTrackpad(ActionComponent component) {
    return component == COMP_TRACKPAD || component == COMP_TRACKPAD_X || component == COMP_TRACKPAD_Y ||
           component == COMP_TRACKPAD_CLICK || component == COMP_TRACKPAD_TOUCH || component == COMP_TRACKPAD_FORCE;
}

// What the action reads on a hand. A component the controller really has beats the trackpad
// emulated from its thumbstick, and within each of those the profile the session was attached
// with wins - an action is routinely suggested on several profiles and on both hands.
static ActionComponent ResolveComponentForHand(XrAction action, int hand) {
    auto bindIt = rt::g_actionBindings.find(action);
    if (bindIt == rt::g_actionBindings.end() || bindIt->second.empty()) return COMP_LEGACY;

    // An action bound on the attached profile is fully described by it, so a binding from another
    // profile must not make the other hand look bound too - Aircar puts its right-stick action on
    // the Vive left trackpad, which would otherwise answer on the left hand as well
    bool boundOnActiveProfile = false;
    for (const rt::ActionBinding& bound : bindIt->second)
        if (bound.profile == rt::g_activeProfile) { boundOnActiveProfile = true; break; }

    ActionComponent real = COMP_NONE, realOther = COMP_NONE;
    ActionComponent trackpad = COMP_NONE, trackpadOther = COMP_NONE;
    for (const rt::ActionBinding& bound : bindIt->second) {
        if (boundOnActiveProfile && bound.profile != rt::g_activeProfile) continue;
        auto pathIt = rt::g_pathStrings.find(bound.binding);
        if (pathIt == rt::g_pathStrings.end()) continue;
        if (hand != HAND_NONE && HandFromPathString(pathIt->second) != hand) continue;
        ActionComponent component = ComponentFromPathString(pathIt->second);
        if (component == COMP_NONE) continue;
        bool onActiveProfile = bound.profile == rt::g_activeProfile;
        ActionComponent& slot = IsEmulatedTrackpad(component)
            ? (onActiveProfile ? trackpad : trackpadOther)
            : (onActiveProfile ? real : realOther);
        if (slot == COMP_NONE) slot = component;
    }
    if (real != COMP_NONE) return real;
    if (realOther != COMP_NONE) return realOther;
    return trackpad != COMP_NONE ? trackpad : trackpadOther;
}

static bool EvalComponentBoolean(ActionComponent component, rt::ControllerState* ctrl) {
    switch (component) {
        case COMP_TRIGGER: return ctrl->triggerPressed;
        case COMP_SQUEEZE: return ctrl->gripPressed;
        case COMP_MENU: return ctrl->menuPressed;
        case COMP_BUTTON_A: return ctrl->primaryPressed;
        case COMP_BUTTON_B: return ctrl->secondaryPressed;
        case COMP_THUMBSTICK: case COMP_THUMBSTICK_X: case COMP_THUMBSTICK_Y: case COMP_THUMBSTICK_CLICK:
            return ctrl->thumbstickPressed;
        case COMP_THUMBSTICK_TOUCH:
            return ctrl->thumbstickPressed || XrVector2f_Length(&ctrl->thumbstick) > 0.1f;
        case COMP_TRACKPAD_FORCE: return XrVector2f_Length(&ctrl->thumbstick) > 0.5f;
        // A trackpad click or touch never follows the thumbstick (walking would fire teleport); the menu
        // button presses it instead, which only reaches actions bound to nothing else (Batman Arkham VR pause)
        case COMP_TRACKPAD_CLICK: case COMP_TRACKPAD_TOUCH: return ctrl->menuPressed;
        default: return false;
    }
}

static float EvalComponentFloat(ActionComponent component, rt::ControllerState* ctrl) {
    switch (component) {
        case COMP_TRIGGER: return ctrl->triggerValue;
        case COMP_SQUEEZE: return ctrl->gripValue;
        case COMP_THUMBSTICK_X: case COMP_TRACKPAD_X: return ctrl->thumbstick.x;
        case COMP_THUMBSTICK_Y: case COMP_TRACKPAD_Y: return ctrl->thumbstick.y;
        // Read as an axis a trackpad still deflects, which is how games drive analog movement
        case COMP_TRACKPAD_CLICK: case COMP_TRACKPAD_FORCE: return XrVector2f_Length(&ctrl->thumbstick);
        // A boolean source read as a float is 0 or 1
        default: return EvalComponentBoolean(component, ctrl) ? 1.0f : 0.0f;
    }
}

static XrVector2f EvalComponentVector2f(ActionComponent component, rt::ControllerState* ctrl) {
    switch (component) {
        case COMP_THUMBSTICK: case COMP_THUMBSTICK_X: case COMP_THUMBSTICK_Y:
        case COMP_TRACKPAD: case COMP_TRACKPAD_X: case COMP_TRACKPAD_Y:
            return ctrl->thumbstick;
        default: return {0.0f, 0.0f};
    }
}

// Answers a query from the suggested bindings. Returns false when the action has none, or when
// legacy_input_matching is set, and the name matching below answers it instead.
static bool TryTypedBoolean(const XrActionStateGetInfo* info, XrActionStateBoolean* state) {
    if (legacyInputMatching) return false;
    int hand = ResolveActionHand(info->action, info->subactionPath);
    if (hand != HAND_NONE) {
        ActionComponent component = ResolveComponentForHand(info->action, hand);
        if (component == COMP_LEGACY) return false;
        state->currentState = EvalComponentBoolean(component, ControllerForHand(hand)) ? XR_TRUE : XR_FALSE;
        state->isActive = component != COMP_NONE ? XR_TRUE : XR_FALSE;
        return true;
    }
    // Bound on both hands: either hand can press it
    ActionComponent left = ResolveComponentForHand(info->action, HAND_LEFT);
    ActionComponent right = ResolveComponentForHand(info->action, HAND_RIGHT);
    if (left == COMP_LEGACY || right == COMP_LEGACY) return false;
    bool pressed = EvalComponentBoolean(left, &rt::g_leftController) ||
                   EvalComponentBoolean(right, &rt::g_rightController);
    state->currentState = pressed ? XR_TRUE : XR_FALSE;
    state->isActive = (left != COMP_NONE || right != COMP_NONE) ? XR_TRUE : XR_FALSE;
    return true;
}

static bool TryTypedFloat(const XrActionStateGetInfo* info, XrActionStateFloat* state) {
    if (legacyInputMatching) return false;
    int hand = ResolveActionHand(info->action, info->subactionPath);
    if (hand != HAND_NONE) {
        ActionComponent component = ResolveComponentForHand(info->action, hand);
        if (component == COMP_LEGACY) return false;
        state->currentState = EvalComponentFloat(component, ControllerForHand(hand));
        state->isActive = component != COMP_NONE ? XR_TRUE : XR_FALSE;
        return true;
    }
    ActionComponent left = ResolveComponentForHand(info->action, HAND_LEFT);
    ActionComponent right = ResolveComponentForHand(info->action, HAND_RIGHT);
    if (left == COMP_LEGACY || right == COMP_LEGACY) return false;
    // Take the hand pushed further, so a two-hand binding follows the one being used
    float leftValue = EvalComponentFloat(left, &rt::g_leftController);
    float rightValue = EvalComponentFloat(right, &rt::g_rightController);
    state->currentState = fabsf(leftValue) >= fabsf(rightValue) ? leftValue : rightValue;
    state->isActive = (left != COMP_NONE || right != COMP_NONE) ? XR_TRUE : XR_FALSE;
    return true;
}

static bool TryTypedVector2f(const XrActionStateGetInfo* info, XrActionStateVector2f* state) {
    if (legacyInputMatching) return false;
    int hand = ResolveActionHand(info->action, info->subactionPath);
    if (hand != HAND_NONE) {
        ActionComponent component = ResolveComponentForHand(info->action, hand);
        if (component == COMP_LEGACY) return false;
        state->currentState = EvalComponentVector2f(component, ControllerForHand(hand));
        state->isActive = component != COMP_NONE ? XR_TRUE : XR_FALSE;
        return true;
    }
    ActionComponent left = ResolveComponentForHand(info->action, HAND_LEFT);
    ActionComponent right = ResolveComponentForHand(info->action, HAND_RIGHT);
    if (left == COMP_LEGACY || right == COMP_LEGACY) return false;
    XrVector2f leftValue = EvalComponentVector2f(left, &rt::g_leftController);
    XrVector2f rightValue = EvalComponentVector2f(right, &rt::g_rightController);
    state->currentState = XrVector2f_Length(&leftValue) >= XrVector2f_Length(&rightValue) ? leftValue : rightValue;
    state->isActive = (left != COMP_NONE || right != COMP_NONE) ? XR_TRUE : XR_FALSE;
    return true;
}

// Helper to check if action name matches input type (case-insensitive substring match)
static bool ActionNameMatches(const std::string& name, const char* pattern) {
    std::string lower = name;
    for (auto& c : lower) c = (char)tolower(c);
    std::string patLower = pattern;
    for (auto& c : patLower) c = (char)tolower(c);
    return lower.find(patLower) != std::string::npos;
}

// The binding path drives the input-type dispatch below, so it has to come from the
// same hand the state is read from - otherwise a two-hand action can be answered with
// one hand's controller and the other hand's component path.
static std::string GetActionNameForHand(XrAction action, int hand) {
    if (auto bindIt = rt::g_actionBindings.find(action); bindIt != rt::g_actionBindings.end()) {
        std::string fallback, trackpad;
        for (const rt::ActionBinding& bound : bindIt->second) {
            auto pathIt = rt::g_pathStrings.find(bound.binding);
            if (pathIt == rt::g_pathStrings.end()) continue;
            if (hand != HAND_NONE && HandFromPathString(pathIt->second) == hand) {
                // Trackpads are emulated from the thumbstick, so a real input on the same hand wins
                if (pathIt->second.find("/trackpad") == std::string::npos) return pathIt->second;
                if (trackpad.empty()) trackpad = pathIt->second;
            }
            if (fallback.empty()) fallback = pathIt->second;
        }
        if (!trackpad.empty()) return trackpad;
        // A hand the action isn't bound on reads nothing, not its own controller through the other hand's path
        if (!fallback.empty()) return hand == HAND_NONE ? fallback : "unbound";
    }
    if (auto actionIt = rt::g_actionPaths.find(action); actionIt != rt::g_actionPaths.end()) {
        if (auto pathIt = rt::g_pathStrings.find(actionIt->second); pathIt != rt::g_pathStrings.end()) {
            return pathIt->second;
        }
    }
    auto nameIt = rt::g_actionNames.find(action);
    if (nameIt != rt::g_actionNames.end()) {
        return "invalid_" + nameIt->second;
    }
    return "unknown";
}

static std::string GetActionName(XrAction action) {
    return GetActionNameForHand(action, HAND_NONE);
}

static bool EvalActionBoolean(const std::string& name, rt::ControllerState* ctrl, bool& recognized) {
    recognized = true;
    if (ActionNameMatches(name, "thumbstick/touch")) {
        return ctrl->thumbstickPressed || XrVector2f_Length(&ctrl->thumbstick) > 0.1f;
    } else if (ActionNameMatches(name, "/touch")) {
        return false;
    } else if (ActionNameMatches(name, "trigger") || ActionNameMatches(name, "select") || ActionNameMatches(name, "fire")) {
        return ctrl->triggerPressed;
    } else if (ActionNameMatches(name, "grip") || ActionNameMatches(name, "squeeze") || ActionNameMatches(name, "grab")) {
        return ctrl->gripPressed;
    } else if (ActionNameMatches(name, "menu")) {
        return ctrl->menuPressed;
    } else if (ActionNameMatches(name, "primary") || ActionNameMatches(name, "/a/") || ActionNameMatches(name, "/x/")) {
        return ctrl->primaryPressed;
    } else if (ActionNameMatches(name, "secondary") || ActionNameMatches(name, "/b/") || ActionNameMatches(name, "/y/")) {
        return ctrl->secondaryPressed;
    } else if (ActionNameMatches(name, "thumbstick") || ActionNameMatches(name, "joystick")) {
        return ctrl->thumbstickPressed;
    } else if (ActionNameMatches(name, "trackpad/force")) {
        return XrVector2f_Length(&ctrl->thumbstick) > 0.5f;
    }
    recognized = false;
    return false;
}

// Unreal only raises an input event for a state flagged changedSinceLastSync, so each action's
// value is compared with the one it had at the previous xrSyncActions
static uint64_t g_syncCount = 0;
struct SyncedActionState { uint64_t sync = 0; XrVector2f previous{}, current{}; };
static std::map<std::pair<XrAction, XrPath>, SyncedActionState> g_syncedActionStates;

static XrBool32 ChangedSinceLastSync(const XrActionStateGetInfo* info, float x, float y = 0.0f) {
    SyncedActionState& s = g_syncedActionStates[{info->action, info->subactionPath}];
    if (s.sync != g_syncCount) {
        s.previous = s.current;
        s.current = {x, y};
        s.sync = g_syncCount;
    }
    return (s.current.x != s.previous.x || s.current.y != s.previous.y) ? XR_TRUE : XR_FALSE;
}

static XrResult XRAPI_PTR xrGetActionStateBoolean_runtime(XrSession, const XrActionStateGetInfo* info, XrActionStateBoolean* state) {
    if (!info || !state) return XR_ERROR_VALIDATION_FAILURE;
    state->type = XR_TYPE_ACTION_STATE_BOOLEAN;
    state->changedSinceLastSync = XR_FALSE;
    state->lastChangeTime = 0;

    if (TryTypedBoolean(info, state)) {
        state->changedSinceLastSync = ChangedSinceLastSync(info, state->currentState ? 1.0f : 0.0f);
        return XR_SUCCESS;
    }

    int hand = ResolveActionHand(info->action, info->subactionPath);
    bool buttonState = false;
    bool recognized = false;
    std::string name;

    if (hand == HAND_NONE) {
        // Action covers both hands (or has no usable binding): OR them together
        // instead of silently answering from one hand.
        bool recLeft = false, recRight = false;
        std::string nameLeft = GetActionNameForHand(info->action, HAND_LEFT);
        std::string nameRight = GetActionNameForHand(info->action, HAND_RIGHT);
        bool left = EvalActionBoolean(nameLeft, &rt::g_leftController, recLeft);
        bool right = EvalActionBoolean(nameRight, &rt::g_rightController, recRight);
        buttonState = left || right;
        recognized = recLeft || recRight;
        name = left ? nameLeft : nameRight;
    } else {
        name = GetActionNameForHand(info->action, hand);
        buttonState = EvalActionBoolean(name, ControllerForHand(hand), recognized);
    }

    if (!recognized && verboseLogging) Logf("[SimXR] xrGetActionStateBoolean Unknown action: %s", name.c_str());
    if (verboseLogging && buttonState) Logf("[SimXR] xrGetActionStateBoolean Button pressed: %s", name.c_str());
    state->currentState = buttonState ? XR_TRUE : XR_FALSE;
    state->changedSinceLastSync = ChangedSinceLastSync(info, buttonState ? 1.0f : 0.0f);
    state->isActive = name == "unbound" ? XR_FALSE : XR_TRUE; // No source on the queried hand
    return XR_SUCCESS;
}

static float EvalActionFloat(const std::string& name, rt::ControllerState* ctrl, bool& recognized) {
    recognized = true;
    float floatState = 0.0f;

    if (ActionNameMatches(name, "thumbstick/touch")) {
        floatState = (ctrl->thumbstickPressed || XrVector2f_Length(&ctrl->thumbstick) > 0.1f) ? 1.0f : 0.0f;
    } else if (ActionNameMatches(name, "/touch")) {
        floatState = 0.0f;
    } else if (ActionNameMatches(name, "trigger") || ActionNameMatches(name, "select") || ActionNameMatches(name, "fire")) {
        floatState = ctrl->triggerValue;
    } else if (ActionNameMatches(name, "grip") || ActionNameMatches(name, "squeeze") || ActionNameMatches(name, "grab") || ActionNameMatches(name, "invalid_shiftdashjump")) {
        floatState = ctrl->gripValue;
    } else if (ActionNameMatches(name, "menu")) {
        floatState = rt::g_leftController.menuPressed ? 1.0f : 0.0f;
    } else if (ActionNameMatches(name, "primary") || ActionNameMatches(name, "/a/") || ActionNameMatches(name, "/x/")) {
        floatState = ctrl->primaryPressed ? 1.0f : 0.0f;
    } else if (ActionNameMatches(name, "secondary") || ActionNameMatches(name, "/b/") || ActionNameMatches(name, "/y/")) {
        floatState = ctrl->secondaryPressed ? 1.0f : 0.0f;
    } else if (ActionNameMatches(name, "thumbstick/click")) {
        floatState = ctrl->thumbstickPressed ? 1.0f : 0.0f;
    } else if (ActionNameMatches(name, "invalid_snapturnleft")) {
        floatState = ctrl->thumbstick.x < -0.5f ? 1.0f : 0.0f;
    } else if (ActionNameMatches(name, "invalid_snapturnright")) {
        floatState = ctrl->thumbstick.x > 0.5f ? 1.0f : 0.0f;
    } else if (ActionNameMatches(name, "thumbstick/x") || ActionNameMatches(name, "invalid_turnright")) {
        floatState = ctrl->thumbstick.x;
    } else if (ActionNameMatches(name, "thumbstick/y") || ActionNameMatches(name, "invalid_teleport") || ActionNameMatches(name, "invalid_walkmantle")) {
        floatState = ctrl->thumbstick.y;
    } else if (ActionNameMatches(name, "invalid_turnleft")) {
        floatState = -ctrl->thumbstick.x;
    } else if (ActionNameMatches(name, "invalid_moveback")) {
        floatState = -ctrl->thumbstick.y;
    } else if (ActionNameMatches(name, "trackpad/force") || ActionNameMatches(name, "trackpad/click")) {
        floatState = XrVector2f_Length(&ctrl->thumbstick);
    } else {
        recognized = false;
    }

    return floatState;
}

static XrResult XRAPI_PTR xrGetActionStateFloat_runtime(XrSession, const XrActionStateGetInfo* info, XrActionStateFloat* state) {
    if (!info || !state) return XR_ERROR_VALIDATION_FAILURE;
    state->type = XR_TYPE_ACTION_STATE_FLOAT;
    state->changedSinceLastSync = XR_FALSE;
    state->lastChangeTime = 0;

    if (TryTypedFloat(info, state)) {
        state->changedSinceLastSync = ChangedSinceLastSync(info, state->currentState);
        return XR_SUCCESS;
    }

    int hand = ResolveActionHand(info->action, info->subactionPath);
    float floatState = 0.0f;
    bool recognized = false;
    std::string name;

    if (hand == HAND_NONE) {
        // Action covers both hands: take whichever is deflected further, so a
        // two-hand binding responds to the hand the player is actually using.
        bool recLeft = false, recRight = false;
        std::string nameLeft = GetActionNameForHand(info->action, HAND_LEFT);
        std::string nameRight = GetActionNameForHand(info->action, HAND_RIGHT);
        float left = EvalActionFloat(nameLeft, &rt::g_leftController, recLeft);
        float right = EvalActionFloat(nameRight, &rt::g_rightController, recRight);
        bool useLeft = fabsf(left) >= fabsf(right);
        floatState = useLeft ? left : right;
        recognized = recLeft || recRight;
        name = useLeft ? nameLeft : nameRight;
    } else {
        name = GetActionNameForHand(info->action, hand);
        floatState = EvalActionFloat(name, ControllerForHand(hand), recognized);
    }

    if (!recognized && verboseLogging) Logf("[SimXR] xrGetActionStateFloat Unknown action: %s", name.c_str());
    if (verboseLogging && (floatState > 0.25f)) Logf("[SimXR] xrGetActionStateFloat Button pressed: %s", name.c_str());
    state->currentState = floatState;
    state->changedSinceLastSync = ChangedSinceLastSync(info, floatState);
    state->isActive = name == "unbound" ? XR_FALSE : XR_TRUE; // No source on the queried hand
    return XR_SUCCESS;
}

static XrResult XRAPI_PTR xrGetActionStatePose_runtime(XrSession, const XrActionStateGetInfo* info, XrActionStatePose* state) {
    if (!info || !state) return XR_ERROR_VALIDATION_FAILURE;
    state->type = XR_TYPE_ACTION_STATE_POSE;
    // Inactive on a hand no binding covers; an action with no suggested bindings stays active as before
    int hand = ResolveActionHand(info->action, info->subactionPath);
    ActionComponent left = hand == HAND_RIGHT ? COMP_NONE : ResolveComponentForHand(info->action, HAND_LEFT);
    ActionComponent right = hand == HAND_LEFT ? COMP_NONE : ResolveComponentForHand(info->action, HAND_RIGHT);
    state->isActive = (left != COMP_NONE || right != COMP_NONE) ? XR_TRUE : XR_FALSE;
    return XR_SUCCESS;
}

static XrVector2f EvalActionVector2f(const std::string& name, rt::ControllerState* ctrl, bool& recognized) {
    recognized = true;
    XrVector2f vec2State = {0.0f, 0.0f};

    if (ActionNameMatches(name, "thumbstick") || ActionNameMatches(name, "trackpad") || ActionNameMatches(name, "invalid_scrolltouchpad")) {
        vec2State = ctrl->thumbstick;
    } else {
        recognized = false;
    }

    return vec2State;
}

static XrResult XRAPI_PTR xrGetActionStateVector2f_runtime(XrSession, const XrActionStateGetInfo* info, XrActionStateVector2f* state) {
    if (!info || !state) return XR_ERROR_VALIDATION_FAILURE;
    state->type = XR_TYPE_ACTION_STATE_VECTOR2F;
    state->changedSinceLastSync = XR_FALSE;
    state->lastChangeTime = 0;

    if (TryTypedVector2f(info, state)) {
        state->changedSinceLastSync = ChangedSinceLastSync(info, state->currentState.x, state->currentState.y);
        return XR_SUCCESS;
    }

    int hand = ResolveActionHand(info->action, info->subactionPath);
    XrVector2f vec2State = {0.0f, 0.0f};
    bool recognized = false;
    std::string name;

    if (hand == HAND_NONE) {
        // Locomotion actions bound on both hands land here: use whichever stick is
        // pushed further rather than always reading the right controller.
        bool recLeft = false, recRight = false;
        std::string nameLeft = GetActionNameForHand(info->action, HAND_LEFT);
        std::string nameRight = GetActionNameForHand(info->action, HAND_RIGHT);
        XrVector2f left = EvalActionVector2f(nameLeft, &rt::g_leftController, recLeft);
        XrVector2f right = EvalActionVector2f(nameRight, &rt::g_rightController, recRight);
        bool useLeft = XrVector2f_Length(&left) >= XrVector2f_Length(&right);
        vec2State = useLeft ? left : right;
        recognized = recLeft || recRight;
        name = useLeft ? nameLeft : nameRight;
    } else {
        name = GetActionNameForHand(info->action, hand);
        vec2State = EvalActionVector2f(name, ControllerForHand(hand), recognized);
    }

    if (!recognized && verboseLogging) Logf("[SimXR] xrGetActionStateVector2f Unknown action: %s", name.c_str());
    state->currentState = vec2State;
    state->changedSinceLastSync = ChangedSinceLastSync(info, vec2State.x, vec2State.y);
    state->isActive = name == "unbound" ? XR_FALSE : XR_TRUE; // No source on the queried hand
    return XR_SUCCESS;
}

static XrResult XRAPI_PTR xrSyncActions_runtime(XrSession, const XrActionsSyncInfo* info) {
    if (!info) return XR_ERROR_VALIDATION_FAILURE;

    rt::DrainPendingControllers();
    g_syncCount++;

    return XR_SUCCESS;
}

static XrResult XRAPI_PTR xrStringToPath_runtime(XrInstance, const char* pathString, XrPath* path) {
    if (!pathString || !path) return XR_ERROR_VALIDATION_FAILURE;
    // Simple hash as path ID
    size_t hash = 5381;
    for (const char* c = pathString; *c; ++c) {
        hash = ((hash << 5) + hash) + *c;
    }
    *path = (XrPath)hash;
    // Store path string for controller detection
    rt::g_pathStrings[*path] = pathString;
    Logf("[SimXR] xrStringToPath: %s -> %llu", pathString, (unsigned long long)*path);
    return XR_SUCCESS;
}

static XrResult XRAPI_PTR xrPathToString_runtime(XrInstance, XrPath path, uint32_t bufferCapacityInput, uint32_t* bufferCountOutput, char* buffer) {
    if (!bufferCountOutput) return XR_ERROR_VALIDATION_FAILURE;
    auto it = rt::g_pathStrings.find(path);
    if (it == rt::g_pathStrings.end()) return XR_ERROR_PATH_INVALID;
    // Two-call idiom: the count includes the terminator, and a zero capacity only asks for it
    *bufferCountOutput = (uint32_t)it->second.size() + 1;
    if (bufferCapacityInput == 0) return XR_SUCCESS;
    if (!buffer || bufferCapacityInput < *bufferCountOutput) return XR_ERROR_SIZE_INSUFFICIENT;
    memcpy(buffer, it->second.c_str(), *bufferCountOutput);

    if (verboseLogging) Logf("[SimXR] xrPathToString: %llu -> %s", (unsigned long long)path, buffer);
    return XR_SUCCESS;
}

static XrResult XRAPI_PTR xrGetCurrentInteractionProfile_runtime(XrSession, XrPath topLevelUserPath, XrInteractionProfileState* interactionProfile) {
    if (!interactionProfile) return XR_ERROR_VALIDATION_FAILURE;
    interactionProfile->type = XR_TYPE_INTERACTION_PROFILE_STATE;
    // Nothing is frozen until the action sets are attached, so before that rank whatever
    // has been suggested so far rather than reporting an arbitrary one
    interactionProfile->interactionProfile =
        rt::g_activeProfile != XR_NULL_PATH ? rt::g_activeProfile : SelectInteractionProfile();
    return XR_SUCCESS;
}

static XrResult XRAPI_PTR xrEnumerateBoundSourcesForAction_runtime(XrSession, const XrBoundSourcesForActionEnumerateInfo* info, uint32_t sourceCapacityInput, uint32_t* sourceCountOutput, XrPath* sources) {
    if (!info || !sourceCountOutput) return XR_ERROR_VALIDATION_FAILURE;
    // The attached profile's bindings, or every suggested one when it has none, since the input
    // lookups answer from other profiles too. Hand paths only: OpenComposite cuts each source
    // down to its /user/hand/<side> prefix and would crash on anything else.
    std::vector<XrPath> bound;
    auto bindIt = rt::g_actionBindings.find(info->action);
    for (int pass = 0; bindIt != rt::g_actionBindings.end() && pass < 2 && bound.empty(); ++pass) {
        for (const rt::ActionBinding& b : bindIt->second) {
            if (pass == 0 && b.profile != rt::g_activeProfile) continue;
            auto pathIt = rt::g_pathStrings.find(b.binding);
            if (pathIt == rt::g_pathStrings.end() || pathIt->second.rfind("/user/hand/", 0) != 0) continue;
            if (std::find(bound.begin(), bound.end(), b.binding) == bound.end()) bound.push_back(b.binding);
        }
    }
    *sourceCountOutput = (uint32_t)bound.size();
    if (sourceCapacityInput == 0 || !sources) return XR_SUCCESS;
    // Fill what fits rather than fail: OpenComposite aborts on any error from a fixed 20-entry array
    uint32_t n = std::min(sourceCapacityInput, (uint32_t)bound.size());
    std::copy(bound.begin(), bound.begin() + n, sources);
    *sourceCountOutput = n;
    return XR_SUCCESS;
}

static XrResult XRAPI_PTR xrGetInputSourceLocalizedName_runtime(XrSession, const XrInputSourceLocalizedNameGetInfo* info, uint32_t bufferCapacityInput, uint32_t* bufferCountOutput, char* buffer) {
    if (!info || !bufferCountOutput) return XR_ERROR_VALIDATION_FAILURE;
    auto pathIt = rt::g_pathStrings.find(info->sourcePath);
    if (pathIt == rt::g_pathStrings.end()) return XR_ERROR_PATH_INVALID;
    // Built from the source path, e.g. "Left Hand Index Controller Trigger", for games that show button prompts
    const std::string& path = pathIt->second;
    std::string name;
    auto append = [&](const std::string& part) {
        if (!part.empty()) name += (name.empty() ? "" : " ") + part;
    };
    if (info->whichComponents & XR_INPUT_SOURCE_LOCALIZED_NAME_USER_PATH_BIT) {
        int hand = HandFromPathString(path);
        append(hand == HAND_LEFT ? "Left Hand" : hand == HAND_RIGHT ? "Right Hand" : "");
    }
    if (info->whichComponents & XR_INPUT_SOURCE_LOCALIZED_NAME_INTERACTION_PROFILE_BIT) {
        auto profileIt = rt::g_pathStrings.find(rt::g_activeProfile);
        const std::string profile = profileIt != rt::g_pathStrings.end() ? profileIt->second : "";
        if (profile.find("/valve/index_controller") != std::string::npos) append("Index Controller");
        else if (profile.find("/oculus/touch_controller") != std::string::npos) append("Touch Controller");
        else if (profile.find("/microsoft/motion_controller") != std::string::npos) append("Mixed Reality Controller");
        else if (profile.find("/htc/vive_controller") != std::string::npos) append("Vive Controller");
        else append("Controller");
    }
    if (info->whichComponents & XR_INPUT_SOURCE_LOCALIZED_NAME_COMPONENT_BIT) {
        size_t inputPos = path.find("/input/");
        if (inputPos != std::string::npos) {
            std::string component = path.substr(inputPos + 7);
            component = component.substr(0, component.find('/'));
            for (char& c : component) if (c == '_') c = ' ';
            if (!component.empty()) component[0] = (char)toupper(component[0]);
            append(component);
        }
    }

    // Two-call idiom: the count includes the terminator, and a zero capacity only asks for it
    *bufferCountOutput = (uint32_t)name.size() + 1;
    if (bufferCapacityInput == 0) return XR_SUCCESS;
    if (!buffer || bufferCapacityInput < *bufferCountOutput) return XR_ERROR_SIZE_INSUFFICIENT;
    memcpy(buffer, name.c_str(), *bufferCountOutput);
    if (verboseLogging) Logf("[SimXR] xrGetInputSourceLocalizedName: %s -> %s", path.c_str(), buffer);
    return XR_SUCCESS;
}

static XrResult XRAPI_PTR xrDestroySwapchain_runtime(XrSwapchain sc) {
    auto it = rt::g_swapchains.find(sc);
    if (it == rt::g_swapchains.end()) return XR_ERROR_HANDLE_INVALID;

    // For OpenGL swapchains, delete the textures
    if (it->second.backend == rt::Swapchain::Backend::OpenGL && !it->second.imagesGL.empty()) {
        // Make app's GL context current if available
        HGLRC prevRC = wglGetCurrentContext();
        HDC prevDC = wglGetCurrentDC();
        if (rt::g_session.glDC && rt::g_session.glRC) {
            wglMakeCurrent(rt::g_session.glDC, rt::g_session.glRC);
        }
        for (GLuint tex : it->second.imagesGL) {
            glDeleteTextures(1, &tex);
        }
        if (prevRC) wglMakeCurrent(prevDC, prevRC);
    }
    // For Vulkan swapchains, destroy images and free memory
    if (it->second.backend == rt::Swapchain::Backend::Vulkan && rt::g_session.vkDevice != VK_NULL_HANDLE) {
        for (size_t i = 0; i < it->second.imagesVK.size(); ++i) {
            if (it->second.imagesVK[i] != VK_NULL_HANDLE)
                vkDestroyImage(rt::g_session.vkDevice, it->second.imagesVK[i], nullptr);
        }
        for (size_t i = 0; i < it->second.imageMemoriesVK.size(); ++i) {
            if (it->second.imageMemoriesVK[i] != VK_NULL_HANDLE)
                vkFreeMemory(rt::g_session.vkDevice, it->second.imageMemoriesVK[i], nullptr);
        }
    }

    rt::g_swapchains.erase(it);
    Logf("[SimXR] xrDestroySwapchain: sc=%p", sc);
    return XR_SUCCESS;
}

// Missing functions Unity needs
static XrResult XRAPI_PTR xrResultToString_runtime(XrInstance, XrResult value, char buffer[XR_MAX_RESULT_STRING_SIZE]) {
    // Every code by name, so a game's own log says what failed rather than a bare XR_ERROR
    switch (value) {
#define WXR_ENUM_NAME(name, val) case name: snprintf(buffer, XR_MAX_RESULT_STRING_SIZE, "%s", #name); return XR_SUCCESS;
        XR_LIST_ENUM_XrResult(WXR_ENUM_NAME)
#undef WXR_ENUM_NAME
        default: break;
    }
    snprintf(buffer, XR_MAX_RESULT_STRING_SIZE, value < 0 ? "XR_UNKNOWN_FAILURE_%d" : "XR_UNKNOWN_SUCCESS_%d", (int)value);
    return XR_SUCCESS;
}

static XrResult XRAPI_PTR xrStructureTypeToString_runtime(XrInstance, XrStructureType value, char buffer[XR_MAX_STRUCTURE_NAME_SIZE]) {
    switch (value) {
#define WXR_ENUM_NAME(name, val) case name: snprintf(buffer, XR_MAX_STRUCTURE_NAME_SIZE, "%s", #name); return XR_SUCCESS;
        XR_LIST_ENUM_XrStructureType(WXR_ENUM_NAME)
#undef WXR_ENUM_NAME
        default: break;
    }
    snprintf(buffer, XR_MAX_STRUCTURE_NAME_SIZE, "XR_UNKNOWN_STRUCTURE_TYPE_%d", (int)value);
    return XR_SUCCESS;
}

static XrResult XRAPI_PTR xrGetReferenceSpaceBoundsRect_runtime(XrSession, XrReferenceSpaceType, XrExtent2Df* bounds) {
    if (!bounds) return XR_ERROR_VALIDATION_FAILURE;
    bounds->width = 3.0f;
    bounds->height = 3.0f;
    Log("[SimXR] xrGetReferenceSpaceBoundsRect: 3x3 meters");
    return XR_SUCCESS;
}

static XrResult XRAPI_PTR xrGetViewConfigurationProperties_runtime(XrInstance, XrSystemId, XrViewConfigurationType type, 
                                                                   XrViewConfigurationProperties* props) {
    if (!props) return XR_ERROR_VALIDATION_FAILURE;
    props->type = XR_TYPE_VIEW_CONFIGURATION_PROPERTIES;
    props->viewConfigurationType = type;
    props->fovMutable = XR_FALSE;
    Log("[SimXR] xrGetViewConfigurationProperties");
    return XR_SUCCESS;
}

static XrResult XRAPI_PTR xrApplyHapticFeedback_runtime(XrSession, const XrHapticActionInfo* info, const XrHapticBaseHeader* haptic) {
    if (!info) return XR_ERROR_VALIDATION_FAILURE;
    // A zero-length or zero-strength pulse is how some games switch the motor off every
    // frame (XR_MIN_HAPTIC_DURATION is -1, so it still buzzes)
    if (haptic && haptic->type == XR_TYPE_HAPTIC_VIBRATION) {
        const XrHapticVibration* vibration = (const XrHapticVibration*)haptic;
        if (vibration->duration == 0 || vibration->amplitude <= 0.0f) return XR_SUCCESS;
    }
    if (udpReader && sendHaptics) {
        // A haptic action bound to both hands and fired without a subaction path
        // applies to both, so buzz both rather than guessing the right controller.
        switch (ResolveActionHand(info->action, info->subactionPath)) {
            case HAND_LEFT:  udpReader->SendData("1 0"); break;
            case HAND_RIGHT: udpReader->SendData("0 1"); break;
            default:         udpReader->SendData("1 1"); break;
        }
    }

    return XR_SUCCESS;
}

static XrResult XRAPI_PTR xrStopHapticFeedback_runtime(XrSession, const XrHapticActionInfo* info) {
    return XR_SUCCESS;
}

// Time conversion functions for XR_KHR_win32_convert_performance_counter_time
static XrResult XRAPI_PTR xrConvertWin32PerformanceCounterToTimeKHR_runtime(XrInstance instance,
                                                                            const LARGE_INTEGER* performanceCounter,
                                                                            XrTime* time) {
    if (!performanceCounter || !time) return XR_ERROR_VALIDATION_FAILURE;
    
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    
    // Convert to nanoseconds
    *time = (performanceCounter->QuadPart * 1000000000) / freq.QuadPart;
    return XR_SUCCESS;
}

static XrResult XRAPI_PTR xrConvertTimeToWin32PerformanceCounterKHR_runtime(XrInstance instance,
                                                                             XrTime time,
                                                                             LARGE_INTEGER* performanceCounter) {
    if (!performanceCounter) return XR_ERROR_VALIDATION_FAILURE;
    
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    
    // Convert from nanoseconds  
    performanceCounter->QuadPart = (time * freq.QuadPart) / 1000000000;
    return XR_SUCCESS;
}

// ----------------------------------------------

struct NameFn { const char* name; PFN_xrVoidFunction fn; };

static const NameFn kFnTable[] = {
    {"xrGetInstanceProcAddr", (PFN_xrVoidFunction)xrGetInstanceProcAddr_runtime},
    {"xrEnumerateApiLayerProperties", (PFN_xrVoidFunction)xrEnumerateApiLayerProperties_runtime},
    {"xrEnumerateInstanceExtensionProperties", (PFN_xrVoidFunction)xrEnumerateInstanceExtensionProperties_runtime},
    {"xrCreateInstance", (PFN_xrVoidFunction)xrCreateInstance_runtime},
    {"xrDestroyInstance", (PFN_xrVoidFunction)xrDestroyInstance_runtime},
    {"xrGetInstanceProperties", (PFN_xrVoidFunction)xrGetInstanceProperties_runtime},
    {"xrGetSystem", (PFN_xrVoidFunction)xrGetSystem_runtime},
    {"xrGetSystemProperties", (PFN_xrVoidFunction)xrGetSystemProperties_runtime},
    {"xrEnumerateViewConfigurations", (PFN_xrVoidFunction)xrEnumerateViewConfigurations_runtime},
    {"xrEnumerateViewConfigurationViews", (PFN_xrVoidFunction)xrEnumerateViewConfigurationViews_runtime},
    {"xrEnumerateEnvironmentBlendModes", (PFN_xrVoidFunction)xrEnumerateEnvironmentBlendModes_runtime},
    {"xrCreateSession", (PFN_xrVoidFunction)xrCreateSession_runtime},
    {"xrDestroySession", (PFN_xrVoidFunction)xrDestroySession_runtime},
    {"xrEnumerateSwapchainFormats", (PFN_xrVoidFunction)xrEnumerateSwapchainFormats_runtime},
    {"xrCreateSwapchain", (PFN_xrVoidFunction)xrCreateSwapchain_runtime},
    {"xrDestroySwapchain", (PFN_xrVoidFunction)xrDestroySwapchain_runtime},
    {"xrEnumerateSwapchainImages", (PFN_xrVoidFunction)xrEnumerateSwapchainImages_runtime},
    {"xrAcquireSwapchainImage", (PFN_xrVoidFunction)xrAcquireSwapchainImage_runtime},
    {"xrWaitSwapchainImage", (PFN_xrVoidFunction)xrWaitSwapchainImage_runtime},
    {"xrReleaseSwapchainImage", (PFN_xrVoidFunction)xrReleaseSwapchainImage_runtime},
    {"xrBeginSession", (PFN_xrVoidFunction)xrBeginSession_runtime},
    {"xrEndSession", (PFN_xrVoidFunction)xrEndSession_runtime},
    {"xrWaitFrame", (PFN_xrVoidFunction)xrWaitFrame_runtime},
    {"xrBeginFrame", (PFN_xrVoidFunction)xrBeginFrame_runtime},
    {"xrEndFrame", (PFN_xrVoidFunction)xrEndFrame_runtime},
    {"xrPollEvent", (PFN_xrVoidFunction)xrPollEvent_runtime},
    {"xrLocateViews", (PFN_xrVoidFunction)xrLocateViews_runtime},
    {"xrGetD3D11GraphicsRequirementsKHR", (PFN_xrVoidFunction)xrGetD3D11GraphicsRequirementsKHR_runtime},
    {"xrGetD3D12GraphicsRequirementsKHR", (PFN_xrVoidFunction)xrGetD3D12GraphicsRequirementsKHR_runtime},
    {"xrGetOpenGLGraphicsRequirementsKHR", (PFN_xrVoidFunction)xrGetOpenGLGraphicsRequirementsKHR_runtime},
    {"xrRequestExitSession", (PFN_xrVoidFunction)xrRequestExitSession_runtime},
    // Space functions
    {"xrCreateReferenceSpace", (PFN_xrVoidFunction)xrCreateReferenceSpace_runtime},
    {"xrDestroySpace", (PFN_xrVoidFunction)xrDestroySpace_runtime},
    {"xrLocateSpace", (PFN_xrVoidFunction)xrLocateSpace_runtime},
    {"xrLocateSpaces", (PFN_xrVoidFunction)xrLocateSpaces_runtime},
    {"xrGetVisibilityMaskKHR", (PFN_xrVoidFunction)xrGetVisibilityMaskKHR_runtime},
    {"xrEnumerateReferenceSpaces", (PFN_xrVoidFunction)xrEnumerateReferenceSpaces_runtime},
    {"xrCreateActionSpace", (PFN_xrVoidFunction)xrCreateActionSpace_runtime},
    // Action functions
    {"xrCreateActionSet", (PFN_xrVoidFunction)xrCreateActionSet_runtime},
    {"xrDestroyActionSet", (PFN_xrVoidFunction)xrDestroyActionSet_runtime},
    {"xrCreateAction", (PFN_xrVoidFunction)xrCreateAction_runtime},
    {"xrDestroyAction", (PFN_xrVoidFunction)xrDestroyAction_runtime},
    {"xrSuggestInteractionProfileBindings", (PFN_xrVoidFunction)xrSuggestInteractionProfileBindings_runtime},
    {"xrAttachSessionActionSets", (PFN_xrVoidFunction)xrAttachSessionActionSets_runtime},
    {"xrGetActionStateBoolean", (PFN_xrVoidFunction)xrGetActionStateBoolean_runtime},
    {"xrGetActionStateFloat", (PFN_xrVoidFunction)xrGetActionStateFloat_runtime},
    {"xrGetActionStatePose", (PFN_xrVoidFunction)xrGetActionStatePose_runtime},
    {"xrGetActionStateVector2f", (PFN_xrVoidFunction)xrGetActionStateVector2f_runtime},
    {"xrSyncActions", (PFN_xrVoidFunction)xrSyncActions_runtime},
    // Path functions
    {"xrStringToPath", (PFN_xrVoidFunction)xrStringToPath_runtime},
    {"xrPathToString", (PFN_xrVoidFunction)xrPathToString_runtime},
    // Interaction functions
    {"xrGetCurrentInteractionProfile", (PFN_xrVoidFunction)xrGetCurrentInteractionProfile_runtime},
    {"xrEnumerateBoundSourcesForAction", (PFN_xrVoidFunction)xrEnumerateBoundSourcesForAction_runtime},
    {"xrGetInputSourceLocalizedName", (PFN_xrVoidFunction)xrGetInputSourceLocalizedName_runtime},
    // Utility functions
    {"xrResultToString", (PFN_xrVoidFunction)xrResultToString_runtime},
    {"xrStructureTypeToString", (PFN_xrVoidFunction)xrStructureTypeToString_runtime},
    {"xrGetReferenceSpaceBoundsRect", (PFN_xrVoidFunction)xrGetReferenceSpaceBoundsRect_runtime},
    {"xrGetViewConfigurationProperties", (PFN_xrVoidFunction)xrGetViewConfigurationProperties_runtime},
    // Haptic functions
    {"xrApplyHapticFeedback", (PFN_xrVoidFunction)xrApplyHapticFeedback_runtime},
    {"xrStopHapticFeedback", (PFN_xrVoidFunction)xrStopHapticFeedback_runtime},
    // Time conversion functions
    {"xrConvertWin32PerformanceCounterToTimeKHR", (PFN_xrVoidFunction)xrConvertWin32PerformanceCounterToTimeKHR_runtime},
    {"xrConvertTimeToWin32PerformanceCounterKHR", (PFN_xrVoidFunction)xrConvertTimeToWin32PerformanceCounterKHR_runtime},
    // Vulkan extension functions
    {"xrGetVulkanGraphicsRequirementsKHR",   (PFN_xrVoidFunction)xrGetVulkanGraphicsRequirementsKHR_runtime},
    {"xrGetVulkanGraphicsRequirements2KHR",  (PFN_xrVoidFunction)xrGetVulkanGraphicsRequirements2KHR_runtime},
    {"xrGetVulkanInstanceExtensionsKHR",     (PFN_xrVoidFunction)xrGetVulkanInstanceExtensionsKHR_runtime},
    {"xrGetVulkanDeviceExtensionsKHR",       (PFN_xrVoidFunction)xrGetVulkanDeviceExtensionsKHR_runtime},
    {"xrGetVulkanGraphicsDeviceKHR",         (PFN_xrVoidFunction)xrGetVulkanGraphicsDeviceKHR_runtime},
    {"xrCreateVulkanInstanceKHR",            (PFN_xrVoidFunction)xrCreateVulkanInstanceKHR_runtime},
    {"xrCreateVulkanDeviceKHR",              (PFN_xrVoidFunction)xrCreateVulkanDeviceKHR_runtime},
    {"xrGetVulkanGraphicsDevice2KHR",        (PFN_xrVoidFunction)xrGetVulkanGraphicsDevice2KHR_runtime},
};

static XrResult XRAPI_PTR xrGetInstanceProcAddr_runtime(XrInstance instance, const char* name, PFN_xrVoidFunction* fn) {
    if (!name || !fn) {
        Logf("[SimXR] xrGetInstanceProcAddr: ERROR - name=%p, fn=%p", name, fn);
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    // Reduce logging verbosity for xrGetInstanceProcAddr
    static bool reduceLogging = false;
    static int callCount = 0;
    callCount++;
    
    for (auto& e : kFnTable) {
        if (strcmp(name, e.name) == 0) { 
            *fn = e.fn;
            if (callCount < 100 || strstr(name, "D3D11") || strstr(name, "Create") || strstr(name, "Destroy")) {
                Logf("[SimXR] xrGetInstanceProcAddr: %s -> FOUND", name);
            }
            return XR_SUCCESS; 
        }
    }
    
    if (callCount < 100 || strstr(name, "D3D11")) {
        Logf("[SimXR] xrGetInstanceProcAddr: %s -> NOT FOUND", name);
    }
    return XR_ERROR_FUNCTION_UNSUPPORTED;
}
