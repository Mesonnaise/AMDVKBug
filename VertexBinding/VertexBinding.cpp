#include<array>
#include<vector>
#include<iostream>
#include<fstream>
#include<filesystem>
#include<vulkan\vulkan.h>
#include<glm/vec3.hpp>
#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include<vma/vk_mem_alloc.h>

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
  VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT messageType,
  const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
  void *pUserData){

  std::cerr<<"Validation Layer: "<<pCallbackData->pMessage<<std::endl;

  return VK_FALSE; // Return VK_TRUE to terminate the application
}


static std::vector<uint32_t> LoadShader(std::filesystem::path filePath){
  std::vector<uint32_t> buffer;
  size_t bufferSize=0;

  if(!std::filesystem::exists(filePath))
    throw std::runtime_error("Unable to find file");

  std::fstream fileStream(filePath.string(),std::ios::binary|std::ios::in);
  if(!fileStream.is_open())
    throw std::runtime_error("Unable to open file handle");

  fileStream.seekg(0,fileStream.end);
  bufferSize=fileStream.tellg();
  fileStream.seekg(0,fileStream.beg);

  if(bufferSize%sizeof(uint32_t)!=0)
    throw std::runtime_error("Invalid shader file size");

  buffer.resize(bufferSize/sizeof(uint32_t));
  fileStream.read(reinterpret_cast<char *>(buffer.data()),bufferSize);
  fileStream.close();

  return buffer;
}

int main(){
  //************** Instance ***********************
#pragma region Instance
  std::array<const char *,0> InstanceLayers={"VK_LAYER_KHRONOS_validation"};
  std::array<const char *,1> InstanceExtensions={VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
  std::array<const char *,4> DeviceExtensions={
    VK_EXT_SHADER_OBJECT_EXTENSION_NAME,
    VK_EXT_MESH_SHADER_EXTENSION_NAME,
    VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME,
    VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME};

  VkApplicationInfo applicationInfo={
    .sType=VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName="Test",
    .applicationVersion=VK_MAKE_VERSION(1, 0, 0),
    .pEngineName="TestEngine",
    .engineVersion=VK_MAKE_VERSION(1, 0, 0),
    .apiVersion=VK_API_VERSION_1_4
  };

  VkInstanceCreateInfo instanceInfo={
    .sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pNext=nullptr,
    .flags=0,
    .pApplicationInfo=&applicationInfo,
    .enabledLayerCount=(uint32_t)InstanceLayers.size(),
    .ppEnabledLayerNames=InstanceLayers.data(),
    .enabledExtensionCount=(uint32_t)InstanceExtensions.size(),
    .ppEnabledExtensionNames=InstanceExtensions.data()
  };
  VkInstance instance=nullptr;
  VkResult result=vkCreateInstance(&instanceInfo,nullptr,&instance);
  if(result!=VK_SUCCESS)
    throw std::runtime_error("Failed to create instance");

  PFN_vkCreateDebugUtilsMessengerEXT pfCreateDebugUtilsMessengerEXT=reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
    vkGetInstanceProcAddr(instance,"vkCreateDebugUtilsMessengerEXT"));

  VkDebugUtilsMessengerEXT debugMessenger;
  VkDebugUtilsMessengerCreateInfoEXT createInfo={
    .sType=VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .messageSeverity=VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT|
                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT|
                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT|
                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    .messageType=VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT|
                 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT|
                 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT|
                 VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT,
    .pfnUserCallback=&DebugCallback,
    .pUserData=nullptr
  };
  pfCreateDebugUtilsMessengerEXT(instance,&createInfo,nullptr,&debugMessenger);

  std::vector<VkPhysicalDevice> physicalDevices;
  uint32_t physicalDeviceCount=0;
  vkEnumeratePhysicalDevices(instance,&physicalDeviceCount,nullptr);
  physicalDevices.resize(physicalDeviceCount);
  vkEnumeratePhysicalDevices(instance,&physicalDeviceCount,physicalDevices.data());

  if(physicalDeviceCount<1)
    throw std::runtime_error("Unable to find graphics device");
#pragma endregion

  //*************** Device ************************
#pragma region Device

  VkPhysicalDeviceBufferDeviceAddressFeatures BufferDeviceAddressFeatures={
    .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR,
    .pNext=nullptr,
    .bufferDeviceAddress=VK_TRUE,
    .bufferDeviceAddressCaptureReplay=VK_FALSE,
    .bufferDeviceAddressMultiDevice=VK_FALSE
  };

  VkPhysicalDeviceDescriptorBufferFeaturesEXT DescriptorBufferFeatures={
    .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT,
    .pNext=&BufferDeviceAddressFeatures,
    .descriptorBuffer=VK_TRUE,
    .descriptorBufferCaptureReplay=VK_FALSE,
    .descriptorBufferImageLayoutIgnored=VK_FALSE,
    .descriptorBufferPushDescriptors=VK_TRUE
  };

  VkPhysicalDeviceShaderObjectFeaturesEXT ShaderObjectFeatures={
    .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT,
    .pNext=&DescriptorBufferFeatures,
    .shaderObject=VK_TRUE
  };

  float queuePriorities=1.0;
  const VkDeviceQueueCreateInfo queueCreateInfos={
    .sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    .pNext=nullptr,
    .flags=0,
    .queueFamilyIndex=0,
    .queueCount=1,
    .pQueuePriorities=&queuePriorities,
  };

  VkDeviceCreateInfo deviceInfo={
    .sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext=&ShaderObjectFeatures,
    .flags=0,
    .queueCreateInfoCount=1,
    .pQueueCreateInfos=&queueCreateInfos,
    .enabledLayerCount=0,
    .ppEnabledLayerNames=nullptr,
    .enabledExtensionCount=(uint32_t)DeviceExtensions.size(),
    .ppEnabledExtensionNames=DeviceExtensions.data(),
    .pEnabledFeatures=nullptr,
  };

  VkDevice device=nullptr;
  result=vkCreateDevice(physicalDevices[0],&deviceInfo,nullptr,&device);
  if(result!=VK_SUCCESS)
    throw std::runtime_error("Failed to create device");
#pragma endregion

  //****** Vulkan function loading ****************
#pragma region Functions
  auto pfCreateShader=reinterpret_cast<PFN_vkCreateShadersEXT>(
    vkGetDeviceProcAddr(device,"vkCreateShadersEXT"));
  auto pfDestroyShader=reinterpret_cast<PFN_vkDestroyShaderEXT>(
    vkGetDeviceProcAddr(device,"vkDestroyShaderEXT"));

  auto pfCmdBindShaders=reinterpret_cast<PFN_vkCmdBindShadersEXT>(
    vkGetDeviceProcAddr(device,"vkCmdBindShadersEXT"));

  auto pfGetDescriptorSetLayoutSize=reinterpret_cast<PFN_vkGetDescriptorSetLayoutSizeEXT>(
    vkGetDeviceProcAddr(device,"vkGetDescriptorSetLayoutSizeEXT"));

  auto pfGetDescriptorSetLayoutBindingOffset=
    reinterpret_cast<PFN_vkGetDescriptorSetLayoutBindingOffsetEXT>(
      vkGetDeviceProcAddr(device,"vkGetDescriptorSetLayoutBindingOffsetEXT"));

  auto pfnCmdBindDescriptorBuffersEXT=
    reinterpret_cast<PFN_vkCmdBindDescriptorBuffersEXT>(
      vkGetDeviceProcAddr(device,"vkCmdBindDescriptorBuffersEXT"));

  auto pfnGetDescriptorEXT=
    reinterpret_cast<PFN_vkGetDescriptorEXT>(
      vkGetDeviceProcAddr(device,"vkGetDescriptorEXT"));

  auto pfnCmdSetDescriptorBufferOffsetsEXT=
    reinterpret_cast<PFN_vkCmdSetDescriptorBufferOffsetsEXT>(
      vkGetDeviceProcAddr(device,"vkCmdSetDescriptorBufferOffsetsEXT"));

  auto pfnCmdSetProvokingVertexModeEXT=reinterpret_cast<PFN_vkCmdSetProvokingVertexModeEXT>(
    vkGetDeviceProcAddr(device,"vkCmdSetProvokingVertexModeEXT"));

  auto pfnCmdSetPolygonModeEXT=reinterpret_cast<PFN_vkCmdSetPolygonModeEXT>(
    vkGetDeviceProcAddr(device,"vkCmdSetPolygonModeEXT"));

  auto pfnCmdSetDepthClampEnableEXT=reinterpret_cast<PFN_vkCmdSetDepthClampEnableEXT>(
    vkGetDeviceProcAddr(device,"vkCmdSetDepthClampEnableEXT"));

  auto pfnCmdSetVertexInputEXT=reinterpret_cast<PFN_vkCmdSetVertexInputEXT>(
    vkGetDeviceProcAddr(device,"vkCmdSetVertexInputEXT"));

  auto pfnCmdBindShaders=reinterpret_cast<PFN_vkCmdBindShadersEXT>(
    vkGetDeviceProcAddr(device,"vkCmdBindShadersEXT"));

#pragma endregion

  //*************** Layout ************************
#pragma region Layout

  /*VkPhysicalDeviceDescriptorBufferPropertiesEXT DescriptorBufferProperties={
    .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT
  };
  VkPhysicalDeviceProperties2 DeviceProperties={
    .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
    .pNext=&DescriptorBufferProperties
  };

  vkGetPhysicalDeviceProperties2(physicalDevices[0],&DeviceProperties);*/

  auto CreateSetLayout=[&device](std::vector<VkDescriptorSetLayoutBinding> bindings)->VkDescriptorSetLayout{
    VkDescriptorSetLayoutCreateInfo descriptorSetInfo={
      .sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext=nullptr,
      .flags=0,
      .bindingCount=(uint32_t)bindings.size(),
      .pBindings=bindings.data()
    };
    VkDescriptorSetLayout layout=nullptr;
    auto result=vkCreateDescriptorSetLayout(device,&descriptorSetInfo,nullptr,&layout);
    if(result!=VK_SUCCESS)
      throw std::runtime_error("Failed to create descriptor set layout");

    return layout;
    };

  auto setLayout=CreateSetLayout({});
#pragma endregion

  //*************** Shader ************************
#pragma region Shader
  std::vector<VkDescriptorSetLayout> computeLayout={setLayout};
  auto vertShaderCode=LoadShader("VertexBindingVert.spv");
  auto fragShaderCode=LoadShader("VertexBindingFrag.spv");

  std::vector<VkShaderCreateInfoEXT> ShaderCreateInfos={{
    .sType=VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT,
    .pNext=nullptr,
    .flags=0,
    .stage=VK_SHADER_STAGE_VERTEX_BIT,
    .nextStage=VK_SHADER_STAGE_FRAGMENT_BIT,
    .codeType=VK_SHADER_CODE_TYPE_SPIRV_EXT,
    .codeSize=(uint32_t)vertShaderCode.size()*sizeof(uint32_t),
    .pCode=vertShaderCode.data(),
    .pName="main",
    .setLayoutCount=1,
    .pSetLayouts=&setLayout,
    .pushConstantRangeCount=0,
    .pPushConstantRanges=nullptr,
    .pSpecializationInfo=nullptr
  },{
    .sType=VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT,
    .pNext=nullptr,
    .flags=0,
    .stage=VK_SHADER_STAGE_FRAGMENT_BIT,
    .nextStage=0,
    .codeType=VK_SHADER_CODE_TYPE_SPIRV_EXT,
    .codeSize=(uint32_t)fragShaderCode.size()*sizeof(uint32_t),
    .pCode=fragShaderCode.data(),
    .pName="main",
    .setLayoutCount=1,
    .pSetLayouts=&setLayout,
    .pushConstantRangeCount=0,
    .pPushConstantRanges=nullptr,
    .pSpecializationInfo=nullptr
  }};

  std::vector<VkShaderEXT> shaders(ShaderCreateInfos.size(),nullptr);
  result=pfCreateShader(device,(uint32_t)ShaderCreateInfos.size(),ShaderCreateInfos.data(),nullptr,shaders.data());
  if(result!=VK_SUCCESS)
    throw std::runtime_error("Failed to create shader objects");

#pragma endregion

  //************ Image/Buffer *********************
#pragma region ImageBuffer
  VmaAllocator allocator=nullptr;

  VmaVulkanFunctions vulkanFunctions={
  .vkGetInstanceProcAddr=&vkGetInstanceProcAddr,
  .vkGetDeviceProcAddr=&vkGetDeviceProcAddr,
  };
  VmaAllocatorCreateInfo allocatorCreateInfo={
    .flags=VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
    .physicalDevice=physicalDevices[0],
    .device=device,
    .preferredLargeHeapBlockSize=0,
    .pAllocationCallbacks=nullptr,
    .pDeviceMemoryCallbacks=nullptr,
    .pHeapSizeLimit=nullptr,
    .pVulkanFunctions=&vulkanFunctions,
    .instance=instance,
    .vulkanApiVersion=VK_API_VERSION_1_4,
    .pTypeExternalMemoryHandleTypes=nullptr
  };
  result=vmaCreateAllocator(&allocatorCreateInfo,&allocator);
  if(result!=VK_SUCCESS)
    throw std::runtime_error("Failed to create VMA allocator");

  VkImage framebuffer=nullptr;
  VkImageView framebufferView=nullptr;
  VmaAllocationInfo framebufferAllocationInfo={};
  VmaAllocation framebufferAllocation=nullptr;

  VkImageCreateInfo imageInfo={
    .sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .pNext=nullptr,
    .imageType=VK_IMAGE_TYPE_2D,
    .format=VK_FORMAT_B8G8R8_SRGB,
    .extent={512,512,1},
    .mipLevels=1,
    .arrayLayers=1,
    .samples=VK_SAMPLE_COUNT_1_BIT,
    .tiling=VK_IMAGE_TILING_OPTIMAL,
    .usage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .sharingMode=VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount=0,
    .pQueueFamilyIndices=nullptr,
    .initialLayout=VK_IMAGE_LAYOUT_UNDEFINED
  };
  
  VmaAllocationCreateInfo imageAllocateInfo={
    .flags=0,
    .usage=VMA_MEMORY_USAGE_AUTO,
    .requiredFlags=VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    .preferredFlags=0,
    .memoryTypeBits=0,
    .pool=nullptr,
    .pUserData=nullptr,
    .priority=0.0f
  };

  result=vmaCreateImage(allocator,&imageInfo,&imageAllocateInfo,&framebuffer,&framebufferAllocation,&framebufferAllocationInfo);
  if(result!=VK_SUCCESS)
    throw std::runtime_error("Failed to create framebuffer");

  VkImageViewCreateInfo imageviewInfo={
    .sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .pNext=nullptr,
    .flags=0,
    .image=framebuffer,
    .viewType=VK_IMAGE_VIEW_TYPE_2D,
    .format=VK_FORMAT_B8G8R8_SRGB,
    .components={
      .r=VK_COMPONENT_SWIZZLE_IDENTITY,
      .g=VK_COMPONENT_SWIZZLE_IDENTITY,
      .b=VK_COMPONENT_SWIZZLE_IDENTITY,
      .a=VK_COMPONENT_SWIZZLE_IDENTITY
    },
    .subresourceRange={
      .aspectMask=VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel=0,
      .levelCount=1,
      .baseArrayLayer=0,
      .layerCount=1
    }
  };

  result=vkCreateImageView(device,&imageviewInfo,nullptr,&framebufferView);
  if(result!=VK_SUCCESS)
    throw std::runtime_error("Failed to create framebuffer view");

  VmaAllocationInfo vertexAllocationInfo={};
  VkBuffer vertexBuffer=nullptr;
  VmaAllocation vertexAllocation=nullptr;

  VkBufferCreateInfo vertexBufferInfo={
    .sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .pNext=nullptr,
    .flags=0,
    .size=1024,
    .usage=VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT|VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    .sharingMode=VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount=0,
    .pQueueFamilyIndices=nullptr
  };



  VmaAllocationCreateInfo vertexAllocateInfo={
    .flags=VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT|VMA_ALLOCATION_CREATE_MAPPED_BIT,
    .usage=VMA_MEMORY_USAGE_AUTO,
    .requiredFlags=VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    .preferredFlags=VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    .memoryTypeBits=0,
    .pool=nullptr,
    .pUserData=nullptr,
    .priority=0.0f
  };

  result=vmaCreateBuffer(allocator,&vertexBufferInfo,&vertexAllocateInfo,&vertexBuffer,&vertexAllocation,&vertexAllocationInfo);
  if(result!=VK_SUCCESS)
    throw std::runtime_error("Failed to create vertex buffer");
#pragma endregion

  //************** Pipeline ***********************
#pragma region Pipeline
  //Pipeline layout is not needed
  //no shader resources are used
 /*VkPipelineLayout pipelineLayout=nullptr;
  VkPipelineLayoutCreateInfo pipelineLayoutInfo={
    .sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .pNext=nullptr,
    .flags=0,
    .setLayoutCount=1,
    .pSetLayouts=&setLayout,
    .pushConstantRangeCount=0,
    .pPushConstantRanges=nullptr
  };
  result=vkCreatePipelineLayout(device,&pipelineLayoutInfo,nullptr,&pipelineLayout);
  if(result!=VK_SUCCESS)
    throw std::runtime_error("Failed to create pipeline layout");
    */
#pragma endregion

  //*********** Command Buffer ********************
#pragma region Command
  VkCommandPool commandPool=nullptr;
  VkCommandPoolCreateInfo commandPoolInfo={
    .sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .pNext=nullptr,
    .flags=0,
    .queueFamilyIndex=0
  };
  result=vkCreateCommandPool(device,&commandPoolInfo,nullptr,&commandPool);
  if(result!=VK_SUCCESS)
    throw std::runtime_error("Failed to create command pool");

  VkCommandBuffer CMDBuffer;
  VkCommandBufferAllocateInfo bufferAllocatorInfo={
    .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .pNext=nullptr,
    .commandPool=commandPool,
    .level=VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount=1
  };

  result=vkAllocateCommandBuffers(device,&bufferAllocatorInfo,&CMDBuffer);
  if(result!=VK_SUCCESS)
    throw std::runtime_error("Failed to allocate command buffer");

#pragma endregion

  /************************************************
   *                                              *
   *        Command Buffer Operations             *
   *                                              *
   ************************************************/

  VkCommandBufferBeginInfo bufferBeginInfo={
    .sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .pNext=nullptr,
    .flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    .pInheritanceInfo=nullptr
  };

  VkVertexInputBindingDescription2EXT vertexInputBinding={
    .sType=VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT,
    .pNext=nullptr,
    .binding=0,
    .stride=sizeof(glm::vec3),
    .inputRate=VK_VERTEX_INPUT_RATE_VERTEX,
    .divisor=1
  };

  VkVertexInputAttributeDescription2EXT vertexInputAttribute{
    .sType=VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT,
    .pNext=nullptr,
    .location=0,
    .binding=0,
    .format=VK_FORMAT_R32G32B32_SFLOAT,
    .offset=0
  };

  VkViewport viewPort={
    .x=0.0f,
    .y=0.0f,
    .width=512,
    .height=512,
    .minDepth=0.0f,
    .maxDepth=1.0f
  };

  VkRect2D scissor={
    .offset={0,0},
    .extent={512,512}
  };

  VkRenderingAttachmentInfo attachmentInfo{
    .sType=VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
    .pNext=nullptr,
    .imageView=framebufferView,
    .imageLayout=VK_IMAGE_LAYOUT_GENERAL,
    .resolveMode=VK_RESOLVE_MODE_NONE,
    .resolveImageView=VK_NULL_HANDLE,
    .resolveImageLayout=VK_IMAGE_LAYOUT_UNDEFINED,
    .loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp=VK_ATTACHMENT_STORE_OP_STORE,
    .clearValue={.color={0.0,0.0,0.0,0.0}}
  };

  VkRenderingInfo renderingInfo={
    .sType=VK_STRUCTURE_TYPE_RENDERING_INFO,
    .pNext=nullptr,
    .flags=0,
    .renderArea={
      .offset={0,0},
      .extent={512,512}
    },
    .layerCount=1,
    .viewMask=0,
    .colorAttachmentCount=1,
    .pColorAttachments=&attachmentInfo,
    .pDepthAttachment=nullptr,
    .pStencilAttachment=nullptr
  };

  vkBeginCommandBuffer(CMDBuffer,&bufferBeginInfo);

  //Required graphic pipeline settings
  vkCmdBeginRendering(CMDBuffer,&renderingInfo);
  vkCmdSetDepthTestEnable(CMDBuffer,VK_FALSE);
  vkCmdSetDepthBiasEnable(CMDBuffer,VK_FALSE);
  pfnCmdSetDepthClampEnableEXT(CMDBuffer,VK_FALSE);
  vkCmdSetDepthBoundsTestEnable(CMDBuffer,VK_FALSE);
  vkCmdSetStencilTestEnable(CMDBuffer,VK_FALSE);
  vkCmdSetCullMode(CMDBuffer,VK_CULL_MODE_BACK_BIT);
  vkCmdSetRasterizerDiscardEnable(CMDBuffer,VK_FALSE);
  vkCmdSetFrontFace(CMDBuffer,VK_FRONT_FACE_COUNTER_CLOCKWISE);
  pfnCmdSetPolygonModeEXT(CMDBuffer,VK_POLYGON_MODE_FILL);
  vkCmdSetPrimitiveTopology(CMDBuffer,VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  pfnCmdSetProvokingVertexModeEXT(CMDBuffer,VK_PROVOKING_VERTEX_MODE_FIRST_VERTEX_EXT);
  vkCmdSetPrimitiveRestartEnable(CMDBuffer,VK_FALSE);

  //Binding shader
  std::array<VkShaderStageFlagBits,3> shaderStages={
    VK_SHADER_STAGE_VERTEX_BIT,VK_SHADER_STAGE_FRAGMENT_BIT,
    VK_SHADER_STAGE_GEOMETRY_BIT};

  shaders.push_back(VK_NULL_HANDLE);
  pfnCmdBindShaders(CMDBuffer,2,shaderStages.data(),shaders.data());

  pfnCmdSetVertexInputEXT(CMDBuffer,1,&vertexInputBinding,1,&vertexInputAttribute);
 
  VkDeviceSize offset=0;
  VkDeviceSize stride=sizeof(glm::vec3);
  //CRASH HERE - The AMD drivers will fail to check for bound vertex buffers.
  //This is a soft crash the driver will recover from.
  //vkCmdBindVertexBuffers2(CMDBuffer,0,1,&vertexBuffer,&offset,&vertexAllocationInfo.size,&stride);

  vkCmdDraw(CMDBuffer,3,1,0,0);

  vkCmdEndRendering(CMDBuffer);

  vkEndCommandBuffer(CMDBuffer);
  VkQueue queue=nullptr;
  vkGetDeviceQueue(device,0,0,&queue);


  VkSubmitInfo submitInfo={
    .sType=VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .pNext=nullptr,
    .waitSemaphoreCount=0,
    .pWaitSemaphores=nullptr,
    .pWaitDstStageMask=nullptr,
    .commandBufferCount=1,
    .pCommandBuffers=&CMDBuffer,
    .signalSemaphoreCount=0,
    .pSignalSemaphores=nullptr
  };

  vkQueueSubmit(queue,1,&submitInfo,nullptr);
  vkQueueWaitIdle(queue);

  vkDestroyCommandPool(device,commandPool,nullptr);
  vmaDestroyBuffer(allocator,vertexBuffer,vertexAllocation);
  vmaDestroyImage(allocator,framebuffer,framebufferAllocation);
  vmaDestroyAllocator(allocator);

  for(auto &shader:shaders){
    if(shader)
      pfDestroyShader(device,shader,nullptr);
  }
  vkDestroyDevice(device,nullptr);
  vkDestroyInstance(instance,nullptr);
  return 0;
}