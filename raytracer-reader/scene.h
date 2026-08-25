#pragma once

#include "material.h"
#include "../raytracer-geom/vector.h"
#include "object.h"
#include "light.h"

#include <vector>
#include <unordered_map>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>

class Scene {
public:
    const std::vector<Object>& GetObjects() const;
    const std::vector<SphereObject>& GetSphereObjects() const;
    const std::vector<Light>& GetLights() const;
    const std::unordered_map<std::string, Material>& GetMaterials() const;

    friend Scene ReadScene(const std::filesystem::path& path);

private:
    std::vector<Object> objects_;
    std::vector<SphereObject> sphere_objects_;
    std::vector<Light> lights_;
    std::unordered_map<std::string, Material> materials_;
};

std::unordered_map<std::string, Material> ReadMaterials(const std::filesystem::path& path);
Scene ReadScene(const std::filesystem::path& path);
