#version 450

layout(local_size_x = 1) in;

layout(std430, set = 0, binding = 0) buffer IN1{
  vec4 value;
} in1;
layout(std430, set = 0, binding = 1) buffer OUT1{
  vec4 value;
} out1;

void main(){
  out1.value = in1.value;
}