#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

#include "common.glsl"

const float PI = 3.14159265358979323846;

float saturate(float x) { return clamp(x, 0.0, 1.0); }

vec3 radianceDirectional(DirectionalLight Ld) {
  vec3 color = Ld.colorLinear_pad.rgb;
  float lux = Ld.directionWS_illuminanceLux.w;
  return color * lux;
}

vec3 directionToDirectionalLight(DirectionalLight Ld) {
  return normalize(-Ld.directionWS_illuminanceLux.xyz);
}

bool getPointLight(vec3 P, PointLight Lp, out vec3 L, out vec3 radiance) {
  vec3 toL = Lp.positionWS - P;
  float d2 = dot(toL, toL);
  float d = sqrt(d2);
  if (d <= 1e-4) return false;

  // Radius cutoff 
  float att = 1.0 - saturate(d / max(Lp.radius, 1e-4));
  att = att * att;

  L = toL / d;

  float inv4pi = 0.07957747155; // 1/(4*pi)
  float E = (Lp.lumens * inv4pi) / max(d2, 1e-4);

  radiance = (Lp.colorLinear * E) * att;
  return true;
}

float D_GGX(float NoH, float a2) {
  float denom = NoH * NoH * (a2 - 1.0) + 1.0;
  return a2 / max(PI * denom * denom, 1e-8);
}

float G_SchlickGGX(float NoX, float k) {
  return NoX / max(NoX * (1.0 - k) + k, 1e-8);
}

float G_Smith(float NoV, float NoL, float k) {
  return G_SchlickGGX(NoV, k) * G_SchlickGGX(NoL, k);
}

vec3 F_Schlick(vec3 F0, float VoH) {
  float f = pow(1.0 - VoH, 5.0);
  return F0 + (1.0 - F0) * f;
}

vec3 evalDirectBRDF(vec3 N, vec3 V, vec3 L, vec3 radiance, vec3 albedo, float metallic, float roughness) {
  float NoV = saturate(dot(N, V));
  float NoL = saturate(dot(N, L));
  if (NoV <= 1e-4 || NoL <= 1e-4) return vec3(0.0);

  vec3 H = normalize(V + L);

  float NoH = saturate(dot(N, H));
  float VoH = saturate(dot(V, H));
  
  // Perceptual roughness -> alpha
  float r = clamp(roughness, 0.04, 1.0);
  float a = r * r;
  float a2 = a * a;

  // Fresnel at normal incidence
  vec3 F0 = mix(vec3(0.04), albedo, metallic);

  vec3 F = F_Schlick(F0, VoH);
  float D = D_GGX(NoH, a2);

  float k = (r + 1.0);
  k = (k * k) * 0.125; // /8 
  float G = G_Smith(NoV, NoL, k);

  vec3 spec = (D * G) * F / max(4.0 * NoV * NoL, 1e-4);

  // Diffuse with lambert, energy compensated
  vec3 kd = (vec3(1.0) - F) * (1.0 - metallic);
  vec3 diff = kd * albedo * (1.0 / PI);

  return (diff + spec) * radiance * NoL;
}

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

void main() {
  uint f = push.frameIndex;
  DebugUBO dbg = g_scene[nonuniformEXT(f)].scene.dbg;

  Material m = mats.materials[v_matId];

  // BaseColor
  vec4 baseTex = sampleTex(m.tex0.x, v_uv, vec4(1.0));
  vec4 base = baseTex * m.baseColorFactor;

  // MetallicRoughness 
  vec4 mrTex = sampleTex(m.tex0.z, v_uv, vec4(0.0, 1.0, 0.0, 1.0));
  float roughness = clamp(m.mrAoAlpha.y * mrTex.g, 0.001, 1.0);
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

  // Directional: from point -> camera
  SceneUBO s = g_scene[nonuniformEXT(f)].scene;
  vec3 camPos = s.camera.cameraPosWS_pad.xyz;
  vec3 V = normalize(camPos - v_worldPos);

  vec3 albedo = base.rgb;

  vec3 lit = vec3(0.0);

  // Directional lights
  uint dlCount = min(s.dirLightCount, kMaxDirLights);
  for (uint i = 0u; i < dlCount; ++i) {
    DirectionalLight Ld = s.dirLights[i];
    vec3 L = directionToDirectionalLight(Ld);
    vec3 radiance = radianceDirectional(Ld);
    lit += evalDirectBRDF(N, V, L, radiance, albedo, metallic, roughness);
  }

  // Point lights
  uint plCount = s.pointLightCount;
  for (uint i = 0u; i < plCount; ++i) {
    PointLight Lp = g_pointLights[nonuniformEXT(f)].lights[i];
    vec3 L;
    vec3 radiance;
    if (getPointLight(v_worldPos, Lp, L, radiance)) {
      lit += evalDirectBRDF(N, V, L, radiance, albedo, metallic, roughness);
    }
  }

  // TODO: remove when IBL is made
  vec3 up = vec3(0.0, 0.0, 1.0);

  float hemi = 0.5 + 0.5 * dot(N, up);
  vec3 sky = vec3(0.04);
  vec3 ground = vec3(0.01);

  vec3 F0 = mix(vec3(0.04), albedo, metallic);
  vec3 F = F_Schlick(F0, saturate(dot(N, V)));
  vec3 kd = (vec3(1.0) - F) * (1.0 - metallic);

  vec3 ambient = kd * albedo * mix(ground, sky, hemi);
  lit += ambient;

  lit *= ao;

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
  float exposure = 1.0 / 1.0;
  vec3 colorOut = 1.0 - exp(-(lit + emissive) * exposure);

  outColor = vec4(colorOut, alpha);
}
