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

  virtual void CreateTexture3D(uint32_t width, uint32_t height, uint32_t depth,
                               Format format, void*& texture) = 0;

  virtual void ClearTexture3D(void* texture_handle) = 0;

  /// @brief Loads provided data (i.e., sub-region of texture 2D) into a
  /// provided texture2D's GPU memory. For OpenGL this is simply a call to
  /// glTextureSubImage2D.
  /// @param texture_handle pointer to the texture handle
  /// @param xoffset x offset within the target 3D texture
  /// @param yoffset y offset within the target 3D texture
  /// @param width width of the source data
  /// @param height height of the source data
  /// @param data_ptr pointer to the data array in memory
  /// @param level will be provided to glTextureSubImage2D's level parameter.
  /// Only used in case graphics API is OpenGL.
  /// @param format will be provided to glTextureSubImage2D's format parameter.
  /// Only used in case graphics API is OpenGL.
  virtual void TextureSubImage2D(void* texture_handle, int32_t xoffset,
                                 int32_t yoffset, int32_t width, int32_t height,
                                 void* data_ptr, int32_t level,
                                 Format format) = 0;

  /// @brief Loads provided data (i.e., sub-region of texture 2D) into a
  /// provided texture3D's GPU memory. For OpenGL this is simply a call to
  /// glTextureSubImage3D.
  /// @param texture_handle pointer to the texture handle
  /// @param xoffset x offset within the target 3D texture
  /// @param yoffset y offset within the target 3D texture
  /// @param zoffset z offset within the target 3D texture
  /// @param width width of the source data
  /// @param height height of the source data
  /// @param depth depth of the source data
  /// @param data_ptr pointer to the data array in memory
  /// @param level will be provided to glTextureSubImage2D's level parameter.
  /// Only used in case graphics API is OpenGL.
  /// @param format will be provided to glTextureSubImage2D's format parameter.
  /// Only used in case graphics API is OpenGL.
  virtual void TextureSubImage3D(void* texture_handle, int32_t xoffset,
                                 int32_t yoffset, int32_t zoffset,
                                 int32_t width, int32_t height, int32_t depth,
                                 void* data_ptr, int32_t level,
                                 Format format) = 0;

  /// @brief to process general events like initialization,	shutdown, device
  /// loss/reset etc.
  /// @param type
  /// @param interfaces
  virtual void ProcessDeviceEvent(UnityGfxDeviceEventType type,
                                  IUnityInterfaces* interfaces) = 0;
};

/// @brief Create a graphics API implementation instance for the given API type.
/// @param apiType Graphics API type. Currently only: kUnityGfxRendererD3D11 and
/// kUnityGfxRendererOpenGLCore are supported
/// @return to the created render API
TextureSubPluginAPI* CreateTextureSubPluginAPI(UnityGfxRenderer apiType);
