#include "scene.h"

#include <fstream>
#include <sstream>

const std::vector<Object>& Scene::GetObjects() const {
    return objects_;
}
const std::vector<SphereObject>& Scene::GetSphereObjects() const {
    return sphere_objects_;
}
const std::vector<Light>& Scene::GetLights() const {
    return lights_;
}
const std::unordered_map<std::string, Material>& Scene::GetMaterials() const {
    return materials_;
}

std::unordered_map<std::string, Material> ReadMaterials(const std::filesystem::path& path) {
    std::unordered_map<std::string, Material> materials;
    std::ifstream file(path);

    // check if file is open
    if (!file.is_open()) {
        return materials;
    }

    std::string line;
    Material* current_material = nullptr;

    // read file
    while (std::getline(file, line)) {
        // skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::stringstream ss(line);
        std::string cmd;
        ss >> cmd;

        // create new material with default values
        if (cmd == "newmtl") {
            std::string name;
            ss >> name;
            materials[name] = Material{.name = name,
                                       .ambient_color = {0.0, 0.0, 0.0},
                                       .diffuse_color = {0.0, 0.0, 0.0},
                                       .specular_color = {0.0, 0.0, 0.0},
                                       .intensity = {0.0, 0.0, 0.0},
                                       .specular_exponent = 1.0,
                                       .refraction_index = 1.0,
                                       .albedo = {1.0, 0.0, 0.0}};
            current_material = &materials[name];
        } else if (current_material) {
            double x;
            double y;
            double z;

            // parse material properties
            if (cmd == "Ka") {
                // Ka: ambient color (color in shadow)
                if (ss >> x >> y >> z) {
                    current_material->ambient_color = Vector{x, y, z};
                }
            } else if (cmd == "Kd") {
                // Kd: diffuse color (base matte color)
                if (ss >> x >> y >> z) {
                    current_material->diffuse_color = Vector{x, y, z};
                }
            } else if (cmd == "Ks") {
                // Ks: specular color (color of shiny highlights)
                if (ss >> x >> y >> z) {
                    current_material->specular_color = Vector{x, y, z};
                }
            } else if (cmd == "Ke") {
                // Ke: emissive intensity (self-illumination color)
                if (ss >> x >> y >> z) {
                    current_material->intensity = Vector{x, y, z};
                }
            } else if (cmd == "Ns") {
                // Ns: specular exponent (shininess / roughness control)
                ss >> current_material->specular_exponent;
            } else if (cmd == "Ni") {
                // Ni: refraction index (optical density for transparent materials)
                ss >> current_material->refraction_index;
            } else if (cmd == "al") {
                // al: albedo components (energy distribution for diffuse, specular, refraction)
                if (ss >> x >> y >> z) {
                    current_material->albedo = Vector{x, y, z};
                }
            }
        }
    }

    return materials;
}

// helper struct for polygon face tokens
struct FaceVertex {
    // index of vertex
    int64_t v_idx = 0;
    // index of normal
    int64_t vn_idx = 0;
    bool has_vn = false;
};

// resolve 1-based and negative obj indices
size_t ResolveIndex(int64_t idx, size_t total_count) {
    if (idx > 0) {
        return static_cast<size_t>(idx - 1);
    }
    return static_cast<size_t>(static_cast<int64_t>(total_count) + idx);
}

// parse sphere
void ParseSphere(std::stringstream& ss, std::vector<SphereObject>& spheres,
                 const Material* current_material) {
    // x, y, z - center coordinates, r - radius
    double x;
    double y;
    double z;
    double r;

    ss >> x >> y >> z >> r;

    spheres.push_back(
        SphereObject{.material = current_material, .sphere = Sphere{Vector{x, y, z}, r}});
}

// parse light
void ParseLight(std::stringstream& ss, std::vector<Light>& lights) {
    // x, y, z - position, r, g, b - intensity
    double x;
    double y;
    double z;
    double r;
    double g;
    double b;

    ss >> x >> y >> z >> r >> g >> b;

    lights.push_back(Light{.position = Vector{x, y, z}, .intensity = Vector{r, g, b}});
}

// parse face and triangulate
void ParseFace(std::stringstream& ss, std::vector<Object>& objects,
               const std::vector<Vector>& global_vertices,
               const std::vector<Vector>& global_normals, const Material* current_material) {
    std::vector<FaceVertex> face_vertices;
    std::string token;

    // parse space-separated vertices of the polygon
    while (ss >> token) {
        FaceVertex fv;
        std::stringstream token_ss(token);

        // v - vertex index
        token_ss >> fv.v_idx;

        // check for texture and normal indices
        if (token_ss.peek() == '/') {
            token_ss.ignore();

            // vt - texture coordinate, ignored in this task
            if (token_ss.peek() != '/') {
                int64_t vt_idx;
                token_ss >> vt_idx;
            }

            // vn - normal index
            if (token_ss.peek() == '/') {
                token_ss.ignore();
                token_ss >> fv.vn_idx;
                fv.has_vn = true;
            }
        }

        face_vertices.push_back(fv);
    }

    // triangulation (P0, Pi, Pi+1)
    if (face_vertices.size() >= 3) {
        for (size_t i = 1; i < face_vertices.size() - 1; ++i) {
            // resolve indices to 0-based absolute format
            size_t idx0 = ResolveIndex(face_vertices[0].v_idx, global_vertices.size());
            size_t idxi = ResolveIndex(face_vertices[i].v_idx, global_vertices.size());
            size_t idxi1 = ResolveIndex(face_vertices[i + 1].v_idx, global_vertices.size());

            Vector n0 =
                face_vertices[0].has_vn
                    ? global_normals[ResolveIndex(face_vertices[0].vn_idx, global_normals.size())]
                    : Vector{0, 0, 0};
            Vector n1 =
                face_vertices[i].has_vn
                    ? global_normals[ResolveIndex(face_vertices[i].vn_idx, global_normals.size())]
                    : Vector{0, 0, 0};
            Vector n2 = face_vertices[i + 1].has_vn
                            ? global_normals[ResolveIndex(face_vertices[i + 1].vn_idx,
                                                          global_normals.size())]
                            : Vector{0, 0, 0};

            Object obj{.material = current_material,
                       .polygon = Triangle{global_vertices[idx0], global_vertices[idxi],
                                           global_vertices[idxi1]},
                       .normals = {n0, n1, n2}};

            objects.push_back(obj);
        }
    }
}

Scene ReadScene(const std::filesystem::path& path) {
    Scene scene;
    std::ifstream file(path);

    // check if file is open
    if (!file.is_open()) {
        return scene;
    }

    // local state for the parsing process
    std::vector<Vector> global_vertices;
    std::vector<Vector> global_normals;
    const Material* current_material = nullptr;

    std::string line;

    // read file
    while (std::getline(file, line)) {
        // skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::stringstream ss(line);
        std::string cmd;
        ss >> cmd;

        if (cmd == "v") {
            // x, y, z - vertex coordinates
            double x;
            double y;
            double z;

            ss >> x >> y >> z;
            global_vertices.push_back(Vector{x, y, z});
        } else if (cmd == "vn") {
            // x, y, z - normal vector components
            double x;
            double y;
            double z;

            ss >> x >> y >> z;
            global_normals.push_back(Vector{x, y, z});
        } else if (cmd == "mtllib") {
            // read material library filename
            std::string mtl_filename;

            ss >> mtl_filename;
            scene.materials_ = ReadMaterials(path.parent_path() / mtl_filename);
        } else if (cmd == "usemtl") {
            // read material name to apply to next objects
            std::string mtl_name;

            ss >> mtl_name;
            if (auto it = scene.materials_.find(mtl_name); it != scene.materials_.end()) {
                current_material = &it->second;
            }
        } else if (cmd == "S") {
            // parse sphere
            ParseSphere(ss, scene.sphere_objects_, current_material);
        } else if (cmd == "P") {
            // parse light
            ParseLight(ss, scene.lights_);
        } else if (cmd == "f") {
            // parse face
            ParseFace(ss, scene.objects_, global_vertices, global_normals, current_material);
        }
    }

    return scene;
}
