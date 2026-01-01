#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 vColor;
layout(location = 1) out vec2 v_uv;

layout(set = 0, binding = 0) uniform CameraUBO {
  mat4 view;
  mat4 proj;
} camera;

// instance data
layout(set = 0, binding = 1) readonly buffer InstanceSSBO {
  mat4 model[];
} inst;

layout(push_constant) uniform Push {
  uint baseInstance;
} push;

void main() {
  uint idx = push.baseInstance + gl_InstanceIndex;
  mat4 M = inst.model[idx];

  gl_Position = camera.proj * camera.view * M * vec4(inPos, 1.0);
  v_uv = inUV;
  vColor = inColor;
}
