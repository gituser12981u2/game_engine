#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 1, binding = 0) uniform sampler2D g_tex[];

layout(location = 0) in vec3 vColor;
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

layout(set = 0, binding = 3) uniform DebugUBO {
  uint view;
  uint flags;
  float value0;
  float value1;
} dbg;

const uint kNoTex = 0xFFFFFFFFu;

vec4 sampleTex(uint idx, vec2 uv, vec4 fallback) {
  if (idx == kNoTex) return fallback;
  return texture(g_tex[nonuniformEXT(idx)], uv);
}

void main() {
  Material m = mats.materials[v_matId];

  // BaseColor
  vec4 baseTex = sampleTex(m.tex0.x, v_uv, vec4(1.0));
  vec4 base = baseTex * m.baseColorFactor;

  // MetallicRoughness 
  vec4 mrTex = sampleTex(m.tex0.z, v_uv, vec4(0.0, 1.0, 0.0, 1.0));
  float roughness = clamp(m.mrAoAlpha.y * mrTex.g, 0.04, 1.0);
  float metallic = clamp(m.mrAoAlpha.x * mrTex.b, 0.0, 1.0);

  // Occlusion (linear) 
  vec4 aoTex = sampleTex(m.tex0.w, v_uv, vec4(1.0));
  float ao = mix(1.0, aoTex.r, clamp(m.mrAoAlpha.z, 0.0, 1.0));

  // Emissive (sRGB)
  vec3 emissiveTex = sampleTex(m.tex1.x, v_uv, vec4(0.0, 0.0, 0.0, 1.0)).rgb;
  vec3 emissive = emissiveTex * m.emissiveFactor.rgb;

  // Alpha 
  float alpha = base.a;
  uint alphaMode = (m.flags.x & 0x3u); // 0 opaque, 1 mask, 2 blend
  if (alphaMode == 0u) alpha = 1.0;
  else if (alphaMode == 1u) {
    if (alpha < m.mrAoAlpha.w) discard;
    alpha = 1.0;
  }

  if (dbg.view == 1u) { outColor = vec4(base.rgb, 1.0); return; }
  if (dbg.view == 2u) { outColor = vec4(vec3(metallic), 1.0); return; }
  if (dbg.view == 3u) { outColor = vec4(vec3(roughness), 1.0); return; }
  if (dbg.view == 4u) { outColor = vec4(vec3(ao), 1.0); return; }
  if (dbg.view == 5u) { outColor = vec4(emissive, 1.0); return; }
  if (dbg.view == 6u) { outColor = vec4(vColor, 1.0); return; } // vertex color
  if (dbg.view == 7u) { outColor = vec4(fract(v_uv), 0.0, 1.0); return; }

  outColor = vec4(base.rgb * ao + emissive, alpha);
}
