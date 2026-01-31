#version 450

// Input attributes from vertex buffer
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 normal;

// Output to fragment shader
layout(location = 3) out vec2 fragTexCoord;
layout(location = 4) out vec3 fragNormal;
layout(location = 5) out vec3 fragWorldPos;

// Uniforms for transformation matrices
layout(set=0, binding=0) uniform SceneData {
  mat4 modelMatrix;
  mat4 viewMatrix;
  mat4 projectionMatrix;
} sceneUBO;

layout(set=1,binding=0) uniform Shared{
  vec3 simpleVex;
} sharedUBO;


void main(){
  // Transform position to world space
  vec4 worldPos = sceneUBO.modelMatrix * vec4(position, 1.0);
    
  // Transform position to clip space
  gl_Position = sceneUBO.projectionMatrix * sceneUBO.viewMatrix * worldPos;
    
  // Pass through texture coordinates
  fragTexCoord = texCoord * sharedUBO.simpleVex.xy;
  
  // Transform normal to world space
  fragNormal = mat3(sceneUBO.modelMatrix) * normal;
    
  // Pass world position
  fragWorldPos = worldPos.xyz;
}

