#ifndef COMMON_GLSL
#define COMMON_GLSL

const uint kMaxDirLights = 4u;

struct CameraUBO {
  mat4 view;
  mat4 proj;
  vec4 cameraPosWS_pad;
};

struct DebugUBO {
  uint view;
  uint flags;
  float value0;
  float value1;
};

struct DirectionalLight {
  vec4 directionWS_illuminanceLux; // xyz dir, w lux
  vec4 colorLinear_pad;  // rgb color
};

struct PointLight {
  vec3 positionWS; 
  float radius;

  vec3 colorLinear; 
  float lumens;
};

struct SceneUBO {
  CameraUBO camera;
  DebugUBO dbg;

  // TODO: pack into single vec
  uint pointLightCount;
  uint dirLightCount; 
  uint padA; 
  uint padB; 

  DirectionalLight dirLights[kMaxDirLights];
};

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

const uint kNoTex = 0xFFFFFFFFu;

#endif // COMMON_GLSL
