#pragma once

#include <stddef.h>
#include <stdint.h>

#include "Unity/IUnityGraphics.h"
#include "Unity/IUnityLog.h"

struct IUnityInterfaces;

enum Format { R8_UINT = 0, R16_UINT = 1 };

extern IUnityInterfaces* g_UnityInterfaces;
extern IUnityGraphics* g_Graphics;
extern IUnityLog* g_Log;

class TextureSubPluginAPI {
 public:
  virtual ~TextureSubPluginAPI() {}

  /// @brief Creates a 3D texture and returns the texture handle in the texture
  /// out parameter
  /// @param[in] texture_id assigned unique texture ID. This can be used later
  /// to retrieve the created texture or destroy it.
  /// @param[in] width 3D texture width. Check with used graphics API 3D texture
  /// dimensions limitations before usage
  /// @param[in] height 3D texture height. Check with used graphics API 3D
  /// texture dimensions limitations before usage
  /// @param[in] depth 3D texture depth. Check with used graphics API 3D texture
  /// dimensions limitations before usage
  /// @param[in] format 3D texture format
  virtual void CreateTexture3D(uint32_t texture_id, uint32_t width,
                               uint32_t height, uint32_t depth,
                               Format format) = 0;

  /// @brief Retrieves the handle of a 3D texture that was created using
  /// CreateTexture3D. This function can be called outside of the render thread
  /// @param[in] texture_id the user assigned unique ID of the texture in the
  /// CreateTexture3D call
  virtual void* RetrieveCreatedTexture3D(uint32_t texture_id) = 0;

  /// @brief Destroys/releases a given 3D texture
  /// @param[in] texture_id 3D texture's ID. This id is assigned by the caller
  /// to CreateTexture3D and is used to fetch a created texture.
  virtual void DestroyTexture3D(uint32_t texture_id) = 0;

  /// @brief Updates a sub-region of a provided 2D texture
  /// @param[in] texture_handle pointer to the texture handle
  /// @param[in] xoffset x offset within the target 3D texture
  /// @param[in] yoffset y offset within the target 3D texture
  /// @param[in] width width of the source region
  /// @param[in] height height of the source region
  /// @param[in] data_ptr pointer to the data array in memory
  /// @param[in] level mipmap level
  /// @param[in] format texture format
  virtual void TextureSubImage2D(void* texture_handle, int32_t xoffset,
                                 int32_t yoffset, int32_t width, int32_t height,
                                 void* data_ptr, int32_t level,
                                 Format format) = 0;

  /// @brief Updates a sub-region of a provided 3D texture
  /// @param[in] texture_handle pointer to the texture handle
  /// @param[in] xoffset x offset within the target 3D texture
  /// @param[in] yoffset y offset within the target 3D texture
  /// @param[in] zoffset z offset within the target 3D texture
  /// @param[in] width width of the source region
  /// @param[in] height height of the source region
  /// @param[in] depth depth of the source region
  /// @param[in] data_ptr pointer to the data array in memory
  /// @param[in] level mipmap level
  /// @param[in] format texture format
  virtual void TextureSubImage3D(void* texture_handle, int32_t xoffset,
                                 int32_t yoffset, int32_t zoffset,
                                 int32_t width, int32_t height, int32_t depth,
                                 void* data_ptr, int32_t level,
                                 Format format) = 0;

  /// @brief Processes general events like initialization, shutdown, device
  /// loss/reset etc.
  /// @param[in] type event type
  /// @param[in] interfaces
  virtual void ProcessDeviceEvent(UnityGfxDeviceEventType type,
                                  IUnityInterfaces* interfaces) = 0;
};

/// @brief Create a graphics API implementation instance for the given API type.
/// @param[in] apiType graphics API type
/// @return pointer to the corresponding graphics API TextureSubPlugin API
TextureSubPluginAPI* CreateTextureSubPluginAPI(UnityGfxRenderer apiType);
