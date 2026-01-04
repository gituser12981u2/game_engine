#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 vColor;
layout(location = 1) out vec2 v_uv;
layout(location = 2) out uint v_matId;

layout(set = 0, binding = 0) uniform CameraUBO {
  mat4 view;
  mat4 proj;
} camera;

// instance data
layout(set = 0, binding = 1) readonly buffer InstanceSSBO {
  mat4 model[];
} inst;

// Material table
// 80 bytes per material
struct Material {
  vec4 baseColorFactor; // rgba
  vec4 emissiveFactor;  // rgb + pad

  // x=metallic, y=roughness, z=aoStrength, w=alphaCutoff
  vec4 mrAoAlpha;

  // Texture indices
  uvec4 tex0; // x=baseColor, y=normal, z=metalRough, w=occlusion
  uvec4 tex1; // x=emissive, y=reserved, z=reserved, w=reserved

  // flags: bits for alphaMode, doubleSided, etc.
  uvec4 flags;
};

layout(set = 0, binding = 2, std430) readonly buffer MaterialSSBO {
  Material materials[];
} mats;

layout(push_constant) uniform Push {
  uint baseInstance;
  uint materialId;
} push;

void main() {
  uint idx = push.baseInstance + gl_InstanceIndex;
  mat4 M = inst.model[idx];

  gl_Position = camera.proj * camera.view * M * vec4(inPos, 1.0);
  v_uv = inUV;
  vColor = inColor;
  v_matId = push.materialId;
}
