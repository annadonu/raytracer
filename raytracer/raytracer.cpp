#include "raytracer.h"
#include "../raytracer-reader/scene.h"
#include "../raytracer-geom/geometry.h"

constexpr double kEps = 1e-4;

// structure to hold closest intersection details
struct IntersectionData {
    double t = -1;
    Vector position;
    Vector normal;
    const Material* material = nullptr;
};

// component-wise vector multiplication for colors
Vector Multiply(const Vector& a, const Vector& b) {
    return Vector{a[0] * b[0], a[1] * b[1], a[2] * b[2]};
}

std::optional<IntersectionData> FindClosestIntersection(const Ray& ray, const Scene& scene) {
    double min_t = std::numeric_limits<double>::infinity();
    std::optional<IntersectionData> result;

    // check sphere intersections
    for (const auto& sphere_obj : scene.GetSphereObjects()) {
        auto intersection = GetIntersection(ray, sphere_obj.sphere);

        if (intersection && intersection->GetDistance() < min_t &&
            intersection->GetDistance() > 0) {
            min_t = intersection->GetDistance();
            Vector normal = intersection->GetNormal();

            // face normal towards the ray
            if (DotProduct(normal, ray.GetDirection()) > 0) {
                normal *= -1;
            }

            result = IntersectionData{.t = intersection->GetDistance(),
                                      .position = intersection->GetPosition(),
                                      .normal = normal,
                                      .material = sphere_obj.material};
        }
    }

    // check polygon intersections
    for (const auto& obj : scene.GetObjects()) {
        auto intersection = GetIntersection(ray, obj.polygon);

        if (intersection && intersection->GetDistance() < min_t &&
            intersection->GetDistance() > 0) {
            min_t = intersection->GetDistance();
            Vector normal = intersection->GetNormal();

            // vertex normals
            const Vector* n0 = obj.GetNormal(0);
            const Vector* n1 = obj.GetNormal(1);
            const Vector* n2 = obj.GetNormal(2);

            // interpolate using barycentric coordinates
            if (n0 && n1 && n2 && Length(*n0) > 1e-6 && Length(*n1) > 1e-6 && Length(*n2) > 1e-6) {
                Vector bary = GetBarycentricCoords(obj.polygon, intersection->GetPosition());
                normal = (*n0) * bary[0] + (*n1) * bary[1] + (*n2) * bary[2];
                normal.Normalize();
            }

            // face normal towards the ray
            if (DotProduct(normal, ray.GetDirection()) > 0) {
                normal *= -1;
            }

            result = IntersectionData{.t = intersection->GetDistance(),
                                      .position = intersection->GetPosition(),
                                      .normal = normal,
                                      .material = obj.material};
        }
    }

    return result;
}

// recursive ray tracing
Vector TraceRay(const Ray& ray, const Scene& scene, int depth, bool inside) {
    // limit recursion
    if (depth <= 0) {
        return Vector{0, 0, 0};
    }

    auto intersection_opt = FindClosestIntersection(ray, scene);

    // ray missed the scene
    if (!intersection_opt) {
        return Vector{0, 0, 0};
    }

    const auto& intersection = *intersection_opt;

    // fallback if material is missing
    if (!intersection.material) {
        return Vector{0, 0, 0};
    }

    const auto& mat = *intersection.material;

    // normalize direction
    Vector dir = ray.GetDirection();
    dir.Normalize();

    Vector sum_diffuse{0, 0, 0};
    Vector sum_specular{0, 0, 0};

    // lighting calculation
    for (const auto& light : scene.GetLights()) {
        // vector from surface to light
        Vector light_vec = light.position - intersection.position;
        double light_dist = Length(light_vec);

        Vector vl = light_vec;
        vl.Normalize();

        // shadow ray
        Ray shadow_ray{intersection.position + intersection.normal * kEps, vl};
        auto shadow_intersection = FindClosestIntersection(shadow_ray, scene);

        // point is in shadow
        if (shadow_intersection && shadow_intersection->t < light_dist - kEps) {
            continue;
        }

        // diffuse contribution
        double diff_factor = std::max(0.0, DotProduct(vl, intersection.normal));
        sum_diffuse += light.intensity * diff_factor;

        // specular contribution
        Vector v_lr = Reflect(vl * -1, intersection.normal);
        v_lr.Normalize();
        double spec_factor = std::max(0.0, DotProduct(dir * -1, v_lr));
        sum_specular += light.intensity * std::pow(spec_factor, mat.specular_exponent);
    }

    // combined illumination
    Vector illumination =
        Multiply(mat.diffuse_color, sum_diffuse) + Multiply(mat.specular_color, sum_specular);
    Vector final_color = illumination * mat.albedo[0] + mat.ambient_color + mat.intensity;

    // reflection
    if (mat.albedo[1] != 0 && !inside) {
        Vector reflect_dir = Reflect(dir, intersection.normal);
        reflect_dir.Normalize();
        Ray reflect_ray{intersection.position + intersection.normal * kEps, reflect_dir};
        final_color += TraceRay(reflect_ray, scene, depth - 1, inside) * mat.albedo[1];
    }

    // refraction
    if (mat.albedo[2] != 0) {
        double eta = inside ? mat.refraction_index : (1 / mat.refraction_index);
        auto refract_dir_opt = Refract(dir, intersection.normal, eta);

        if (refract_dir_opt) {
            Vector refract_dir = *refract_dir_opt;
            refract_dir.Normalize();
            Ray refract_ray{intersection.position - intersection.normal * kEps, refract_dir};

            double transparency = inside ? 1 : mat.albedo[2];
            final_color += TraceRay(refract_ray, scene, depth - 1, !inside) * transparency;
        }
    }

    return final_color;
}

Image Render(const std::filesystem::path& path, const CameraOptions& camera_options,
             const RenderOptions& render_options) {
    Scene scene = ReadScene(path);
    int width = static_cast<int>(camera_options.screen_width);
    int height = static_cast<int>(camera_options.screen_height);
    Image image(width, height);

    // camera basis vectors
    Vector forward = camera_options.look_to - camera_options.look_from;
    forward.Normalize();

    Vector right;
    Vector world_up{0, 1, 0};
    Vector temp_right = CrossProduct(forward, world_up);

    // handle looking strictly up or down
    if (Length(temp_right) < 1e-6) {
        right = Vector{1, 0, 0};
    } else {
        right = temp_right;
        right.Normalize();
    }

    Vector up = CrossProduct(right, forward);
    up.Normalize();

    // buffers
    std::vector<Vector> hdr_buffer(width * height, Vector{0, 0, 0});
    std::vector<double> depth_buffer(width * height, -1);
    double max_depth = 0;

    double scale = std::tan(camera_options.fov / 2);
    double aspect_ratio = static_cast<double>(width) / height;

    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            // screen coordinates to camera space
            double x_cam = (2 * (i + 0.5) / width - 1) * scale * aspect_ratio;
            double y_cam = (1 - 2 * (j + 0.5) / height) * scale;

            Vector ray_dir = right * x_cam + up * y_cam + forward;
            ray_dir.Normalize();

            Ray ray{camera_options.look_from, ray_dir};
            int pixel_index = j * width + i;

            if (render_options.mode == RenderMode::kDepth) {
                auto intersection_opt = FindClosestIntersection(ray, scene);
                if (intersection_opt) {
                    depth_buffer[pixel_index] = intersection_opt->t;
                    max_depth = std::max(max_depth, intersection_opt->t);
                }
            } else if (render_options.mode == RenderMode::kNormal) {
                auto intersection_opt = FindClosestIntersection(ray, scene);
                if (intersection_opt) {
                    Vector n = intersection_opt->normal;
                    hdr_buffer[pixel_index] = n * 0.5 + Vector{0.5, 0.5, 0.5};
                } else {
                    hdr_buffer[pixel_index] = Vector{0, 0, 0};
                }
            } else if (render_options.mode == RenderMode::kFull) {
                hdr_buffer[pixel_index] = TraceRay(ray, scene, render_options.depth, false);
            }
        }
    }

    // normalize depth mode output
    if (render_options.mode == RenderMode::kDepth) {
        for (int idx = 0; idx < width * height; ++idx) {
            if (depth_buffer[idx] < 0) {
                hdr_buffer[idx] = Vector{1, 1, 1};
            } else {
                double norm_d = (max_depth > 0) ? (depth_buffer[idx] / max_depth) : 0;
                hdr_buffer[idx] = Vector{norm_d, norm_d, norm_d};
            }
        }
    }

    // tone mapping max component
    double max_c = 0;
    if (render_options.mode == RenderMode::kFull) {
        for (const auto& pixel : hdr_buffer) {
            max_c = std::max({max_c, pixel[0], pixel[1], pixel[2]});
        }
    }

    // post-processing
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            Vector color = hdr_buffer[j * width + i];

            if (render_options.mode == RenderMode::kFull && max_c > 0) {
                Vector v_in = color;
                Vector num = Multiply(v_in, Vector{1, 1, 1} + v_in / (max_c * max_c));
                Vector den = Vector{1, 1, 1} + v_in;
                color = Vector{num[0] / den[0], num[1] / den[1], num[2] / den[2]};
            }

            // gamma correction
            if (render_options.mode == RenderMode::kFull) {
                color[0] = std::pow(std::max(0.0, color[0]), 1.0 / 2.2);
                color[1] = std::pow(std::max(0.0, color[1]), 1.0 / 2.2);
                color[2] = std::pow(std::max(0.0, color[2]), 1.0 / 2.2);
            }

            // convert to rgb and set pixel
            RGB rgb{static_cast<uint8_t>(std::clamp(color[0] * 255, 0.0, 255.0)),
                    static_cast<uint8_t>(std::clamp(color[1] * 255, 0.0, 255.0)),
                    static_cast<uint8_t>(std::clamp(color[2] * 255, 0.0, 255.0))};

            image.SetPixel(rgb, j, i);
        }
    }

    return image;
}
