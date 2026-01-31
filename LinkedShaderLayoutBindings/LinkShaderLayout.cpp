#include<stdexcept>
#include<array>
#include<vector>
#include<filesystem>
#include<fstream>
#include<vulkan/vulkan.h>


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
  std::array<const char *,0> InstanceLayers;//={"VK_LAYER_KHRONOS_validation"};
  std::array<const char *,0> InstanceExtensions={};
  std::array<const char *,3> DeviceExtensions={
    VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME,VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
    VK_EXT_SHADER_OBJECT_EXTENSION_NAME};
  std::vector<VkShaderCreateInfoEXT> ShaderCreateInfos;

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
    .enabledExtensionCount=0,
    .ppEnabledExtensionNames=nullptr
  };
  VkInstance instance=nullptr;
  VkResult result=vkCreateInstance(&instanceInfo,nullptr,&instance);
  if(result!=VK_SUCCESS)
    throw std::runtime_error("Failed to create instance");

#pragma endregion

  //*************** Device ************************
#pragma region Device
  std::vector<VkPhysicalDevice> physicalDevices;
  uint32_t physicalDeviceCount=0;
  vkEnumeratePhysicalDevices(instance,&physicalDeviceCount,nullptr);
  physicalDevices.resize(physicalDeviceCount);
  vkEnumeratePhysicalDevices(instance,&physicalDeviceCount,physicalDevices.data());

  if(physicalDeviceCount<1)
    throw std::runtime_error("Unable to find graphics device");

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
#pragma endregion

  //*************** Layout ************************
#pragma region Layout
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

  VkDescriptorSetLayoutBinding bindingShared={
    .binding=0,
    .descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount=1,
    .stageFlags=VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT,
    .pImmutableSamplers=nullptr
  };

  VkDescriptorSetLayoutBinding bindingVertex={
  .binding=0,
  .descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
  .descriptorCount=1,
  .stageFlags=VK_SHADER_STAGE_VERTEX_BIT,
  .pImmutableSamplers=nullptr
  };

  VkDescriptorSetLayoutBinding bindingFragmentBuffer={
  .binding=0,
  .descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
  .descriptorCount=1,
  .stageFlags=VK_SHADER_STAGE_FRAGMENT_BIT,
  .pImmutableSamplers=nullptr
  };

  VkDescriptorSetLayoutBinding bindingFragmentTexture={
    .binding=1,
    .descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .descriptorCount=1,
    .stageFlags=VK_SHADER_STAGE_FRAGMENT_BIT,
    .pImmutableSamplers=nullptr
  };

  VkDescriptorSetLayout descriptorSetSharedLayout=CreateSetLayout({bindingShared});
  VkDescriptorSetLayout descriptorSetVertexLayout=CreateSetLayout({bindingVertex});
  VkDescriptorSetLayout descriptorSetFragmentLayout=CreateSetLayout({bindingFragmentBuffer,bindingFragmentTexture});
#pragma endregion

  //*************** Shader ************************
#pragma region Shader
  auto vertexShaderCode=LoadShader("LinkedShaderLayoutVert.spv");
  auto fragmentShaderCode=LoadShader("LinkedShaderLayoutFrag.spv");

  //Userland AMD driver amdvlk64.dll will crash because of memory access violation 
  //The crash is assocated with a missmatch between the descriptor resources between 
  //the shaders
  //This is a very fragile function to call. 
  std::vector<VkDescriptorSetLayout> vetexLayout={descriptorSetVertexLayout,descriptorSetSharedLayout};
  std::vector<VkDescriptorSetLayout> fragmentLayout={descriptorSetFragmentLayout,descriptorSetSharedLayout};

  const bool useLinkStages=1;

  ShaderCreateInfos.push_back({
    .sType=VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT,
    .pNext=nullptr,
    .flags=useLinkStages?VK_SHADER_CREATE_LINK_STAGE_BIT_EXT:0,
    .stage=VK_SHADER_STAGE_VERTEX_BIT,
    .nextStage=VK_SHADER_STAGE_FRAGMENT_BIT,
    .codeType=VK_SHADER_CODE_TYPE_SPIRV_EXT,
    .codeSize=(uint32_t)vertexShaderCode.size()*sizeof(uint32_t),
    .pCode=vertexShaderCode.data(),
    .pName="main",
    .setLayoutCount=(uint32_t)vetexLayout.size(),
    .pSetLayouts=vetexLayout.data(),
    .pushConstantRangeCount=0,
    .pPushConstantRanges=nullptr,
    .pSpecializationInfo=nullptr
  });

  ShaderCreateInfos.push_back({
    .sType=VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT,
    .pNext=nullptr,
    .flags=useLinkStages?VK_SHADER_CREATE_LINK_STAGE_BIT_EXT:0,
    .stage=VK_SHADER_STAGE_FRAGMENT_BIT,
    .nextStage=0,
    .codeType=VK_SHADER_CODE_TYPE_SPIRV_EXT,
    .codeSize=(uint32_t)fragmentShaderCode.size()*sizeof(uint32_t),
    .pCode=fragmentShaderCode.data(),
    .pName="main",
    .setLayoutCount=(uint32_t)fragmentLayout.size(),
    .pSetLayouts=fragmentLayout.data(),
    .pushConstantRangeCount=0,
    .pPushConstantRanges=nullptr,
    .pSpecializationInfo=nullptr
  });

  std::vector<VkShaderEXT> shaders(ShaderCreateInfos.size(),nullptr);
  result=pfCreateShader(device,(uint32_t)ShaderCreateInfos.size(),ShaderCreateInfos.data(),nullptr,shaders.data());
  if(result!=VK_SUCCESS)
    throw std::runtime_error("Failed to create shader objects");

#pragma endregion

  for(auto &shader:shaders)
    pfDestroyShader(device,shader,nullptr);

  ShaderCreateInfos.clear();
  vkDestroyDescriptorSetLayout(device,descriptorSetSharedLayout,nullptr);
  vkDestroyDescriptorSetLayout(device,descriptorSetVertexLayout,nullptr);
  vkDestroyDescriptorSetLayout(device,descriptorSetFragmentLayout,nullptr);
  vetexLayout.clear();
  fragmentLayout.clear();
  vertexShaderCode.clear();
  fragmentShaderCode.clear();

  vkDestroyDevice(device,nullptr);
  vkDestroyInstance(instance,nullptr);

  return 0;
}