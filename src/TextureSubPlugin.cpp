#include <assert.h>
#include <math.h>

#include "PlatformBase.hpp"
#include "TextureSubPluginAPI.hpp"

enum Event {
  TextureSubImage2D = 0,
  TextureSubImage3D = 1,
  CreateTexture3D = 2,
  ClearTexture3D = 3
};

static void UNITY_INTERFACE_API
OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType);

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
UnityPluginLoad(IUnityInterfaces* unityInterfaces) {
  g_UnityInterfaces = unityInterfaces;
  g_Graphics = g_UnityInterfaces->Get<IUnityGraphics>();
  g_Graphics->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);
  g_Log = g_UnityInterfaces->Get<IUnityLog>();

  // Run OnGraphicsDeviceEvent(initialize) manually on plugin load
  OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload() {
  g_Graphics->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);
}

// GraphicsDeviceEvent
static TextureSubPluginAPI* s_CurrentAPI = NULL;
static UnityGfxRenderer s_DeviceType = kUnityGfxRendererNull;

static void UNITY_INTERFACE_API
OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType) {
  // Create graphics API implementation upon initialization
  if (eventType == kUnityGfxDeviceEventInitialize) {
    assert(s_CurrentAPI == NULL);
    s_DeviceType = g_Graphics->GetRenderer();
    s_CurrentAPI = CreateRenderAPI(s_DeviceType);
  }

  // Let the implementation process the device related events
  if (s_CurrentAPI) {
    s_CurrentAPI->ProcessDeviceEvent(eventType, g_UnityInterfaces);
  }

  // Cleanup graphics API implementation upon shutdown
  if (eventType == kUnityGfxDeviceEventShutdown) {
    delete s_CurrentAPI;
    s_CurrentAPI = NULL;
    s_DeviceType = kUnityGfxRendererNull;
  }
}

struct TextureSubImage2DParams {
  void* texture_handle;
  int32_t xoffset;
  int32_t yoffset;
  int32_t width;
  int32_t height;
  void* data_ptr;
  int32_t level;
  Format format;
};

struct TextureSubImage3DParams {
  void* texture_handle;
  int32_t xoffset;
  int32_t yoffset;
  int32_t zoffset;
  int32_t width;
  int32_t height;
  int32_t depth;
  void* data_ptr;
  int32_t level;
  Format format;
};

struct CreateTexture3DParams {
  uint32_t width;
  uint32_t height;
  uint32_t depth;
  Format format;
};

struct ClearTexture3DParams {
  void* texture_handle;
};

// global state parameters
static TextureSubImage2DParams g_TextureSubImage2DParams;
static TextureSubImage3DParams g_TextureSubImage3DParams;
static CreateTexture3DParams g_CreateTexture3DParams;
static ClearTexture3DParams g_ClearTexture3DParams;
static void* g_Texture3D = NULL;

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
UpdateTextureSubImage2DParams(void* texture_handle, int32_t xoffset,
                              int32_t yoffset, int32_t width, int32_t height,
                              void* data_ptr, int32_t level, Format format) {
  g_TextureSubImage2DParams.texture_handle = texture_handle;
  g_TextureSubImage2DParams.xoffset = xoffset;
  g_TextureSubImage2DParams.yoffset = yoffset;
  g_TextureSubImage2DParams.width = width;
  g_TextureSubImage2DParams.height = height;
  g_TextureSubImage2DParams.data_ptr = data_ptr;
  g_TextureSubImage2DParams.level = level;
  g_TextureSubImage2DParams.format = format;
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
UpdateTextureSubImage3DParams(void* texture_handle, int32_t xoffset,
                              int32_t yoffset, int32_t zoffset, int32_t width,
                              int32_t height, int32_t depth, void* data_ptr,
                              int32_t level, Format format) {
  g_TextureSubImage3DParams.texture_handle = texture_handle;
  g_TextureSubImage3DParams.xoffset = xoffset;
  g_TextureSubImage3DParams.yoffset = yoffset;
  g_TextureSubImage3DParams.zoffset = zoffset;
  g_TextureSubImage3DParams.width = width;
  g_TextureSubImage3DParams.height = height;
  g_TextureSubImage3DParams.depth = depth;
  g_TextureSubImage3DParams.data_ptr = data_ptr;
  g_TextureSubImage3DParams.level = level;
  g_TextureSubImage3DParams.format = format;
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
UpdateCreateTexture3DParams(uint32_t width, uint32_t height, uint32_t depth,
                            Format format) {
  g_CreateTexture3DParams.width = width;
  g_CreateTexture3DParams.height = height;
  g_CreateTexture3DParams.depth = depth;
  g_CreateTexture3DParams.format = format;
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
UpdateClearTexture3DParams(void* texture_handle) {
  g_ClearTexture3DParams.texture_handle = texture_handle;
}

static void UNITY_INTERFACE_API OnRenderEvent(int eventID) {
  // Unknown / unsupported graphics device type? Do nothing
  if (s_CurrentAPI == NULL) return;

  switch ((Event)eventID) {
    case Event::TextureSubImage2D: {
      s_CurrentAPI->TextureSubImage2D(
          g_TextureSubImage2DParams.texture_handle,
          g_TextureSubImage2DParams.xoffset, g_TextureSubImage2DParams.yoffset,
          g_TextureSubImage2DParams.width, g_TextureSubImage2DParams.height,
          g_TextureSubImage2DParams.data_ptr, g_TextureSubImage2DParams.level,
          g_TextureSubImage2DParams.format);
      break;
    }
    case Event::TextureSubImage3D: {
      s_CurrentAPI->TextureSubImage3D(
          g_TextureSubImage3DParams.texture_handle,
          g_TextureSubImage3DParams.xoffset, g_TextureSubImage3DParams.yoffset,
          g_TextureSubImage3DParams.zoffset, g_TextureSubImage3DParams.width,
          g_TextureSubImage3DParams.height, g_TextureSubImage3DParams.depth,
          g_TextureSubImage3DParams.data_ptr, g_TextureSubImage3DParams.level,
          g_TextureSubImage3DParams.format);
      break;
    }
    case Event::CreateTexture3D: {
      s_CurrentAPI->CreateTexture3D(
          g_CreateTexture3DParams.width, g_CreateTexture3DParams.height,
          g_CreateTexture3DParams.depth, g_CreateTexture3DParams.format,
          g_Texture3D);
      break;
    }
    case Event::ClearTexture3D: {
      s_CurrentAPI->ClearTexture3D(g_ClearTexture3DParams.texture_handle);
      break;
    }
    default:
      break;
  }
}

extern "C" UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
GetRenderEventFunc() {
  return OnRenderEvent;
}

extern "C" UNITY_INTERFACE_EXPORT void* UNITY_INTERFACE_API
RetrieveCreatedTexture3D() {
  return g_Texture3D;
}
