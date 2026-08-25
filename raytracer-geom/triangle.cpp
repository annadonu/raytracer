#include "triangle.h"

Triangle::Triangle(const Vector& a, const Vector& b, const Vector& c) : vertices_{a, b, c} {
}

const Vector& Triangle::operator[](size_t ind) const {
    return vertices_[ind];
}

double Triangle::Area() const {
    // 2 vectors out of 1 vertice
    Vector vec1 = vertices_[1] - vertices_[0];
    Vector vec2 = vertices_[2] - vertices_[0];

    // s = 1/2 * crossProduct
    return 0.5 * Length(CrossProduct(vec1, vec2));
}
