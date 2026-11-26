#pragma once

struct GeometryVertex {
    vec3 pos;
    vec3 color;
};

struct Cone {
    vec3  origin;
    vec3  dir;
    float radius;
};

struct Cylinder {
    vec3 origin;
    vec3 dir;
    float radius;
};

struct Sphere {
    vec3 origin;
    float radius;
};

struct AABB {
    vec3 min;
    vec3 max;
};

using Shape = std::variant<Cone, Cylinder, Sphere, AABB>;

void MakeConeVertices(std::vector<GeometryVertex>& vertices, const vec3& origin, const vec3& dir, float radius);
void MakeCylinderVertices(std::vector<GeometryVertex>& vertices, const vec3& origin, const vec3& dir, float radius);
void MakeSphereVertices(std::vector<GeometryVertex>& vertices, const vec3& origin, float radius);
void MakeAABBVertices(std::vector<GeometryVertex>& vertices, const vec3& min, const vec3& max);
void MakeShapeVertices(std::vector<GeometryVertex>& vertices, const Shape& shape);

