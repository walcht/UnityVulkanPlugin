#include "PlatformBase.hpp"
#include "TextureSubPluginAPI.hpp"

#if SUPPORT_D3D11

#include <assert.h>
#include <d3d11.h>

#include <sstream>
#include <string>

#include "Unity/IUnityGraphicsD3D11.h"
#include "Unity/IUnityLog.h"

class TextureSubPluginAPI_D3D11 : public TextureSubPluginAPI {
 public:
  TextureSubPluginAPI_D3D11();
  virtual ~TextureSubPluginAPI_D3D11() {}

  virtual void ProcessDeviceEvent(UnityGfxDeviceEventType type,
                                  IUnityInterfaces* interfaces);

  virtual void CreateTexture3D(uint32_t width, uint32_t height, uint32_t depth,
                               Format format, void*& texture);

  virtual void ClearTexture3D(void* texture_handle);

  virtual void TextureSubImage2D(void* texture_handle, int32_t xoffset,
                                 int32_t yoffset, int32_t width, int32_t height,
                                 void* data_ptr, int32_t level, Format format);

  virtual void TextureSubImage3D(void* texture_handle, int32_t xoffset,
                                 int32_t yoffset, int32_t zoffset,
                                 int32_t width, int32_t height, int32_t depth,
                                 void* data_ptr, int32_t level, Format format);

 private:
  ID3D11Device* m_Device;
};

TextureSubPluginAPI* CreateTextureSubPluginAPI_D3D11() {
  return new TextureSubPluginAPI_D3D11();
}

TextureSubPluginAPI_D3D11::TextureSubPluginAPI_D3D11() : m_Device(NULL) {}

void TextureSubPluginAPI_D3D11::ProcessDeviceEvent(
    UnityGfxDeviceEventType type, IUnityInterfaces* interfaces) {
  switch (type) {
    case kUnityGfxDeviceEventInitialize: {
      IUnityGraphicsD3D11* d3d = interfaces->Get<IUnityGraphicsD3D11>();
      m_Device = d3d->GetDevice();
      break;
    }
  }
}

void TextureSubPluginAPI_D3D11::TextureSubImage2D(
    void* texture_handle, int32_t xoffset, int32_t yoffset, int32_t width,
    int32_t height, void* data_ptr, int32_t level, Format format) {
  // determine row pitch/depth from provided format
  uint32_t row_pitch;
  switch (format) {
    case Format::R8_UINT:
      row_pitch = width;  // width * sizeof(uint8_t)
      break;
    case Format::R16_UINT:
      row_pitch = width * 2;  // width * sizeof(uint16_t)
      break;
    default:
      return;
      break;
  }

  ID3D11Texture2D* d3dtex = (ID3D11Texture2D*)texture_handle;
  assert(d3dtex);
  ID3D11DeviceContext* ctx = NULL;
  m_Device->GetImmediateContext(&ctx);

  // make sure to set front and back such that front < back
  // even if they are not needed so that this works
  D3D11_BOX box;
  box.left = xoffset;
  box.top = yoffset;
  box.front = 0;
  box.right = xoffset + width;
  box.bottom = yoffset + height;
  box.back = 1;

  ctx->UpdateSubresource(d3dtex, 0, &box, data_ptr, row_pitch, 0);
  ctx->Release();
}

void TextureSubPluginAPI_D3D11::TextureSubImage3D(
    void* texture_handle, int32_t xoffset, int32_t yoffset, int32_t zoffset,
    int32_t width, int32_t height, int32_t depth, void* data_ptr, int32_t level,
    Format format) {
  // determine row pitch/depth from provided format
  uint32_t row_pitch;
  switch (format) {
    case Format::R8_UINT:
      row_pitch = width;  // width * sizeof(uint8_t)
      break;
    case Format::R16_UINT:
      row_pitch = width * 2;  // width * sizeof(uint16_t)
      break;
    default:
      return;
      break;
  }

  uint32_t depth_pitch = height * row_pitch;
  ID3D11Texture2D* d3dtex = (ID3D11Texture2D*)texture_handle;
  assert(d3dtex);
  ID3D11DeviceContext* ctx = NULL;
  m_Device->GetImmediateContext(&ctx);

  D3D11_BOX box;
  box.left = xoffset;
  box.top = yoffset;
  box.front = zoffset;
  box.right = xoffset + width;
  box.bottom = yoffset + height;
  box.back = zoffset + depth;

  ctx->UpdateSubresource(d3dtex, 0, &box, data_ptr, row_pitch, depth_pitch);
  ctx->Release();
}

// google: direct3d 11 resources limits
void TextureSubPluginAPI_D3D11::CreateTexture3D(uint32_t width, uint32_t height,
                                                uint32_t depth, Format format,
                                                void*& texture) {
  uint32_t size_in_bytes;
  D3D11_TEXTURE3D_DESC desc;
  desc.Width = width;
  desc.Height = height;
  desc.Depth = depth;
  desc.MipLevels = 1;
  switch (format) {
    case R8_UINT:
      desc.Format = DXGI_FORMAT_R8_UNORM;
      size_in_bytes = width * height * depth;
      break;
    case R16_UINT:
      desc.Format = DXGI_FORMAT_R16_UNORM;
      size_in_bytes = width * height * depth * 2;
      break;
    default:
      texture = NULL;
      return;
      break;
  }
  desc.Usage = D3D11_USAGE_DEFAULT;
  desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  desc.CPUAccessFlags = 0;
  desc.MiscFlags = 0;

  if (size_in_bytes > D3D11_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_C_TERM) {
    std::ostringstream msg;
    msg << "Texture size exceeds Direct3D 11/12 max resource size. Texture "
           "Size: "
        << size_in_bytes / (1024 * 1024) << "MB"
        << " Max: " << D3D11_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_C_TERM
        << "MB";
    UNITY_LOG_ERROR(g_Log, msg.str().c_str());
    texture = NULL;
    return;
  }

  ID3D11DeviceContext* ctx = NULL;
  m_Device->GetImmediateContext(&ctx);

  ID3D11Texture3D* d3dtex;
  HRESULT result = m_Device->CreateTexture3D(&desc, NULL, &d3dtex);
  if (S_OK != result) {
    std::ostringstream msg;
    msg << "CreateTexture3D failed, return code: 0x" << std::hex << result;
    UNITY_LOG_ERROR(g_Log, msg.str().c_str());
    texture = NULL;
    return;
  }
  std::ostringstream address;
  address << "created texture 3D ID3D11Texture3D ptr: 0x" << std::hex << d3dtex;
  UNITY_LOG(g_Log, address.str().c_str());

  texture = d3dtex;
  ctx->Release();
}

void TextureSubPluginAPI_D3D11::ClearTexture3D(void* texture_handle) {
  ID3D11Texture3D* d3dtex = (ID3D11Texture3D*)texture_handle;
  assert(d3dtex);
  // TODO: is this the correct way to release a resource in Direct3D 11?
  // d3dtex->Release();
}

#endif  // #if SUPPORT_D3D11
