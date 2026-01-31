# AMDVKBug

This repository documents a series of driver-level issues discovered in the AMD Catalyst driver suite for Windows, specifically affecting the Vulkan graphics API implementation. These issues are related to the Shader Objects, Descriptor Buffers extensions, and dynamic rendering state.

### DescriptorBuffer.cpp

The kernel space driver (amdkmdag.sys) does not validate the the addresses used in descriptor buffers before the buffer is copied to hardware resulting in a recoverable crash of the driver. 
Shows a single example of the issue, other access violations can also happen when an invalid descriptor address is bound to the command buffer via `vkCmdBindDescriptorBuffersExt` or no descriptor buffer is bound at all. 

### VertexBinding.cpp

When using dynamic rendering state in a Vulkan command buffer, a memory access violation can occur in kernel space if a vertex buffer is not bound to the command buffer via the vkCmdBindVertexBuffers2

### LinkedShaderLayout.cpp

When using the shader object extension for Vulkan, the `vkCreateShadersEXT` function can cause a memory access violation in the amdvlk64.dll library. This occurs when there is a mismatch in descriptor resource layouts between shaders being linked or when the provided descriptor set layouts do not align with the shader's requirements. This is an interesting issue, the validation layer provided by LunarG for the Windows Vulkan SDK will catch this mismatch between graphic pipeline shaders during binding to the command buffer but not while creating the shader objects. 
