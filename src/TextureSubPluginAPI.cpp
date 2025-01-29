#include "PlatformBase.hpp"
#include "TextureSubPluginAPI.hpp"
#include "Unity/IUnityGraphics.h"

IUnityInterfaces* g_UnityInterfaces = NULL;
IUnityGraphics* g_Graphics = NULL;
IUnityLog* g_Log = NULL;

TextureSubPluginAPI* CreateRenderAPI(UnityGfxRenderer apiType) {
#if SUPPORT_D3D11
  if (apiType == kUnityGfxRendererD3D11) {
    extern TextureSubPluginAPI* CreateRenderAPI_D3D11();
    return CreateRenderAPI_D3D11();
  }
#endif  // if SUPPORT_D3D11

#if SUPPORT_D3D12
  if (apiType == kUnityGfxRendererD3D12) {
    extern TextureSubPluginAPI* CreateRenderAPI_D3D12();
    return CreateRenderAPI_D3D12();
  }
#endif  // if SUPPORT_D3D12

#if SUPPORT_OPENGL_UNIFIED
  if (apiType == kUnityGfxRendererOpenGLCore ||
      apiType == kUnityGfxRendererOpenGLES30) {
    extern TextureSubPluginAPI* CreateRenderAPI_OpenGLCoreES(UnityGfxRenderer apiType);
    return CreateRenderAPI_OpenGLCoreES(apiType);
  }
#endif  // if SUPPORT_OPENGL_UNIFIED

#if SUPPORT_VULKAN
  if (apiType == kUnityGfxRendererVulkan) {
    extern TextureSubPluginAPI* CreateRenderAPI_Vulkan();
    return CreateRenderAPI_Vulkan();
  }
#endif  // if SUPPORT_VULKAN

  // Unknown or unsupported graphics API
  return NULL;
}
