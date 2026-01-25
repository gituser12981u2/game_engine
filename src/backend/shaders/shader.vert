#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#include "common.glsl"

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inUV;

layout(location = 0) out vec3 vColor;
layout(location = 1) out vec2 v_uv;
layout(location = 2) flat out uint v_matId;
layout(location = 3) out vec3 v_worldPos;
layout(location = 4) out vec3 v_worldN;

layout(set = 0, binding = 0, std140) uniform SceneSet {
  SceneUBO scene;
} g_scene[];

// instance data
layout(set = 0, binding = 1, std430) readonly buffer InstanceSSBO {
  mat4 model[];
} g_inst[];

layout(set = 0, binding = 2, std430) readonly buffer MaterialSSBO {
  Material materials[];
} mats;

layout(push_constant) uniform Push {
  uint frameIndex;
  uint baseInstance;
  uint materialId;
} push;

void main() {
  uint f = push.frameIndex;
  uint idx = push.baseInstance + gl_InstanceIndex;

  mat4 M = g_inst[nonuniformEXT(f)].model[idx];
  SceneUBO s = g_scene[nonuniformEXT(f)].scene;

  vec4 wp = M * vec4(inPos, 1.0);
  v_worldPos = wp.xyz;

  mat3 Nmat = transpose(inverse(mat3(M)));
  v_worldN = normalize(Nmat * inNormal);

  gl_Position = s.camera.proj * s.camera.view * wp;

  v_uv = inUV;
  vColor = inColor;
  v_matId = push.materialId;
}
