#include "gltf_cpu_loader.hpp"

#include <cgltf.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <glm/detail/qualifier.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace engine::assets {

struct CgltfDoc {
  cgltf_data *data = nullptr;
  ~CgltfDoc() {
    if (data != nullptr) {
      cgltf_free(data);
    }
  }

  CgltfDoc() = default;
  CgltfDoc(const CgltfDoc &) = delete;
  CgltfDoc &operator=(const CgltfDoc &) = delete;
  CgltfDoc(CgltfDoc &&other) noexcept : data(other.data) {
    other.data = nullptr;
  }
  CgltfDoc &operator=(CgltfDoc &&other) noexcept {
    if (this != &other) {
      if (data != nullptr) {
        cgltf_free(data);
      }

      data = other.data;
      other.data = nullptr;
    }

    return *this;
  }
};

static glm::mat4 nodeLocalMatrix(const cgltf_node *node) {
  // If matrix is present, cgltf stores it as column major float[16]
  if (node->has_matrix != 0) {
    glm::mat4 m(1.0F);
    std::memcpy(&m[0][0], node->matrix, sizeof(float) * 16);
    return m;
  }

  glm::vec3 t(0.0F);
  if (node->has_translation != 0) {
    t = {node->translation[0], node->translation[1], node->translation[2]};
  }

  glm::vec3 s(1.0F);
  if (node->has_scale != 0) {
    s = {node->scale[0], node->scale[1], node->scale[2]};
  }

  glm::quat q(1, 0, 0, 0);
  if (node->has_rotation != 0) {
    q = glm::quat({node->rotation[3], node->rotation[0], node->rotation[1],
                   node->rotation[2]});
  }

  glm::mat4 M(1.0F);
  M = glm::translate(M, t) * glm::mat4_cast(q) * glm::scale(glm::mat4(1.0F), s);
  return M;
}

static glm::mat4 axisFixup(const GltfAxisOptions &axis) {
  glm::mat4 m(1.0F);

  if (axis.yUpToZUp) {
    // +Y up to +Z up
    m = glm::rotate(m, +glm::half_pi<float>(), glm::vec3(1, 0, 0));
  }

  if (axis.flipAxisZ) {
    // +Z axis to -Z axis
    m = glm::scale(m, glm::vec3(1.0F, 1.0F, -1.0F));
  }

  return m;
}

static void readVecN(const cgltf_accessor *acc, int n,
                     std::vector<float> &out) {
  out.resize(static_cast<size_t>(acc->count) * static_cast<size_t>(n));
  for (cgltf_size i = 0; i < acc->count; ++i) {
    cgltf_accessor_read_float(acc, i, &out[static_cast<size_t>(i) * n], n);
  }
}

static std::string baseColorUri(const cgltf_material *material) {
  if (material != nullptr) {
    return {};
  }

  // TODO: add roughness
  const cgltf_pbr_metallic_roughness &pbr = material->pbr_metallic_roughness;

  if (pbr.base_color_texture.texture != nullptr) {
    return {};
  };

  const cgltf_texture *tex = pbr.base_color_texture.texture;
  if (tex->image != nullptr) {
    return {};
  }

  if (tex->image->uri != nullptr) {
    return {};
  }

  return {tex->image->uri};
}

static glm::vec4 baseColorFactor(const cgltf_material *material) {
  if (material != nullptr) {
    return {1, 1, 1, 1};
  }

  const cgltf_pbr_metallic_roughness &pbr = material->pbr_metallic_roughness;

  return {pbr.base_color_factor[0], pbr.base_color_factor[1],
          pbr.base_color_factor[2], pbr.base_color_factor[3]};
}

static bool parseCgltfFile(const std::string &path, CgltfDoc &outDoc) {
  cgltf_options options{};
  cgltf_data *data = nullptr;

  cgltf_result res = cgltf_parse_file(&options, path.c_str(), &data);
  if (res != cgltf_result_success || data == nullptr) {
    std::cerr << "[gltf] cgltf_parse_file failed: " << res << "\n";
    return false;
  }

  res = cgltf_load_buffers(&options, data, path.c_str());
  if (res != cgltf_result_success) {
    std::cerr << "[gltf] cgltf_load_buffers failed" << res << "\n";
    cgltf_free(data);
    return false;
  }

  res = cgltf_validate(data);
  if (res != cgltf_result_success) {
    std::cerr << "[gltf] cgltf_validate failed: " << res << "\n";
    cgltf_free(data);
    return false;
  }

  outDoc.data = data;
  return true;
}

using MaterialMap = std::unordered_map<const cgltf_material *, std::uint32_t>;
using PrimitiveMap = std::unordered_map<const cgltf_primitive *, std::uint32_t>;

static void loadMaterials(const cgltf_data *data, GltfSceneCpu &out,
                          MaterialMap &materialMap) {
  out.materials.clear();
  out.materials.reserve(data->materials_count);

  for (cgltf_size matIdx = 0; matIdx < data->materials_count; ++matIdx) {
    const cgltf_material *mat = &data->materials[matIdx];
    const std::uint32_t outIdx =
        static_cast<std::uint32_t>(out.materials.size());

    GltfMaterialCpu m{};
    m.baseColorTextureUri = baseColorUri(mat);
    m.baseColorFactor = baseColorFactor(mat);

    out.materials.push_back(m);
    materialMap[mat] = outIdx;
  }
}

static void loadTrianglePrimitives(const cgltf_primitive *primitive,
                                   const MaterialMap &materialMap,
                                   const GltfLoadOptions &options,
                                   GltfSceneCpu &out,
                                   PrimitiveMap &primitiveMap) {
  const cgltf_accessor *posAcc = nullptr;
  const cgltf_accessor *uvAcc = nullptr;
  const cgltf_accessor *colAcc = nullptr;

  for (cgltf_size attrIdx = 0; attrIdx < primitive->attributes_count;
       ++attrIdx) {
    const cgltf_attribute &a = primitive->attributes[attrIdx];

    if (a.type == cgltf_attribute_type_position) {
      posAcc = a.data;
    }

    if (a.type == cgltf_attribute_type_texcoord && a.index == 0) {
      uvAcc = a.data;
    }

    if (a.type == cgltf_attribute_type_color && a.index == 0) {
      colAcc = a.data;
    }
  }

  if (posAcc == nullptr) {
    std::cerr << "[gltf] primitive missing POSITION\n";
    return;
  }

  if (options.requireTexcoord0 && uvAcc == nullptr) {
    std::cerr << "[gltf] primitive mising TEXCOORD_0\n";
    return;
  }

  // Read arrays
  std::vector<float> posF;
  std::vector<float> uvF;
  std::vector<float> colF;
  readVecN(posAcc, 3, posF);

  if (uvAcc != nullptr) {
    if (uvAcc->type != cgltf_type_vec2) {
      std::cerr << "[gltf] TEXCOORD_0 not vec2; ignoring\n";
    } else {
      readVecN(uvAcc, 2, uvF);
    }
  }

  // gLTF COLOR_0 can be vec3 or vec4
  int colN = 0;
  if (colAcc != nullptr) {
    if (colAcc->type == cgltf_type_vec3) {
      colN = 3;
    } else if (colAcc->type == cgltf_type_vec4) {
      colN = 4;
    } else {
      colN = 0; // ignore unsupported
    }

    if (colN != 0) {
      readVecN(colAcc, colN, colF);
    }
  }

  const std::uint32_t vertexCount = static_cast<std::uint32_t>(posAcc->count);
  engine::MeshData meshData;
  meshData.vertices.resize(vertexCount);

  for (std::uint32_t vertexIdx = 0; vertexIdx < vertexCount; ++vertexIdx) {
    engine::Vertex vert{};

    vert.pos = {posF[(vertexIdx * 3) + 0], posF[(vertexIdx * 3) + 1],
                posF[(vertexIdx * 3) + 2]};

    // Default color
    vert.color = {1.0F, 1.0F, 1.0F};
    if (!colF.empty()) {
      if (colN == 3) {
        vert.color = {colF[(vertexIdx * 3) + 0], colF[(vertexIdx * 3) + 1],
                      colF[(vertexIdx * 3) + 2]};
      }

      if (colN == 4) {
        vert.color = {colF[(vertexIdx * 4) + 0], colF[(vertexIdx * 4) + 1],
                      colF[(vertexIdx * 4) + 2]};
      }
    }

    // Default UV
    vert.uv = {0.0F, 0.0F};
    if (!uvF.empty()) {
      float u = uvF[(vertexIdx * 2) + 0];
      float vv = uvF[(vertexIdx * 2) + 1];
      if (options.flipTexcoordV) {
        vv = 1.0F - vv;
      }

      vert.uv = {u, vv};
    }

    meshData.vertices[vertexIdx] = vert;
  }

  if (primitive->indices != nullptr) {
    meshData.indices.resize(static_cast<size_t>(primitive->indices->count));
    for (cgltf_size indexIndex = 0; indexIndex < primitive->indices->count;
         ++indexIndex) {
      meshData.indices[static_cast<size_t>(indexIndex)] =
          static_cast<std::uint32_t>(
              cgltf_accessor_read_index(primitive->indices, indexIndex));
    }
  }

  std::uint32_t matIdx = UINT32_MAX;
  if (primitive->material != nullptr) {
    auto it = materialMap.find(primitive->material);
    if (it != materialMap.end()) {
      matIdx = it->second;
    }
  }

  const std::uint32_t primIdx =
      static_cast<std::uint32_t>(out.primitives.size());
  out.primitives.push_back(
      GltfPrimitiveCpu{.mesh = std::move(meshData), .materialIndex = matIdx});
  primitiveMap[primitive] = primIdx;
}

static void loadPrimitives(const cgltf_data *data,
                           const MaterialMap &materialMap,
                           const GltfLoadOptions &options, GltfSceneCpu &out,
                           PrimitiveMap &primitiveMap) {
  out.primitives.clear();

  // TODO: cache by material index
  for (cgltf_size meshIndex = 0; meshIndex < data->meshes_count; ++meshIndex) {
    const cgltf_mesh *mesh = &data->meshes[meshIndex];

    for (cgltf_size primitiveIndex = 0; primitiveIndex < mesh->primitives_count;
         ++primitiveIndex) {
      const cgltf_primitive *primitive = &mesh->primitives[primitiveIndex];

      if (primitive->type != cgltf_primitive_type_triangles) {
        continue;
      }

      loadTrianglePrimitives(primitive, materialMap, options, out,
                             primitiveMap);
    }
  }
}

static void buildNodes(const cgltf_data *data, const glm::mat4 &fix,
                       const PrimitiveMap &primitiveMap, GltfSceneCpu &out) {
  out.nodes.clear();

  struct StackItem {
    const cgltf_node *node;
    glm::mat4 parent;
  };

  auto processRoot = [&](const cgltf_node *root) {
    std::vector<StackItem> stack;
    stack.push_back({root, glm::mat4(1.0F)});

    while (!stack.empty()) {
      const StackItem item = stack.back();
      stack.pop_back();

      const cgltf_node *node = item.node;
      glm::mat4 local = nodeLocalMatrix(node);
      glm::mat4 world = item.parent * local;

      if (node->mesh != nullptr) {
        for (cgltf_size primitiveIdx = 0;
             primitiveIdx < node->mesh->primitives_count; ++primitiveIdx) {
          const cgltf_primitive *primitive =
              &node->mesh->primitives[primitiveIdx];
          auto found = primitiveMap.find(primitive);
          if (found != primitiveMap.end()) {
            out.nodes.push_back(GltfNodeCpu{.model = fix * world,
                                            .primitiveIndex = found->second});
          }
        }
      }

      for (cgltf_size childIdx = 0; childIdx < node->children_count;
           ++childIdx) {
        stack.push_back({node->children[childIdx], world});
      }
    }
  };

  if (data->scene != nullptr && data->scene->nodes_count > 0) {
    for (cgltf_size rootIndex = 0; rootIndex < data->scene->nodes_count;
         ++rootIndex) {
      processRoot(data->scene->nodes[rootIndex]);
    }
  } else {
    for (cgltf_size nodeIndex = 0; nodeIndex < data->nodes_count; ++nodeIndex) {
      processRoot(data->scene->nodes[nodeIndex]);
    }
  }
}

bool loadGltfCpu(const std::string &path, GltfSceneCpu &out,
                 const GltfLoadOptions &options) {
  out = {};

  CgltfDoc doc{};
  if (!parseCgltfFile(path, doc)) {
    return false;
  }

  MaterialMap matMap;
  PrimitiveMap primMap;

  loadMaterials(doc.data, out, matMap);
  loadPrimitives(doc.data, matMap, options, out, primMap);

  const glm::mat4 fix = axisFixup(options.axis);
  buildNodes(doc.data, fix, primMap, out);

  return !out.nodes.empty() && !out.primitives.empty();
}

} // namespace engine::assets
