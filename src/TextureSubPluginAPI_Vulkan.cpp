#include "PlatformBase.hpp"
#include "TextureSubPluginAPI.hpp"

#if SUPPORT_VULKAN

#include <math.h>
#include <string.h>
#include <sstream>

#include <map>
#include <vector>

// This plugin does not link to the Vulkan loader, easier to support multiple
// APIs and systems that don't have Vulkan support
#define VK_NO_PROTOTYPES
#include "Unity/IUnityGraphicsVulkan.h"

#define UNITY_USED_VULKAN_API_FUNCTIONS(apply) \
  apply(vkCreateInstance);                     \
  apply(vkCmdBeginRenderPass);                 \
  apply(vkCreateBuffer);                       \
  apply(vkGetPhysicalDeviceMemoryProperties);  \
  apply(vkGetBufferMemoryRequirements);        \
  apply(vkMapMemory);                          \
  apply(vkBindBufferMemory);                   \
  apply(vkAllocateMemory);                     \
  apply(vkDestroyBuffer);                      \
  apply(vkFreeMemory);                         \
  apply(vkUnmapMemory);                        \
  apply(vkQueueWaitIdle);                      \
  apply(vkDeviceWaitIdle);                     \
  apply(vkCmdCopyBufferToImage);               \
  apply(vkFlushMappedMemoryRanges);            \
  apply(vkCreatePipelineLayout);               \
  apply(vkCreateShaderModule);                 \
  apply(vkDestroyShaderModule);                \
  apply(vkCreateGraphicsPipelines);            \
  apply(vkCmdBindPipeline);                    \
  apply(vkCmdDraw);                            \
  apply(vkCmdPushConstants);                   \
  apply(vkCmdBindVertexBuffers);               \
  apply(vkDestroyPipeline);                    \
  apply(vkDestroyPipelineLayout);

#define VULKAN_DEFINE_API_FUNCPTR(func) static PFN_##func func
VULKAN_DEFINE_API_FUNCPTR(vkGetInstanceProcAddr);
UNITY_USED_VULKAN_API_FUNCTIONS(VULKAN_DEFINE_API_FUNCPTR);
#undef VULKAN_DEFINE_API_FUNCPTR

static void LoadVulkanAPI(PFN_vkGetInstanceProcAddr getInstanceProcAddr,
                          VkInstance instance) {
  if (!vkGetInstanceProcAddr && getInstanceProcAddr)
    vkGetInstanceProcAddr = getInstanceProcAddr;

  if (!vkCreateInstance)
    vkCreateInstance = (PFN_vkCreateInstance)vkGetInstanceProcAddr(
        VK_NULL_HANDLE, "vkCreateInstance");

#define LOAD_VULKAN_FUNC(fn) \
  if (!fn) fn = (PFN_##fn)vkGetInstanceProcAddr(instance, #fn)
  UNITY_USED_VULKAN_API_FUNCTIONS(LOAD_VULKAN_FUNC);
#undef LOAD_VULKAN_FUNC
}

static VKAPI_ATTR void VKAPI_CALL Hook_vkCmdBeginRenderPass(
    VkCommandBuffer commandBuffer,
    const VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents contents) {
  // Change this to 'true' to override the clear color with green
  const bool allowOverrideClearColor = false;
  if (pRenderPassBegin->clearValueCount <= 16 &&
      pRenderPassBegin->clearValueCount > 0 && allowOverrideClearColor) {
    VkClearValue clearValues[16] = {};
    memcpy(clearValues, pRenderPassBegin->pClearValues,
           pRenderPassBegin->clearValueCount * sizeof(VkClearValue));

    VkRenderPassBeginInfo patchedBeginInfo = *pRenderPassBegin;
    patchedBeginInfo.pClearValues = clearValues;
    for (unsigned int i = 0; i < pRenderPassBegin->clearValueCount - 1; ++i) {
      clearValues[i].color.float32[0] = 0.0;
      clearValues[i].color.float32[1] = 1.0;
      clearValues[i].color.float32[2] = 0.0;
      clearValues[i].color.float32[3] = 1.0;
    }
    vkCmdBeginRenderPass(commandBuffer, &patchedBeginInfo, contents);
  } else {
    vkCmdBeginRenderPass(commandBuffer, pRenderPassBegin, contents);
  }
}

static VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkInstance* pInstance) {
  vkCreateInstance = (PFN_vkCreateInstance)vkGetInstanceProcAddr(
      VK_NULL_HANDLE, "vkCreateInstance");
  VkResult result = vkCreateInstance(pCreateInfo, pAllocator, pInstance);
  if (result == VK_SUCCESS) LoadVulkanAPI(vkGetInstanceProcAddr, *pInstance);

  return result;
}

static int FindMemoryTypeIndex(
    VkPhysicalDeviceMemoryProperties const& physicalDeviceMemoryProperties,
    VkMemoryRequirements const& memoryRequirements,
    VkMemoryPropertyFlags memoryPropertyFlags) {
  uint32_t memoryTypeBits = memoryRequirements.memoryTypeBits;

  // Search memtypes to find first index with those properties
  for (uint32_t memoryTypeIndex = 0; memoryTypeIndex < VK_MAX_MEMORY_TYPES;
       ++memoryTypeIndex) {
    if ((memoryTypeBits & 1) == 1) {
      // Type is available, does it match user properties?
      if ((physicalDeviceMemoryProperties.memoryTypes[memoryTypeIndex]
               .propertyFlags &
           memoryPropertyFlags) == memoryPropertyFlags)
        return memoryTypeIndex;
    }
    memoryTypeBits >>= 1;
  }

  return -1;
}

static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
Hook_vkGetInstanceProcAddr(VkInstance device, const char* funcName) {
  if (!funcName) return NULL;

#define INTERCEPT(fn) \
  if (strcmp(funcName, #fn) == 0) return (PFN_vkVoidFunction) & Hook_##fn
  INTERCEPT(vkCreateInstance);
#undef INTERCEPT

  return NULL;
}

static PFN_vkGetInstanceProcAddr UNITY_INTERFACE_API
InterceptVulkanInitialization(PFN_vkGetInstanceProcAddr getInstanceProcAddr,
                              void*) {
  vkGetInstanceProcAddr = getInstanceProcAddr;
  return Hook_vkGetInstanceProcAddr;
}

extern "C" void RenderAPI_Vulkan_OnPluginLoad(IUnityInterfaces* interfaces) {
  if (IUnityGraphicsVulkanV2* vulkanInterface =
          interfaces->Get<IUnityGraphicsVulkanV2>())
    vulkanInterface->AddInterceptInitialization(InterceptVulkanInitialization,
                                                NULL, 0);
  else if (IUnityGraphicsVulkan* vulkanInterface =
               interfaces->Get<IUnityGraphicsVulkan>())
    vulkanInterface->InterceptInitialization(InterceptVulkanInitialization,
                                             NULL);
}

struct VulkanBuffer {
  VkBuffer buffer;
  VkDeviceMemory deviceMemory;
  void* mapped;
  VkDeviceSize sizeInBytes;
  VkDeviceSize deviceMemorySize;
  VkMemoryPropertyFlags deviceMemoryFlags;
};

class TextureSubPluginAPI_Vulkan : public TextureSubPluginAPI {
 public:
  TextureSubPluginAPI_Vulkan();
  virtual ~TextureSubPluginAPI_Vulkan() {}

  virtual void ProcessDeviceEvent(UnityGfxDeviceEventType type,
                                  IUnityInterfaces* interfaces);

  virtual void CreateTexture3D(uint32_t width, uint32_t height, uint32_t depth,
                               Format format, void*& texture){};

  virtual void ClearTexture3D(void* texture_handle){};

  virtual void TextureSubImage2D(void* texture_handle, int32_t xoffset,
                                 int32_t yoffset, int32_t width, int32_t height,
                                 void* data_ptr, int32_t level,
                                 Format format){};

  virtual void TextureSubImage3D(void* texture_handle, int32_t xoffset,
                                 int32_t yoffset, int32_t zoffset,
                                 int32_t width, int32_t height, int32_t depth,
                                 void* data_ptr, int32_t level, Format format);

 private:
  typedef std::vector<VulkanBuffer> VulkanBuffers;
  typedef std::map<unsigned long long, VulkanBuffers> DeleteQueue;

 private:
  bool CreateVulkanBuffer(size_t bytes, VulkanBuffer* buffer,
                          VkBufferUsageFlags usage);
  void ImmediateDestroyVulkanBuffer(const VulkanBuffer& buffer);
  void SafeDestroy(unsigned long long frameNumber, const VulkanBuffer& buffer);
  void GarbageCollect(bool force = false);

 private:
  IUnityGraphicsVulkan* m_UnityVulkan;
  UnityVulkanInstance m_Instance;
  VulkanBuffer m_TextureStagingBuffer;
  std::map<unsigned long long, VulkanBuffers> m_DeleteQueue;
};

TextureSubPluginAPI* CreateRenderAPI_Vulkan() {
  return new TextureSubPluginAPI_Vulkan();
}

TextureSubPluginAPI_Vulkan::TextureSubPluginAPI_Vulkan()
    : m_UnityVulkan(NULL), m_TextureStagingBuffer() {}

void TextureSubPluginAPI_Vulkan::ProcessDeviceEvent(
    UnityGfxDeviceEventType type, IUnityInterfaces* interfaces) {
  switch (type) {
    case kUnityGfxDeviceEventInitialize: {
      m_UnityVulkan = interfaces->Get<IUnityGraphicsVulkan>();
      m_Instance = m_UnityVulkan->Instance();

      // Make sure Vulkan API functions are loaded
      LoadVulkanAPI(m_Instance.getInstanceProcAddr, m_Instance.instance);

      UnityVulkanPluginEventConfig config_1{};
      config_1.graphicsQueueAccess = kUnityVulkanGraphicsQueueAccess_DontCare;
      config_1.renderPassPrecondition = kUnityVulkanRenderPass_EnsureInside;
      config_1.flags =
          kUnityVulkanEventConfigFlag_EnsurePreviousFrameSubmission |
          kUnityVulkanEventConfigFlag_ModifiesCommandBuffersState;
      m_UnityVulkan->ConfigureEvent(1, &config_1);

      // alternative way to intercept API
      m_UnityVulkan->InterceptVulkanAPI(
          "vkCmdBeginRenderPass",
          (PFN_vkVoidFunction)Hook_vkCmdBeginRenderPass);
      break;
    }
    case kUnityGfxDeviceEventShutdown: {
      if (m_Instance.device != VK_NULL_HANDLE) {
        GarbageCollect(true);
      }
      m_UnityVulkan = NULL;
      m_Instance = UnityVulkanInstance();
      break;
    }
  }
}

bool TextureSubPluginAPI_Vulkan::CreateVulkanBuffer(size_t sizeInBytes,
                                                    VulkanBuffer* buffer,
                                                    VkBufferUsageFlags usage) {
  if (sizeInBytes == 0) return false;

  VkBufferCreateInfo bufferCreateInfo{};
  bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferCreateInfo.pNext = NULL;
  bufferCreateInfo.pQueueFamilyIndices = &m_Instance.queueFamilyIndex;
  bufferCreateInfo.queueFamilyIndexCount = 1;
  bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  bufferCreateInfo.usage = usage;
  bufferCreateInfo.flags = 0;
  bufferCreateInfo.size = sizeInBytes;

  *buffer = VulkanBuffer();

  if (vkCreateBuffer(m_Instance.device, &bufferCreateInfo, NULL,
                     &buffer->buffer) != VK_SUCCESS)
    return false;

  VkPhysicalDeviceMemoryProperties physicalDeviceProperties;
  vkGetPhysicalDeviceMemoryProperties(m_Instance.physicalDevice,
                                      &physicalDeviceProperties);

  VkMemoryRequirements memoryRequirements;
  vkGetBufferMemoryRequirements(m_Instance.device, buffer->buffer,
                                &memoryRequirements);

  const int memoryTypeIndex =
      FindMemoryTypeIndex(physicalDeviceProperties, memoryRequirements,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
  if (memoryTypeIndex < 0) {
    ImmediateDestroyVulkanBuffer(*buffer);
    return false;
  }

  VkMemoryAllocateInfo memoryAllocateInfo{};
  memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  memoryAllocateInfo.pNext = NULL;
  memoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;
  memoryAllocateInfo.allocationSize = memoryRequirements.size;

  if (vkAllocateMemory(m_Instance.device, &memoryAllocateInfo, NULL,
                       &buffer->deviceMemory) != VK_SUCCESS) {
    ImmediateDestroyVulkanBuffer(*buffer);
    return false;
  }

  if (vkMapMemory(m_Instance.device, buffer->deviceMemory, 0, VK_WHOLE_SIZE, 0,
                  &buffer->mapped) != VK_SUCCESS) {
    ImmediateDestroyVulkanBuffer(*buffer);
    return false;
  }

  if (vkBindBufferMemory(m_Instance.device, buffer->buffer,
                         buffer->deviceMemory, 0) != VK_SUCCESS) {
    ImmediateDestroyVulkanBuffer(*buffer);
    return false;
  }

  buffer->sizeInBytes = sizeInBytes;
  buffer->deviceMemoryFlags =
      physicalDeviceProperties.memoryTypes[memoryTypeIndex].propertyFlags;
  buffer->deviceMemorySize = memoryAllocateInfo.allocationSize;

  return true;
}

void TextureSubPluginAPI_Vulkan::ImmediateDestroyVulkanBuffer(
    const VulkanBuffer& buffer) {
  if (buffer.buffer != VK_NULL_HANDLE)
    vkDestroyBuffer(m_Instance.device, buffer.buffer, NULL);

  if (buffer.mapped && buffer.deviceMemory != VK_NULL_HANDLE)
    vkUnmapMemory(m_Instance.device, buffer.deviceMemory);

  if (buffer.deviceMemory != VK_NULL_HANDLE)
    vkFreeMemory(m_Instance.device, buffer.deviceMemory, NULL);
}

void TextureSubPluginAPI_Vulkan::SafeDestroy(unsigned long long frameNumber,
                                             const VulkanBuffer& buffer) {
  m_DeleteQueue[frameNumber].push_back(buffer);
}

void TextureSubPluginAPI_Vulkan::GarbageCollect(bool force /*= false*/) {
  UnityVulkanRecordingState recordingState{};
  if (force)
    recordingState.safeFrameNumber = ~0ull;
  else if (!m_UnityVulkan->CommandRecordingState(
               &recordingState, kUnityVulkanGraphicsQueueAccess_DontCare))
    return;

  DeleteQueue::iterator it = m_DeleteQueue.begin();
  while (it != m_DeleteQueue.end()) {
    if (it->first <= recordingState.safeFrameNumber) {
      for (size_t i = 0; i < it->second.size(); ++i)
        ImmediateDestroyVulkanBuffer(it->second[i]);
      m_DeleteQueue.erase(it++);
    } else
      ++it;
  }
}

void TextureSubPluginAPI_Vulkan::TextureSubImage3D(
    void* texture_handle, int32_t xoffset, int32_t yoffset, int32_t zoffset,
    int32_t width, int32_t height, int32_t depth, void* data_ptr, int32_t level,
    Format format) {
  UnityVulkanRecordingState recordingState;
  if (!m_UnityVulkan->CommandRecordingState(
          &recordingState, kUnityVulkanGraphicsQueueAccess_DontCare)) {
    std::ostringstream ss;
    ss << __FUNCTION__
       << " failed to intercept the current command buffer state";
    UNITY_LOG_ERROR(g_Log, ss.str().c_str());
    return;
  }
  // safely (not necessarily immediately) destroy the current staging buffer
  // before creating a new one. A staging buffer is simply a buffer in host
  // (CPU) visible memory that we copy image data to which then a command on
  // the client (GPU) copies a (sub)region from.
  // TODO:  maybe we can check if requirements have changed. If not, then no
  //        staging buffer recreation has to be done.
  SafeDestroy(recordingState.currentFrameNumber, m_TextureStagingBuffer);
  size_t data_size;
  switch (format) {
    case R8_UINT:
      data_size = static_cast<size_t>(width) * height * height;
      break;
    case R16_UINT:
      data_size = static_cast<size_t>(width) * height * height * 2;
      break;
    default:
      return;
  }
  m_TextureStagingBuffer = VulkanBuffer();
  if (!CreateVulkanBuffer(data_size, &m_TextureStagingBuffer,
                          VK_BUFFER_USAGE_TRANSFER_SRC_BIT)) {
    std::ostringstream ss;
    ss << __FUNCTION__
       << " failed to create texture staging buffer";
    UNITY_LOG_ERROR(g_Log, ss.str().c_str());
    return;
  }
  memcpy(m_TextureStagingBuffer.mapped, data_ptr, data_size);
  // vkUnmapMemory(m_Instance.device, m_TextureStagingBuffer.deviceMemory);

  // cannot do resource uploads inside renderpass
  m_UnityVulkan->EnsureOutsideRenderPass();

  // get the VkImage from the provided texture handle
  UnityVulkanImage image;
  if (!m_UnityVulkan->AccessTexture(
          texture_handle, UnityVulkanWholeImage,
          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
          VK_ACCESS_TRANSFER_WRITE_BIT,
          kUnityVulkanResourceAccess_PipelineBarrier, &image)) {
    std::ostringstream ss;
    ss << __FUNCTION__
       << " failed to access texture from provided texture handle: "
       << texture_handle;
    UNITY_LOG_ERROR(g_Log, ss.str().c_str());
    return;
  }

  VkBufferImageCopy region{};
  region.bufferImageHeight = 0;
  region.bufferRowLength = 0;
  region.bufferOffset = 0;
  region.imageOffset.x = xoffset;
  region.imageOffset.y = yoffset;
  region.imageOffset.z = zoffset;
  region.imageExtent.width = width;
  region.imageExtent.height = height;
  region.imageExtent.depth = depth;
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageSubresource.mipLevel = static_cast<uint32_t>(level);
  vkCmdCopyBufferToImage(recordingState.commandBuffer,
                         m_TextureStagingBuffer.buffer, image.image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

#endif  // #if SUPPORT_VULKAN
