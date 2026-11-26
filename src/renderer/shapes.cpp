#include "shapes.hpp"

#include <cmath>

namespace {

constexpr float RELATIVE_RESOLUTION = 8.0f;

constexpr float PI     = 3.14159265358979323846f;
constexpr float TWO_PI = 2.0f * PI;

constexpr vec3 CONE_COLOR     = vec3(0.9f, 0.4f, 0.2f);
constexpr vec3 CYLINDER_COLOR = vec3(0.2f, 0.6f, 0.9f);
constexpr vec3 SPHERE_COLOR   = vec3(0.3f, 0.8f, 0.4f);
constexpr vec3 AABB_COLOR     = vec3(1.0f, 1.0f, 1.0f);

int SegmentCount(float size) {
    return std::max(3, static_cast<int>(std::ceil(size * RELATIVE_RESOLUTION)));
}

struct Basis {
    vec3 axis;
    vec3 u;
    vec3 v;
};

Basis MakeBasis(const vec3& dir) {
    Basis basis;
    basis.axis = glm::normalize(dir);

    const vec3 reference = std::abs(basis.axis.y) < 0.99f
        ? vec3(0.0f, 1.0f, 0.0f)
        : vec3(1.0f, 0.0f, 0.0f);

    basis.u = glm::normalize(glm::cross(reference, basis.axis));
    basis.v = glm::cross(basis.axis, basis.u);
    return basis;
}

vec3 RingPoint(const vec3& center, const Basis& basis, float radius, float angle) {
    return center + radius * (std::cos(angle) * basis.u + std::sin(angle) * basis.v);
}

void PushTriangle(std::vector<GeometryVertex>& vertices, const vec3& a, const vec3& b, const vec3& c, const vec3& color) {
    vertices.push_back({ a, color });
    vertices.push_back({ b, color });
    vertices.push_back({ c, color });
}

void PushQuad(std::vector<GeometryVertex>& vertices, const vec3& a, const vec3& b, const vec3& c, const vec3& d, const vec3& color) {
    PushTriangle(vertices, a, b, c, color);
    PushTriangle(vertices, a, c, d, color);
}

}

void MakeConeVertices(std::vector<GeometryVertex>& vertices, const vec3& origin, const vec3& dir, float radius) {
    const float height = glm::length(dir);
    if (height <= 0.0f || radius <= 0.0f) {
        return;
    }

    const int   segments    = SegmentCount(std::max(radius, height));
    const Basis basis       = MakeBasis(dir);
    const vec3  base_center = origin + dir;

    for (int i = 0; i < segments; ++i) {
        const float angle0 = TWO_PI * static_cast<float>(i) / static_cast<float>(segments);
        const float angle1 = TWO_PI * static_cast<float>(i + 1) / static_cast<float>(segments);

        const vec3 p0 = RingPoint(base_center, basis, radius, angle0);
        const vec3 p1 = RingPoint(base_center, basis, radius, angle1);

        PushTriangle(vertices, origin, p1, p0, CONE_COLOR);
        PushTriangle(vertices, base_center, p0, p1, CONE_COLOR);
    }
}

void MakeCylinderVertices(std::vector<GeometryVertex>& vertices, const vec3& origin, const vec3& dir, float radius) {
    const float height = glm::length(dir);
    if (height <= 0.0f || radius <= 0.0f) {
        return;
    }

    const int   segments   = SegmentCount(std::max(radius, height));
    const Basis basis      = MakeBasis(dir);
    const vec3  top_center = origin + dir;

    for (int i = 0; i < segments; ++i) {
        const float angle0 = TWO_PI * static_cast<float>(i) / static_cast<float>(segments);
        const float angle1 = TWO_PI * static_cast<float>(i + 1) / static_cast<float>(segments);

        const vec3 b0 = RingPoint(origin, basis, radius, angle0);
        const vec3 b1 = RingPoint(origin, basis, radius, angle1);
        const vec3 t0 = RingPoint(top_center, basis, radius, angle0);
        const vec3 t1 = RingPoint(top_center, basis, radius, angle1);

        PushQuad(vertices, b0, b1, t1, t0, CYLINDER_COLOR);
        PushTriangle(vertices, origin, b1, b0, CYLINDER_COLOR);
        PushTriangle(vertices, top_center, t0, t1, CYLINDER_COLOR);
    }
}

void MakeSphereVertices(std::vector<GeometryVertex>& vertices, const vec3& origin, float radius) {
    if (radius <= 0.0f) {
        return;
    }

    const int segments = SegmentCount(radius);

    for (int lat = 0; lat < segments; ++lat) {
        const float phi0 = PI * static_cast<float>(lat) / static_cast<float>(segments);
        const float phi1 = PI * static_cast<float>(lat + 1) / static_cast<float>(segments);

        const float sin_phi0 = std::sin(phi0);
        const float cos_phi0 = std::cos(phi0);
        const float sin_phi1 = std::sin(phi1);
        const float cos_phi1 = std::cos(phi1);

        for (int lon = 0; lon < segments; ++lon) {
            const float theta0 = TWO_PI * static_cast<float>(lon) / static_cast<float>(segments);
            const float theta1 = TWO_PI * static_cast<float>(lon + 1) / static_cast<float>(segments);

            const vec3 p00 = origin + radius * vec3(sin_phi0 * std::cos(theta0), cos_phi0, sin_phi0 * std::sin(theta0));
            const vec3 p01 = origin + radius * vec3(sin_phi0 * std::cos(theta1), cos_phi0, sin_phi0 * std::sin(theta1));
            const vec3 p10 = origin + radius * vec3(sin_phi1 * std::cos(theta0), cos_phi1, sin_phi1 * std::sin(theta0));
            const vec3 p11 = origin + radius * vec3(sin_phi1 * std::cos(theta1), cos_phi1, sin_phi1 * std::sin(theta1));

            PushQuad(vertices, p00, p10, p11, p01, SPHERE_COLOR);
        }
    }
}

void MakeAABBVertices(std::vector<GeometryVertex>& vertices, const vec3& min, const vec3& max) {
    if (min.x >= max.x || min.y >= max.y || min.z >= max.z) {
        return;
    }

    const vec3 p000 = vec3(min.x, min.y, min.z);
    const vec3 p001 = vec3(min.x, min.y, max.z);
    const vec3 p010 = vec3(min.x, max.y, min.z);
    const vec3 p011 = vec3(min.x, max.y, max.z);
    const vec3 p100 = vec3(max.x, min.y, min.z);
    const vec3 p101 = vec3(max.x, min.y, max.z);
    const vec3 p110 = vec3(max.x, max.y, min.z);
    const vec3 p111 = vec3(max.x, max.y, max.z);

    PushQuad(vertices, p000, p001, p011, p010, AABB_COLOR);
    PushQuad(vertices, p100, p110, p111, p101, AABB_COLOR);
    PushQuad(vertices, p000, p100, p101, p001, AABB_COLOR);
    PushQuad(vertices, p010, p011, p111, p110, AABB_COLOR);
    PushQuad(vertices, p000, p010, p110, p100, AABB_COLOR);
    PushQuad(vertices, p001, p101, p111, p011, AABB_COLOR);
}

void MakeShapeVertices(std::vector<GeometryVertex>& vertices, const Shape& shape) {
    std::visit(overloaded{
        [&](const Cone& cone) { MakeConeVertices(vertices, cone.origin, cone.dir, cone.radius); },
        [&](const Cylinder& cylinder) { MakeCylinderVertices(vertices, cylinder.origin, cylinder.dir, cylinder.radius); },
        [&](const Sphere& sphere) { MakeSphereVertices(vertices, sphere.origin, sphere.radius); },
        [&](const AABB& aabb) { MakeAABBVertices(vertices, aabb.min, aabb.max); },
    }, shape);
}

