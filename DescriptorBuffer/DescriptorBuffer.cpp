#include<vector>
#include<array>
#include<filesystem>
#include<fstream>
#include<iostream>

#include<vulkan\vulkan.h>
#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include<vma/vk_mem_alloc.h>

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

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
  VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT messageType,
  const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
  void *pUserData){

  std::cerr<<"Validation Layer: "<<pCallbackData->pMessage<<std::endl;

  return VK_FALSE; // Return VK_TRUE to terminate the application
}

int main(){
  //************** Instance ***********************
#pragma region Instance
  std::array<const char *,0> InstanceLayers;//={"VK_LAYER_KHRONOS_validation"};
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
  PFN_vkCreateShadersEXT pfCreateShader=reinterpret_cast<PFN_vkCreateShadersEXT>(
    vkGetDeviceProcAddr(device,"vkCreateShadersEXT"));
  PFN_vkDestroyShaderEXT pfDestroyShader=reinterpret_cast<PFN_vkDestroyShaderEXT>(
    vkGetDeviceProcAddr(device,"vkDestroyShaderEXT"));

  PFN_vkCmdBindShadersEXT pfCmdBindShaders=reinterpret_cast<PFN_vkCmdBindShadersEXT>(
    vkGetDeviceProcAddr(device,"vkCmdBindShadersEXT"));

  PFN_vkGetDescriptorSetLayoutSizeEXT pfGetDescriptorSetLayoutSize=reinterpret_cast<PFN_vkGetDescriptorSetLayoutSizeEXT>(
    vkGetDeviceProcAddr(device,"vkGetDescriptorSetLayoutSizeEXT"));

  PFN_vkGetDescriptorSetLayoutBindingOffsetEXT pfGetDescriptorSetLayoutBindingOffset=
    reinterpret_cast<PFN_vkGetDescriptorSetLayoutBindingOffsetEXT>(
      vkGetDeviceProcAddr(device,"vkGetDescriptorSetLayoutBindingOffsetEXT"));

  PFN_vkCmdBindDescriptorBuffersEXT pfnCmdBindDescriptorBuffersEXT=
    reinterpret_cast<PFN_vkCmdBindDescriptorBuffersEXT>(
    vkGetDeviceProcAddr(device,"vkCmdBindDescriptorBuffersEXT"));

  PFN_vkGetDescriptorEXT pfnGetDescriptorEXT=
    reinterpret_cast<PFN_vkGetDescriptorEXT>(
    vkGetDeviceProcAddr(device,"vkGetDescriptorEXT"));

  PFN_vkCmdSetDescriptorBufferOffsetsEXT pfnCmdSetDescriptorBufferOffsetsEXT=
    reinterpret_cast<PFN_vkCmdSetDescriptorBufferOffsetsEXT>(
      vkGetDeviceProcAddr(device,"vkCmdSetDescriptorBufferOffsetsEXT"));

  VkPhysicalDeviceDescriptorBufferPropertiesEXT DescriptorBufferProperties={
    .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT
  };
  VkPhysicalDeviceProperties2 DeviceProperties={
    .sType=VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
    .pNext=&DescriptorBufferProperties
  };
#pragma endregion

  //*************** Layout ************************
#pragma region Layout
  vkGetPhysicalDeviceProperties2(physicalDevices[0],&DeviceProperties);


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

  VkDescriptorSetLayoutBinding bindingInput={
    .binding=0,
    .descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    .descriptorCount=1,
    .stageFlags=VK_SHADER_STAGE_COMPUTE_BIT,
    .pImmutableSamplers=nullptr
  };
  VkDescriptorSetLayoutBinding bindingOutput={
    .binding=1,
    .descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    .descriptorCount=1,
    .stageFlags=VK_SHADER_STAGE_COMPUTE_BIT,
    .pImmutableSamplers=nullptr
  };

  auto setLayout=CreateSetLayout({bindingInput,bindingOutput});
#pragma endregion

  //*************** Shader ************************
#pragma region Shader
  std::vector<VkDescriptorSetLayout> computeLayout={setLayout};
  auto computeShaderCode=LoadShader("Shaders/comp.spv");


  std::vector<VkShaderCreateInfoEXT> ShaderCreateInfos={{
    .sType=VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT,
    .pNext=nullptr,
    .flags=0,
    .stage=VK_SHADER_STAGE_COMPUTE_BIT,
    .nextStage=0,
    .codeType=VK_SHADER_CODE_TYPE_SPIRV_EXT,
    .codeSize=(uint32_t)computeShaderCode.size()*sizeof(uint32_t),
    .pCode=computeShaderCode.data(),
    .pName="main",
    .setLayoutCount=(uint32_t)computeLayout.size(),
    .pSetLayouts=computeLayout.data(),
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
  auto GetBufferAddress=[device](VkBuffer buffer)->auto{
    VkBufferDeviceAddressInfo bufferDeviceAddressInfo={
     .sType=VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
     .pNext=nullptr,
     .buffer=buffer
    };

    return vkGetBufferDeviceAddress(device,&bufferDeviceAddressInfo);
  };

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


  VkDeviceSize descriptorSize=0;
  pfGetDescriptorSetLayoutSize(device,setLayout,&descriptorSize);

  VmaAllocationInfo descriptorAllocationInfo={};
  VkBuffer descriptorBuffer=nullptr;
  VmaAllocation descriptorBufferAllocation=nullptr;

  VkDeviceSize inputSize=2048;
  VmaAllocationInfo inputAllocationInfo={};
  VkBuffer inputBuffer=nullptr;
  VmaAllocation inputBufferAllocation=nullptr;

  VkDeviceSize outputSize=4096;
  VmaAllocationInfo outputAllocationInfo={};
  VkBuffer outputBuffer=nullptr;
  VmaAllocation outputBufferAllocation=nullptr;

  VkBufferCreateInfo bufferInfo={
    .sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .pNext=nullptr,
    .flags=0,
    .size=descriptorSize*1024,
    .usage=VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT|VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT,
    .sharingMode=VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount=0,
    .pQueueFamilyIndices=nullptr
  };

  VmaAllocationCreateInfo allocateInfo={
    .flags=VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT|VMA_ALLOCATION_CREATE_MAPPED_BIT,
    .usage=VMA_MEMORY_USAGE_AUTO,
    .requiredFlags=VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    .preferredFlags=VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    .memoryTypeBits=0,
    .pool=nullptr,
    .pUserData=nullptr,
    .priority=0.0f
  };

  result=vmaCreateBuffer(allocator,&bufferInfo,&allocateInfo,&descriptorBuffer,&descriptorBufferAllocation,&descriptorAllocationInfo);
  if(result!=VK_SUCCESS)
    throw std::runtime_error("Failed to create buffer memory");

  bufferInfo.usage=VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT|VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  bufferInfo.size=inputSize;
  result=vmaCreateBuffer(allocator,&bufferInfo,&allocateInfo,&inputBuffer,&inputBufferAllocation,&inputAllocationInfo);
  if(result!=VK_SUCCESS)
    throw std::runtime_error("Failed to create buffer memory");

  bufferInfo.size=outputSize;
  result=vmaCreateBuffer(allocator,&bufferInfo,&allocateInfo,&outputBuffer,&outputBufferAllocation,&outputAllocationInfo);
  if(result!=VK_SUCCESS)
    throw std::runtime_error("Failed to create buffer memory"); 
#pragma endregion

  //************** Pipeline ***********************
#pragma region Pipeline
  auto GetDescriptor=[device,pfnGetDescriptorEXT,DescriptorBufferProperties,descriptorAllocationInfo](VkDeviceAddress address,VkDeviceSize size,VkDeviceSize offset){
    VkDescriptorAddressInfoEXT inputAddressInfo={
      .sType=VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
      .pNext=nullptr,
      .address=address,
      .range=size,
      .format=VK_FORMAT_UNDEFINED
    };

    VkDescriptorGetInfoEXT InputDescriptorGetInfo={
      .sType=VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
      .pNext=nullptr,
      .type=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .data={.pStorageBuffer=&inputAddressInfo}
    };

    pfnGetDescriptorEXT(
      device,&InputDescriptorGetInfo,
      DescriptorBufferProperties.storageBufferDescriptorSize,
      (uint8_t *)descriptorAllocationInfo.pMappedData+offset);
  };
  VkDeviceSize bindingOffset=0;
  pfGetDescriptorSetLayoutBindingOffset(device,setLayout,0,&bindingOffset);

  //CRASH HERE - an invalid offset causes a access violation in the AMD driver
  //This is another soft crash that the drivers will recover but can 
  //cause context losses a cross the board. 
  GetDescriptor(GetBufferAddress(inputBuffer)/*+(32*1024*1024)*/,inputSize,bindingOffset);
  std::cout<<std::format("Binding 0 Offset {}\n",bindingOffset);

  pfGetDescriptorSetLayoutBindingOffset(device,setLayout,1,&bindingOffset);
  GetDescriptor(GetBufferAddress(outputBuffer),outputSize,bindingOffset);
  std::cout<<std::format("Binding 1 Offset {}\n",bindingOffset);

  VkPipelineLayout pipelineLayout=nullptr;
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

  /*******************************
   *                             *
   *  Command Buffer Operations  *
   *                             *
   *******************************/

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
  vkBeginCommandBuffer(CMDBuffer,&bufferBeginInfo);

  VkBufferDeviceAddressInfo bufferDeviceAddressInfo={
    .sType=VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
    .pNext=nullptr,
    .buffer=descriptorBuffer
  };

  auto descriptorBufferAddress=GetBufferAddress(descriptorBuffer);

  VkShaderStageFlagBits stageFlags=VK_SHADER_STAGE_COMPUTE_BIT;
  pfCmdBindShaders(CMDBuffer,(uint32_t)shaders.size(),&stageFlags,shaders.data());
   
  VkDescriptorBufferBindingInfoEXT bufferBindingInfo={
    .sType=VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT,
    .pNext=nullptr,
    .address=descriptorBufferAddress,
    .usage=VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT
  };
  pfnCmdBindDescriptorBuffersEXT(CMDBuffer,1,&bufferBindingInfo);

  uint32_t bufferIndice=0;
  VkDeviceSize bufferOffset=0;

  pfnCmdSetDescriptorBufferOffsetsEXT(CMDBuffer,VK_PIPELINE_BIND_POINT_COMPUTE,pipelineLayout,0,1,&bufferIndice,&bufferOffset);
  vkCmdDispatch(CMDBuffer,1,1,1);

  vkEndCommandBuffer(CMDBuffer);
  VkQueue queue=nullptr;
  vkGetDeviceQueue(device,0,0,&queue);
  
  auto input=reinterpret_cast<float *>(inputAllocationInfo.pMappedData);
  input[0]=2.5f;
  input[1]=3.5f;
  input[2]=4.5f;
  input[3]=5.5f;

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
  vmaDestroyBuffer(allocator,descriptorBuffer,descriptorBufferAllocation);
  vmaDestroyBuffer(allocator,inputBuffer,inputBufferAllocation);
  vmaDestroyBuffer(allocator,outputBuffer,outputBufferAllocation);
  vmaDestroyAllocator(allocator);

  for(auto &shader:shaders)
    pfDestroyShader(device,shader,nullptr);

  vkDestroyDevice(device,nullptr);
  vkDestroyInstance(instance,nullptr);
  return 0;
}