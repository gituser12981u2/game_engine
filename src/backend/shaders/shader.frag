#version 450

layout(set=1, binding=0) uniform sampler2D u_baseColor;

layout(location = 0) in vec3 v_color;
layout(location = 1) in vec2 v_uv;
layout(location = 2) flat in uint v_matId;

layout(location = 0) out vec4 outColor;

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

void main() {
  vec4 tex = texture(u_baseColor, v_uv);
  vec4 factor = mats.materials[v_matId].baseColorFactor;
  outColor = tex * factor;
}
