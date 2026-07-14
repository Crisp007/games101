#pragma once

#include "Object.hpp"

#include <cstring>
#include <fstream>

bool rayTriangleIntersect(const Vector3f& v0, const Vector3f& v1, const Vector3f& v2, const Vector2f& st_v0, const Vector2f& st_v1, const Vector2f& st_v2, 
    const Vector3f& orig, const Vector3f& dir, float& tnear, float& u, float& v, std::ofstream& out_file)
{
    // TODO: Implement this function that tests whether the triangle
    // that's specified bt v0, v1 and v2 intersects with the ray (whose
    // origin is *orig* and direction is *dir*)
    // Also don't forget to update tnear, u and v.
    float t, b1, b2;

    Vector3f p0 = v0;
    Vector3f p1 = v1;
    Vector3f p2 = v2;
    Vector3f e1 = p1-p0;
    Vector3f e2 = p2-p0;
    Vector3f s = orig-p0;
    Vector3f s1 = crossProduct(dir, e2);
    Vector3f s2 = crossProduct(s, e1);
    float a = dotProduct(s1, e1);

    t = dotProduct(s2, e2)/a;
    b1 = dotProduct(s1, s)/a;
    b2 = dotProduct(s2, dir)/a;

    // out_file << "trangle: v0:" << v0 << " v1:" << v1 << "  v2:" << v2 << std::endl;
    // out_file << "trangle: st_v0:" << st_v0.x << " " << st_v0.y << " st_v1:" << st_v1.x << " " << st_v1.y<< "  st_v2:" << st_v2.x << " " << st_v2.y << std::endl;
    // out_file << "trangle: t:" << t << " b1:" << b1 << "  b2:" << b2 << std::endl;

    if (t >= 0 && b1 >= 0 && b2 >= 0 && (b1+b2 <= 1)) {
        // Vector3f intersection_cor = t*dir;
        // tnear = sqrt(pow(intersection_cor.x, 2) + pow(intersection_cor.y, 2) + pow(intersection_cor.z, 2));
        tnear = t;
        // u = (1-b1-b2)*st_v0.x + b1*st_v1.x+ b2*st_v2.x;
        // v = (1-b1-b2)*st_v0.y + b1*st_v1.y+ b2*st_v2.y;
        u = b1;
        v = b2;
        // out_file << "trangle intersect: intersection_cor: " << intersection_cor << "  u: " << u << "  v: " << v << std::endl;
        return true;
    }

    return false;
}

class MeshTriangle : public Object
{
public:
    MeshTriangle(const Vector3f* verts, const uint32_t* vertsIndex, const uint32_t& numTris, const Vector2f* st)
    {
        uint32_t maxIndex = 0;
        for (uint32_t i = 0; i < numTris * 3; ++i)
            if (vertsIndex[i] > maxIndex)
                maxIndex = vertsIndex[i];
        maxIndex += 1;
        vertices = std::unique_ptr<Vector3f[]>(new Vector3f[maxIndex]);
        memcpy(vertices.get(), verts, sizeof(Vector3f) * maxIndex);
        vertexIndex = std::unique_ptr<uint32_t[]>(new uint32_t[numTris * 3]);
        memcpy(vertexIndex.get(), vertsIndex, sizeof(uint32_t) * numTris * 3);
        numTriangles = numTris;
        stCoordinates = std::unique_ptr<Vector2f[]>(new Vector2f[maxIndex]);
        memcpy(stCoordinates.get(), st, sizeof(Vector2f) * maxIndex);
    }

    bool intersect(const Vector3f& orig, const Vector3f& dir, float& tnear, uint32_t& index,
                   Vector2f& uv, std::ofstream& out_file) const override
    {
        bool intersect = false;
        for (uint32_t k = 0; k < numTriangles; ++k)
        {
            const Vector3f& v0 = vertices[vertexIndex[k * 3]];
            const Vector3f& v1 = vertices[vertexIndex[k * 3 + 1]];
            const Vector3f& v2 = vertices[vertexIndex[k * 3 + 2]];

            const Vector2f& st_v0 = stCoordinates[vertexIndex[k * 3]];
            const Vector2f& st_v1 = stCoordinates[vertexIndex[k * 3 + 1]];
            const Vector2f& st_v2 = stCoordinates[vertexIndex[k * 3 + 2]];

            float t, u, v;

            if (rayTriangleIntersect(v0, v1, v2, st_v0, st_v1, st_v2, orig, dir, t, u, v, out_file) && t < tnear)
            {
                tnear = t;
                uv.x = u;
                uv.y = v;
                index = k;
                intersect |= true;
            }
        }

        return intersect;
    }

    void getSurfaceProperties(const Vector3f&, const Vector3f&, const uint32_t& index, const Vector2f& uv, Vector3f& N,
                              Vector2f& st) const override
    {
        const Vector3f& v0 = vertices[vertexIndex[index * 3]];
        const Vector3f& v1 = vertices[vertexIndex[index * 3 + 1]];
        const Vector3f& v2 = vertices[vertexIndex[index * 3 + 2]];
        Vector3f e0 = normalize(v1 - v0);
        Vector3f e1 = normalize(v2 - v1);
        N = normalize(crossProduct(e0, e1));
        const Vector2f& st0 = stCoordinates[vertexIndex[index * 3]];
        const Vector2f& st1 = stCoordinates[vertexIndex[index * 3 + 1]];
        const Vector2f& st2 = stCoordinates[vertexIndex[index * 3 + 2]];
        st = st0 * (1 - uv.x - uv.y) + st1 * uv.x + st2 * uv.y;
    }

    Vector3f evalDiffuseColor(const Vector2f& st, std::ofstream& out_file) const override
    {
        float scale = 5;
        // out_file << "evalDiffuseColor st: " << st.x << " " << st.y << std::endl;
        // out_file << "evalDiffuseColor fmodf(st.x * scale, 1) > 0.5: " << (fmodf(st.x * scale, 1) > 0.5) << 
        //     " fmodf(st.y * scale, 1) > 0.5: " << (fmodf(st.y * scale, 1) > 0.5)<< std::endl;
        float pattern = (fmodf(st.x * scale, 1) > 0.5) ^ (fmodf(st.y * scale, 1) > 0.5);
        return lerp(Vector3f(0.815, 0.235, 0.031), Vector3f(0.937, 0.937, 0.231), pattern);
    }

    std::unique_ptr<Vector3f[]> vertices;
    uint32_t numTriangles;
    std::unique_ptr<uint32_t[]> vertexIndex;
    std::unique_ptr<Vector2f[]> stCoordinates;
};
