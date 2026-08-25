#include "geometry.h"

std::optional<Intersection> GetIntersection(const Ray& ray, const Sphere& sphere) {
    constexpr double kEpsilon = std::numeric_limits<double>::epsilon();

    // normalize direction
    Vector dir = ray.GetDirection();
    dir.Normalize();

    // vector from sphere center to ray origin
    Vector oc = ray.GetOrigin() - sphere.GetCenter();

    // coefficients for ax^2 + bx + c = 0
    // dir is normalized, so DotProduct(dir, dir) = 1
    double a = 1;
    double b = 2 * DotProduct(oc, dir);
    double c = DotProduct(oc, oc) - sphere.GetRadius() * sphere.GetRadius();

    // discriminant
    double discriminant = b * b - 4 * a * c;

    // check if ray misses the sphere
    if (discriminant < 0) {
        return std::nullopt;
    }

    double sqrt_discriminant = std::sqrt(discriminant);

    // find roots (intersection points)
    double t1 = (-b - sqrt_discriminant) / 2;
    double t2 = (-b + sqrt_discriminant) / 2;

    // choose the closest valid intersection
    double t = -1;
    if (t1 > kEpsilon) {
        t = t1;
    } else if (t2 > kEpsilon) {
        t = t2;
    } else {
        return std::nullopt;
    }

    // 3D position and normalize
    Vector position = ray.GetOrigin() + dir * t;
    Vector normal = position - sphere.GetCenter();
    normal.Normalize();

    // face normal towards the ray
    if (DotProduct(normal, dir) > 0) {
        normal *= -1;
    }

    return Intersection(position, normal, t);
}

std::optional<Intersection> GetIntersection(const Ray& ray, const Triangle& triangle) {
    constexpr double kEpsilon = std::numeric_limits<double>::epsilon();

    // normalize direction
    Vector dir = ray.GetDirection();
    dir.Normalize();

    // 2 vectors out of 1 vertice
    Vector vec1 = triangle[1] - triangle[0];
    Vector vec2 = triangle[2] - triangle[0];

    // helper vec for Kramer's method
    Vector cross_vec2 = CrossProduct(dir, vec2);

    double det = DotProduct(vec1, cross_vec2);

    // check if parallel (det -> 0)
    if (std::fabs(det) < kEpsilon) {
        return std::nullopt;
    }

    double inv_det = 1 / det;

    // barycentric coordinate for u
    Vector s = ray.GetOrigin() - triangle[0];
    double u = inv_det * DotProduct(s, cross_vec2);

    // ray passes outside vec2's bounds
    if (u < -kEpsilon || u - 1 > kEpsilon) {
        return std::nullopt;
    }

    // barycentric coordinate for v
    Vector s_cross_vec1 = CrossProduct(s, vec1);
    double v = inv_det * DotProduct(dir, s_cross_vec1);

    // ray passes outside vec1's bounds
    if (v < -kEpsilon || u + v - 1 > kEpsilon) {
        return std::nullopt;
    }

    // compute t to find where on the ray the intersection is
    double t = inv_det * DotProduct(vec2, s_cross_vec1);

    // ray intersection
    if (t > kEpsilon) {
        Vector position = ray.GetOrigin() + dir * t;
        Vector normal = CrossProduct(vec1, vec2);
        normal.Normalize();

        if (DotProduct(normal, dir) > 0) {
            normal *= -1;
        }

        return Intersection(position, normal, t);
    }

    // there is a line intersection but not a ray intersection
    return std::nullopt;
}

// reflection direction calculate as I - 2.0 * dot(N, I) * N
// I - incident vector, N - normal vector
Vector Reflect(const Vector& ray, const Vector& normal) {
    Vector normal_normalized = normal;
    normal_normalized.Normalize();
    return ray - 2 * DotProduct(ray, normal_normalized) * normal_normalized;
}

// eta = n1/n2 - specifies the ratio of indices of refraction
std::optional<Vector> Refract(const Vector& ray, const Vector& normal, double eta) {
    Vector ray_normalized = ray;
    ray_normalized.Normalize();
    Vector normal_normalized = normal;
    normal_normalized.Normalize();

    // k = 1.0 - eta * eta * (1.0 - dot(N, I) * dot(N, I));
    double k = 1 - eta * eta *
                       (1 - DotProduct(ray_normalized, normal_normalized) *
                                DotProduct(normal_normalized, ray_normalized));

    // R = eta * I - (eta * dot(N, I) + sqrt(k)) * N;
    // R - refraction vector
    if (k < 0) {
        return std::nullopt;
    } else {
        return eta * ray_normalized -
               (eta * DotProduct(normal_normalized, ray_normalized) + std::sqrt(k)) *
                   normal_normalized;
    }
}

// edge approach
Vector GetBarycentricCoords(const Triangle& triangle, const Vector& r) {
    // point r in a triangle's plane
    Vector r1 = triangle[0];
    Vector r2 = triangle[1];
    Vector r3 = triangle[2];

    // edges
    Vector e1 = r2 - r1;
    Vector e2 = r3 - r1;
    Vector e3 = r - r1;

    // dot products to turn into 3D system
    double d00 = DotProduct(e1, e1);
    double d01 = DotProduct(e1, e2);
    double d11 = DotProduct(e2, e2);
    double d20 = DotProduct(e3, e1);
    double d21 = DotProduct(e3, e2);

    // denominator of Kramer's method
    double denom = d00 * d11 - d01 * d01;

    // lambda_2 and lambda_3
    double lambda_2 = (d11 * d20 - d01 * d21) / denom;
    double lambda_3 = (d00 * d21 - d01 * d20) / denom;

    // lambda_1 + lambda_2 + lambda_3= 1
    double lambda_1 = 1 - lambda_2 - lambda_3;

    // barycentric coordinates
    return Vector(lambda_1, lambda_2, lambda_3);
}
