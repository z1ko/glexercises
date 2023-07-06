#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <vector>
#include <cstdlib>
#include <unordered_map>

#include <graphics.hpp>
#include <mesh.hpp>

namespace glib {

struct mesh_t 
{  
  buffer_t  buffer;

  texture_t albedo;
  texture_t specular;
  texture_t normal;
};

struct model_t 
{
  std::vector<mesh_t> meshes;
};

// Load model from file
model_t model_load(const char *filepath);
void model_render(const model_t &model, const program_t &program);

#ifdef GLIB_MODEL_IMPL
#undef GLIB_MODEL_IMPL

void model_render(const model_t &model, const program_t &program) {
  for (int i = 0; i < model.meshes.size(); ++i) {
    const mesh_t& mesh = model.meshes[i];

    // Bind standard textures
    glib::texture_bind(mesh.albedo,   0); // diffuse
    glib::texture_bind(mesh.specular, 1); // specular

    glib::render(mesh.buffer, program);
  }
}

// Load only the first one
static texture_t process_material_texture(aiMaterial *material, aiTextureType type, const std::string &folder) {
  if (material->GetTextureCount(type) == 0) {
    std::cout << "ERROR::ASSIMP::" << "No texture for " << type << "type" << std::endl;
    exit(1);
  }

  aiString str;
  material->GetTexture(type, 0, &str);
  std::string path = folder + "/" + str.C_Str();
  
  // Contains all previous loaded textures
  static std::unordered_map<std::string, texture_t> loaded_textures;
  if (loaded_textures.find(path) == loaded_textures.end()) {
    std::cout << "loading texture at " << path << "\n";
    texture_t texture = texture_load(path.c_str(), GL_RGB, GL_REPEAT);
    loaded_textures.insert({{path, texture}});
  }

  return loaded_textures.find(path)->second;
}

static void process_mesh(model_t &model, aiMesh *mesh, const aiScene *scene, const std::string &folder) {
  
  // Position - Normals - UVs
  std::vector<float> vertices;
  std::vector<index_t> indices;

  // Load all mesh data
  for (int i = 0; i < mesh->mNumVertices; ++i) 
  {
    // Position
    float x = mesh->mVertices[i].x;
    float y = mesh->mVertices[i].y;
    float z = mesh->mVertices[i].z;
   
    //printf("position(%f, %f, %f)\n", x, y, z);
    vertices.push_back(x);
    vertices.push_back(y);
    vertices.push_back(z);

    // Normal
    float nx = mesh->mNormals[i].x; 
    float ny = mesh->mNormals[i].y; 
    float nz = mesh->mNormals[i].z; 

    //printf("normal(%f, %f, %f)\n", nx, ny, nz);
    vertices.push_back(nx);
    vertices.push_back(ny);
    vertices.push_back(nz);

    // UVs
    float u = 0.0f, v = 0.0f;
    if (mesh->mTextureCoords[0]) {
      u = mesh->mTextureCoords[0][i].x;
      v = mesh->mTextureCoords[0][i].y;
    }

    //printf("uv(%f, %f)\n", u, v);
    vertices.push_back(u);
    vertices.push_back(v);
  }

  // Process indices
  for (int i = 0; i < mesh->mNumFaces; ++i) {
    aiFace face = mesh->mFaces[i];
    for (int j = 0; j < face.mNumIndices; ++j)
      indices.push_back(face.mIndices[j]);
  }

  mesh_t result = {
    .buffer = buffer_create(&vertices, &indices, glib::layout_3F3F2F)
  };

  // Process material
  if (mesh->mMaterialIndex > 0) {
    aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
    result.albedo   = process_material_texture(material, aiTextureType_DIFFUSE, folder);
    result.specular = process_material_texture(material, aiTextureType_SPECULAR, folder);
    //result.normal   = process_material_texture(material);
  }

  model.meshes.push_back(result);
}

static void process_node(model_t &model, aiNode *node, const aiScene *scene, const std::string& folder) {
  
  // Process all meshes
  for (int i = 0; i < node->mNumMeshes; ++i) {
    std::cout << "processing mesh of node\n";
    aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
    process_mesh(model, mesh, scene, folder);
  }

  // Continue traversal of node tree
  for (int i = 0; i < node->mNumChildren; ++i) {
    printf("processing child node (%d/%d)\n", i + 1, node->mNumChildren);
    process_node(model, node->mChildren[i], scene, folder);
  }
}

model_t model_load(const char* filepath) {
  model_t result;

  Assimp::Importer importer;
  const aiScene *scene = importer.ReadFile(filepath, aiProcess_Triangulate | aiProcess_FlipUVs);
  if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
    std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
    exit(1);
  }

  std::string folder{filepath};
  folder = folder.substr(0, folder.find_last_of("/"));
  process_node(result, scene->mRootNode, scene, folder);
  return result;
}

#endif

}
