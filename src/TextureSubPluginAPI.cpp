#include "TextureSubPluginAPI.hpp"

#include "PlatformBase.hpp"
#include "Unity/IUnityGraphics.h"

IUnityInterfaces* g_UnityInterfaces = NULL;
IUnityGraphics* g_Graphics = NULL;
IUnityLog* g_Log = NULL;

TextureSubPluginAPI* CreateTextureSubPluginAPI(UnityGfxRenderer apiType) {

#if SUPPORT_D3D11
  if (apiType == kUnityGfxRendererD3D11) {
    extern TextureSubPluginAPI* CreateTextureSubPluginAPI_D3D11();
    return CreateTextureSubPluginAPI_D3D11();
  }
#endif  // if SUPPORT_D3D11

#if SUPPORT_OPENGL_UNIFIED
  if (apiType == kUnityGfxRendererOpenGLCore ||
      apiType == kUnityGfxRendererOpenGLES30) {
    extern TextureSubPluginAPI* CreateTextureSubPluginAPI_OpenGLCoreES(
        UnityGfxRenderer apiType);
    return CreateTextureSubPluginAPI_OpenGLCoreES(apiType);
  }
#endif  // if SUPPORT_OPENGL_UNIFIED

#if SUPPORT_VULKAN
  if (apiType == kUnityGfxRendererVulkan) {
    extern TextureSubPluginAPI* CreateTextureSubPluginAPI_Vulkan();
    return CreateTextureSubPluginAPI_Vulkan();
  }
#endif  // if SUPPORT_VULKAN

  // Unknown or unsupported graphics API
  return NULL;
}
