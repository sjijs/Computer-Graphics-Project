#ifndef MODEL_LOADER_H
#define MODEL_LOADER_H

#include "triangle_mesh.h"
#include "material.h"
#include "rtweekend.h" // 提供 vec3/point3 等
#include <memory>
#include <string>
#include <vector>

// Assimp 头文件
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

struct ModelLoadOptions {
    vec3 scale{1,1,1};
    vec3 translate{0,0,0};
    bool flip_winding = false;     // 有些模型需要翻转面顺序
    bool center_model = false;     // 将模型中心移动到(0,0,0)
};

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

        // 优先使用 face 索引
        if (mesh->HasFaces()) {
            for (unsigned fi = 0; fi < mesh->mNumFaces; ++fi) {
                const aiFace& face = mesh->mFaces[fi];
                if (face.mNumIndices < 3) continue;

                // 每个 face 已经是三角化后的（因为 aiProcess_Triangulate）
                if (face.mNumIndices == 3) {
                    aiVector3D va = mesh->mVertices[face.mIndices[0]];
                    aiVector3D vb = mesh->mVertices[face.mIndices[1]];
                    aiVector3D vc = mesh->mVertices[face.mIndices[2]];

                    point3 A(va.x * opt.scale.x() + opt.translate.x(),
                             va.y * opt.scale.y() + opt.translate.y(),
                             va.z * opt.scale.z() + opt.translate.z());
                    point3 B(vb.x * opt.scale.x() + opt.translate.x(),
                             vb.y * opt.scale.y() + opt.translate.y(),
                             vb.z * opt.scale.z() + opt.translate.z());
                    point3 C(vc.x * opt.scale.x() + opt.translate.x(),
                             vc.y * opt.scale.y() + opt.translate.y(),
                             vc.z * opt.scale.z() + opt.translate.z());

                    if (opt.center_model) {
                        A = A - center; B = B - center; C = C - center;
                    }

                    if (opt.flip_winding)
                        mesh_hittable->add_triangle(A, C, B, mat);
                    else
                        mesh_hittable->add_triangle(A, B, C, mat);
                } else {
                    // 保险处理：若 Assimp 没三角化（理论不会），按扇形剖分
                    for (unsigned k = 2; k < face.mNumIndices; ++k) {
                        aiVector3D va = mesh->mVertices[face.mIndices[0]];
                        aiVector3D vb = mesh->mVertices[face.mIndices[k-1]];
                        aiVector3D vc = mesh->mVertices[face.mIndices[k]];

                        point3 A(va.x * opt.scale.x() + opt.translate.x(),
                                 va.y * opt.scale.y() + opt.translate.y(),
                                 va.z * opt.scale.z() + opt.translate.z());
                        point3 B(vb.x * opt.scale.x() + opt.translate.x(),
                                 vb.y * opt.scale.y() + opt.translate.y(),
                                 vb.z * opt.scale.z() + opt.translate.z());
                        point3 C(vc.x * opt.scale.x() + opt.translate.x(),
                                 vc.y * opt.scale.y() + opt.translate.y(),
                                 vc.z * opt.scale.z() + opt.translate.z());

                        if (opt.center_model) {
                            A = A - center; B = B - center; C = C - center;
                        }

                        if (opt.flip_winding)
                            mesh_hittable->add_triangle(A, C, B, mat);
                        else
                            mesh_hittable->add_triangle(A, B, C, mat);
                    }
                }
            }
        } else {
            // 无索引：按你的想法，每3个顶点构成一个三角形
            unsigned vcount = mesh->mNumVertices;
            for (unsigned vi = 0; vi + 2 < vcount; vi += 3) {
                aiVector3D va = mesh->mVertices[vi + 0];
                aiVector3D vb = mesh->mVertices[vi + 1];
                aiVector3D vc = mesh->mVertices[vi + 2];

                point3 A(va.x * opt.scale.x() + opt.translate.x(),
                         va.y * opt.scale.y() + opt.translate.y(),
                         va.z * opt.scale.z() + opt.translate.z());
                point3 B(vb.x * opt.scale.x() + opt.translate.x(),
                         vb.y * opt.scale.y() + opt.translate.y(),
                         vb.z * opt.scale.z() + opt.translate.z());
                point3 C(vc.x * opt.scale.x() + opt.translate.x(),
                         vc.y * opt.scale.y() + opt.translate.y(),
                         vc.z * opt.scale.z() + opt.translate.z());

                if (opt.center_model) {
                    A = A - center; B = B - center; C = C - center;
                }

                if (opt.flip_winding)
                    mesh_hittable->add_triangle(A, C, B, mat);
                else
                    mesh_hittable->add_triangle(A, B, C, mat);
            }
        }
    }

    mesh_hittable->build_bvh();
    return mesh_hittable;
}

#endif