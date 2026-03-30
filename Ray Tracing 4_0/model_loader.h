#ifndef MODEL_LOADER_H
#define MODEL_LOADER_H

#include "triangle_mesh.h"
#include "material.h"
#include "rtweekend.h" // 提供 vec3/point3 等
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>

#include <assimp/material.h>

// Assimp 头文件
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

struct ModelLoadOptions {
    vec3 scale{1,1,1};
    vec3 translate{0,0,0};
    bool flip_winding = false;     // 有些模型需要翻转面顺序
    bool center_model = false;     // 将模型中心移动到(0,0,0)
    bool import_embedded_materials = true;
};

inline std::string model_dirname(const std::string& filepath) {
    auto p = filepath.find_last_of("\\/");
    if (p == std::string::npos) return ".";
    return filepath.substr(0, p);
}

inline std::string join_model_path(const std::string& base, const std::string& rel) {
    if (rel.empty()) return base;

    // 绝对路径（Windows + Unix）
    if ((rel.size() > 1 && rel[1] == ':') || rel[0] == '/' || rel[0] == '\\')
        return rel;

    if (base.empty()) return rel;
    if (base.back() == '/' || base.back() == '\\')
        return base + rel;

    return base + "/" + rel;
}

inline bool assimp_try_get_texture(const aiMaterial* mat, aiTextureType type, std::string& out_path) {
    aiString tex_path;
    if (mat->GetTexture(type, 0, &tex_path) == AI_SUCCESS) {
        out_path = tex_path.C_Str();
        return true;
    }
    return false;
}

inline std::shared_ptr<material> create_material_from_assimp(
    const aiMaterial* ai_mat,
    const std::string& model_dir,
    const std::shared_ptr<material>& fallback_mat
) {
    if (!ai_mat) return fallback_mat;

    aiColor4D base_rgba(1, 1, 1, 1);
    if (aiGetMaterialColor(ai_mat, AI_MATKEY_BASE_COLOR, &base_rgba) != AI_SUCCESS) {
        aiGetMaterialColor(ai_mat, AI_MATKEY_COLOR_DIFFUSE, &base_rgba);
    }

    float metallic = 1.0f;
    float roughness = 1.0f;
    aiGetMaterialFloat(ai_mat, AI_MATKEY_METALLIC_FACTOR, &metallic);
    aiGetMaterialFloat(ai_mat, AI_MATKEY_ROUGHNESS_FACTOR, &roughness);

    aiColor4D emissive_rgba(0, 0, 0, 1);
    aiGetMaterialColor(ai_mat, AI_MATKEY_COLOR_EMISSIVE, &emissive_rgba);

    std::string base_tex_rel;
    std::string normal_tex_rel;
    std::string metallic_tex_rel;
    std::string roughness_tex_rel;
    std::string packed_mr_tex_rel;
    std::string emissive_tex_rel;

    assimp_try_get_texture(ai_mat, aiTextureType_BASE_COLOR, base_tex_rel)
        || assimp_try_get_texture(ai_mat, aiTextureType_DIFFUSE, base_tex_rel);
    assimp_try_get_texture(ai_mat, aiTextureType_NORMALS, normal_tex_rel)
        || assimp_try_get_texture(ai_mat, aiTextureType_HEIGHT, normal_tex_rel);
    assimp_try_get_texture(ai_mat, aiTextureType_METALNESS, metallic_tex_rel);
    assimp_try_get_texture(ai_mat, aiTextureType_DIFFUSE_ROUGHNESS, roughness_tex_rel);
    assimp_try_get_texture(ai_mat, aiTextureType_UNKNOWN, packed_mr_tex_rel);
    assimp_try_get_texture(ai_mat, aiTextureType_EMISSIVE, emissive_tex_rel);

    auto base_color = color(base_rgba.r, base_rgba.g, base_rgba.b);
    auto disney = std::shared_ptr<disney_material>();

    if (!base_tex_rel.empty()) {
        auto base_tex = make_shared<image_texture>(join_model_path(model_dir, base_tex_rel).c_str());
        disney = make_shared<disney_material>(
            base_tex,
            std::clamp(static_cast<double>(metallic), 0.0, 1.0),
            std::clamp(static_cast<double>(roughness), 0.02, 1.0),
            0.5,
            0.0,
            0.0
        );
    } else {
        disney = make_shared<disney_material>(
            base_color,
            std::clamp(static_cast<double>(metallic), 0.0, 1.0),
            std::clamp(static_cast<double>(roughness), 0.02, 1.0),
            0.5,
            0.0,
            0.0
        );
    }

    if (!normal_tex_rel.empty()) {
        disney->set_normal_texture(make_shared<image_texture>(join_model_path(model_dir, normal_tex_rel).c_str()));
    }

    if (!packed_mr_tex_rel.empty()) {
        disney->set_metallic_roughness_texture(make_shared<image_texture>(join_model_path(model_dir, packed_mr_tex_rel).c_str()));
    } else if (!roughness_tex_rel.empty()) {
        // 对于分离贴图，优先粗糙度贴图。
        disney->set_metallic_roughness_texture(make_shared<image_texture>(join_model_path(model_dir, roughness_tex_rel).c_str()));
    } else if (!metallic_tex_rel.empty()) {
        disney->set_metallic_roughness_texture(make_shared<image_texture>(join_model_path(model_dir, metallic_tex_rel).c_str()));
    }

    if (!emissive_tex_rel.empty()) {
        disney->set_emissive_texture(make_shared<image_texture>(join_model_path(model_dir, emissive_tex_rel).c_str()));
    }

    disney->set_emissive_factor(color(emissive_rgba.r, emissive_rgba.g, emissive_rgba.b));
    return disney;
}

inline std::shared_ptr<hittable> load_model_as_hittable(
    const std::string& filepath,
    std::shared_ptr<material> mat,
    const ModelLoadOptions& opt = {}
) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        filepath,
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality |
        aiProcess_OptimizeMeshes |
        aiProcess_PreTransformVertices // 将层级预变换，便于直接取顶点
    );

    if (!scene || !scene->mRootNode || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
        throw std::runtime_error(std::string("Assimp load failed: ") + importer.GetErrorString());
    }

    std::string model_dir = model_dirname(filepath);

    std::vector<std::shared_ptr<material>> imported_materials(scene->mNumMaterials, mat);
    if (opt.import_embedded_materials) {
        for (unsigned mi = 0; mi < scene->mNumMaterials; ++mi) {
            imported_materials[mi] = create_material_from_assimp(scene->mMaterials[mi], model_dir, mat);
        }
    }

    // 先收集所有顶点，若 center_model 为真则做居中处理
    // 但由于我们逐 mesh 转换，居中可以通过整体 bbox 再修正。这里简单策略：
    vec3 bbox_min( 1e30, 1e30, 1e30);
    vec3 bbox_max(-1e30,-1e30,-1e30);

    auto update_bbox = [&](const point3& p){
        bbox_min = vec3(fmin(bbox_min.x(), p.x()), fmin(bbox_min.y(), p.y()), fmin(bbox_min.z(), p.z()));
        bbox_max = vec3(fmax(bbox_max.x(), p.x()), fmax(bbox_max.y(), p.y()), fmax(bbox_max.z(), p.z()));
    };

    // 先遍历一次获取 bbox（按预变换后的场景）
    for (unsigned mi = 0; mi < scene->mNumMeshes; ++mi) {
        const aiMesh* mesh = scene->mMeshes[mi];
        for (unsigned vi = 0; vi < mesh->mNumVertices; ++vi) {
            aiVector3D v = mesh->mVertices[vi];
            point3 p(v.x * opt.scale.x() + opt.translate.x(),
                     v.y * opt.scale.y() + opt.translate.y(),
                     v.z * opt.scale.z() + opt.translate.z());
            update_bbox(p);
        }
    }

    vec3 center(0,0,0);
    if (opt.center_model) {
        center = 0.5 * (bbox_min + bbox_max);
    }

    auto mesh_hittable = std::make_shared<triangle_mesh>();

    // 再次遍历并创建三角形
    for (unsigned mi = 0; mi < scene->mNumMeshes; ++mi) {
        const aiMesh* mesh = scene->mMeshes[mi];
        std::shared_ptr<material> mesh_mat = mat;
        if (opt.import_embedded_materials && mesh->mMaterialIndex < imported_materials.size()) {
            mesh_mat = imported_materials[mesh->mMaterialIndex];
        }

        auto get_vertex = [&](unsigned idx) {
            aiVector3D v = mesh->mVertices[idx];
            point3 p(v.x * opt.scale.x() + opt.translate.x(),
                     v.y * opt.scale.y() + opt.translate.y(),
                     v.z * opt.scale.z() + opt.translate.z());
            if (opt.center_model) p = p - center;
            return p;
        };

        auto get_uv = [&](unsigned idx) {
            if (mesh->HasTextureCoords(0)) {
                aiVector3D t = mesh->mTextureCoords[0][idx];
                return vec3(t.x, t.y, 0);
            }
            return vec3(0, 0, 0);
        };

        auto get_normal = [&](unsigned idx) {
            if (mesh->HasNormals()) {
                aiVector3D n = mesh->mNormals[idx];
                return unit_vector(vec3(n.x, n.y, n.z));
            }
            return vec3(0, 0, 0);
        };

        bool has_uv = mesh->HasTextureCoords(0);
        bool has_normals = mesh->HasNormals();

        // 优先使用 face 索引
        if (mesh->HasFaces()) {
            for (unsigned fi = 0; fi < mesh->mNumFaces; ++fi) {
                const aiFace& face = mesh->mFaces[fi];
                if (face.mNumIndices < 3) continue;

                // 每个 face 已经是三角化后的（因为 aiProcess_Triangulate）
                if (face.mNumIndices == 3) {
                    unsigned i0 = face.mIndices[0];
                    unsigned i1 = face.mIndices[1];
                    unsigned i2 = face.mIndices[2];
                    if (opt.flip_winding) std::swap(i1, i2);

                    mesh_hittable->add_triangle(
                        get_vertex(i0), get_vertex(i1), get_vertex(i2),
                        get_uv(i0), get_uv(i1), get_uv(i2),
                        get_normal(i0), get_normal(i1), get_normal(i2),
                        has_uv, has_normals,
                        mesh_mat
                    );
                } else {
                    // 保险处理：若 Assimp 没三角化（理论不会），按扇形剖分
                    for (unsigned k = 2; k < face.mNumIndices; ++k) {
                        unsigned i0 = face.mIndices[0];
                        unsigned i1 = face.mIndices[k - 1];
                        unsigned i2 = face.mIndices[k];
                        if (opt.flip_winding) std::swap(i1, i2);

                        mesh_hittable->add_triangle(
                            get_vertex(i0), get_vertex(i1), get_vertex(i2),
                            get_uv(i0), get_uv(i1), get_uv(i2),
                            get_normal(i0), get_normal(i1), get_normal(i2),
                            has_uv, has_normals,
                            mesh_mat
                        );
                    }
                }
            }
        } else {
            // 无索引：按你的想法，每3个顶点构成一个三角形
            unsigned vcount = mesh->mNumVertices;
            for (unsigned vi = 0; vi + 2 < vcount; vi += 3) {
                unsigned i0 = vi;
                unsigned i1 = vi + 1;
                unsigned i2 = vi + 2;
                if (opt.flip_winding) std::swap(i1, i2);

                mesh_hittable->add_triangle(
                    get_vertex(i0), get_vertex(i1), get_vertex(i2),
                    get_uv(i0), get_uv(i1), get_uv(i2),
                    get_normal(i0), get_normal(i1), get_normal(i2),
                    has_uv, has_normals,
                    mesh_mat
                );
            }
        }
    }

    mesh_hittable->build_bvh();
    return mesh_hittable;
}

#endif