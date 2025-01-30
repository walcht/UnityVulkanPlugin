#include <assert.h>
#include <math.h>

#include <unordered_map>

#include "PlatformBase.hpp"
#include "TextureSubPluginAPI.hpp"
#include "Unity/IUnityLog.h"

enum Event {
  TextureSubImage2D = 0,
  TextureSubImage3D = 1,
  CreateTexture3D = 2,
  ClearTexture3D = 3
};

// global state
static TextureSubPluginAPI* s_CurrentAPI = NULL;
static UnityGfxRenderer s_DeviceType = kUnityGfxRendererNull;
static std::unordered_map<uint32_t, void*> g_CreatedTextures;

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
  // clear created textures
  for (auto it = g_CreatedTextures.begin(); it != g_CreatedTextures.end();) {
    // this is not run in render thread => might crash?
    s_CurrentAPI->ClearTexture3D(it->second);
  }
  g_Graphics->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);
}

static void UNITY_INTERFACE_API
OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType) {
  // Create graphics API implementation upon initialization
  if (eventType == kUnityGfxDeviceEventInitialize) {
    assert(s_CurrentAPI == NULL);
    s_DeviceType = g_Graphics->GetRenderer();
    s_CurrentAPI = CreateTextureSubPluginAPI(s_DeviceType);
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
  uint32_t texture_id;
  uint32_t width;
  uint32_t height;
  uint32_t depth;
  Format format;
};

struct ClearTexture3DParams {
  uint32_t texture_id;
};

static void UNITY_INTERFACE_API OnRenderEvent(int eventID, void* data) {
  // Unknown / unsupported graphics device type? Do nothing
  if (s_CurrentAPI == NULL) return;

  switch ((Event)eventID) {
    case Event::TextureSubImage2D: {
      auto args = static_cast<TextureSubImage2DParams*>(data);
      s_CurrentAPI->TextureSubImage2D(
          args->texture_handle, args->xoffset, args->yoffset, args->width,
          args->height, args->data_ptr, args->level, args->format);
      break;
    }
    case Event::TextureSubImage3D: {
      auto args = static_cast<TextureSubImage3DParams*>(data);
      s_CurrentAPI->TextureSubImage3D(args->texture_handle, args->xoffset,
                                      args->yoffset, args->zoffset, args->width,
                                      args->height, args->depth, args->data_ptr,
                                      args->level, args->format);
      break;
    }
    case Event::CreateTexture3D: {
      auto args = static_cast<CreateTexture3DParams*>(data);
      if (auto search = g_CreatedTextures.find(args->texture_id);
          search != g_CreatedTextures.end()) {
        UNITY_LOG_ERROR(
            g_Log, "a texture with the provided texture ID already exists!");
        return;
      }
      g_CreatedTextures[args->texture_id] = nullptr;
      s_CurrentAPI->CreateTexture3D(args->width, args->height, args->depth,
                                    args->format,
                                    g_CreatedTextures[args->texture_id]);
      break;
    }
    case Event::ClearTexture3D: {
      auto args = static_cast<ClearTexture3DParams*>(data);
      if (auto search = g_CreatedTextures.find(args->texture_id);
          search != g_CreatedTextures.end()) {
        s_CurrentAPI->ClearTexture3D(search->second);
        g_CreatedTextures.erase(search);
        return;
      }
      UNITY_LOG_ERROR(g_Log,
                      "no texture was created with the provided texture ID");
      break;
    }
    default:
      break;
  }
}

extern "C" UnityRenderingEventAndData UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
GetRenderEventFunc() {
  return OnRenderEvent;
}

extern "C" UNITY_INTERFACE_EXPORT void* UNITY_INTERFACE_API
RetrieveCreatedTexture3D(uint32_t texture_id) {
  if (auto search = g_CreatedTextures.find(texture_id);
      search != g_CreatedTextures.end()) {
    return search->second;
  }
  UNITY_LOG_ERROR(g_Log, "no texture was created with the provided texture ID");
  return nullptr;
}
