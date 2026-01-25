#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#include "common.glsl"

layout(set = 1, binding = 0) uniform sampler2D g_tex[];

layout(location = 0) in vec3 vColor;
layout(location = 1) in vec2 v_uv;
layout(location = 2) flat in uint v_matId;
layout(location = 3) in vec3 v_worldPos;
layout(location = 4) in vec3 v_worldN;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0, std140) uniform SceneSet {
  SceneUBO scene;
} g_scene[];

layout(set = 0, binding = 2, std430) readonly buffer MaterialSSBO {
  Material materials[];
} mats;

layout(set = 0, binding = 3, std430) readonly buffer PointLightSSBO {
  PointLight lights[];
} g_pointLights[];

layout(push_constant) uniform Push {
  uint frameIndex;
  uint baseInstance;
  uint materialId;
} push;

vec4 sampleTex(uint idx, vec2 uv, vec4 fallback) {
  if (idx == kNoTex) return fallback;
  return texture(g_tex[nonuniformEXT(idx)], uv);
}

float saturate(float x) { return clamp(x, 0.0, 1.0); }

// Lambert for directional lights
vec3 evalLambertDir(vec3 N, DirectionalLight Ld, vec3 albedoLinear) {
  vec3 L = normalize(-Ld.directionWS_illuminanceLux.xyz);
  float lux = Ld.directionWS_illuminanceLux.w;
  vec3 color = Ld.colorLinear_pad.rgb;
  float NoL = saturate(dot(N, L));
  vec3 E = color * lux;
  return albedoLinear * (E * NoL);
}

vec3 evalLambertPoint(vec3 P, vec3 N, PointLight Lp, vec3 albedoLinear) {
  vec3 toL = Lp.positionWS - P;
  float d2 = dot(toL, toL);
  float d = sqrt(d2);
  if (d <= 1e-4) {
    return vec3(0.0);
  }

  // Radius cutoff
  float att = 1.0 - saturate(d / max(Lp.radius, 1e-4));
  att = att * att;

  vec3 L = toL / d;
  float NoL = saturate(dot(N, L));

  // point light luminous flux (lumens) spread over sphere: E ~ lumens / (4*pi*r^2)
  float inv4pi = 0.0795774715; // 1/(4*pi)
  float E = (Lp.lumens * inv4pi) / max(d2, 1e-4);

  vec3 radiance = Lp.colorLinear * E;
  return albedoLinear * (radiance * NoL) * att;
}

void main() {
  uint f = push.frameIndex;
  DebugUBO dbg = g_scene[nonuniformEXT(f)].scene.dbg;

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

  vec3 N = normalize(v_worldN);
  vec3 albedo = base.rgb;
  vec3 lit = vec3(0.0);

  // Directional
  SceneUBO s = g_scene[nonuniformEXT(f)].scene;
  uint dlCount = min(s.dirLightCount, kMaxDirLights);
  for (uint i = 0u; i < dlCount; ++i) {
    lit += evalLambertDir(N, s.dirLights[i], albedo);
  }

  // Point lights
  uint plCount = s.pointLightCount;
  for (uint i = 0u; i < plCount; ++i) {
    PointLight Lp = g_pointLights[nonuniformEXT(f)].lights[i];
    lit += evalLambertPoint(v_worldPos, N, Lp, albedo);
  }

  // TODO: remove when IBL is made
  vec3 up = vec3(0.0, 0.0, 1.0);
  float hemi = 0.5 + 0.5 * dot(N, up);
  vec3 sky = vec3(0.04);
  vec3 ground = vec3(0.01);
  vec3 ambient = albedo * mix(ground, sky, hemi);
  lit += ambient;

  lit *= ao;
  vec3 colotOut = lit + emissive;

  if (dbg.view == 1u) { outColor = vec4(base.rgb, 1.0); return; }
  if (dbg.view == 2u) { outColor = vec4(vec3(metallic), 1.0); return; }
  if (dbg.view == 3u) { outColor = vec4(vec3(roughness), 1.0); return; }
  if (dbg.view == 4u) { outColor = vec4(vec3(ao), 1.0); return; }
  if (dbg.view == 5u) { outColor = vec4(emissive, 1.0); return; }
  if (dbg.view == 6u) { outColor = vec4(vColor, 1.0); return; } // vertex color
  if (dbg.view == 7u) { outColor = vec4(fract(v_uv), 0.0, 1.0); return; }
  if (dbg.view == 8u) { outColor = vec4(normalize(v_worldN) * 0.5 + 0.5, 1.0); return; }
  if (dbg.view == 9u) { outColor = vec4(fract(v_worldPos * 0.1), 1.0); return; }
  if (dbg.view == 10u) { outColor = vec4(lit, 1.0); return; }


  // TODO: remove after HDR + tonemapping
  float exposure = 1.0 / 10000.0;
  vec3 colorOut = 1.0 - exp(-lit * exposure);

  outColor = vec4(colotOut, alpha);
}
