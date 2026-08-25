#include "vector.h"

Vector::Vector() : data_{} {
}

Vector::Vector(double x, double y, double z) : data_{x, y, z} {
}

double& Vector::operator[](size_t ind) {
    return data_[ind];
}

double Vector::operator[](size_t ind) const {
    return data_[ind];
}

void Vector::Normalize() {
    double norma = std::sqrt(data_[0] * data_[0] + data_[1] * data_[1] + data_[2] * data_[2]);
    if (norma != 0) {
        data_[0] /= norma;
        data_[1] /= norma;
        data_[2] /= norma;
    }
}

Vector operator-(const Vector& vec) {
    return Vector(-vec[0], -vec[1], -vec[2]);
}

Vector& operator+=(Vector& lhv, const Vector& rhv) {
    lhv.data_[0] += rhv.data_[0];
    lhv.data_[1] += rhv.data_[1];
    lhv.data_[2] += rhv.data_[2];
    return lhv;
}

Vector& operator*=(Vector& lhv, double scalar) {
    lhv.data_[0] *= scalar;
    lhv.data_[1] *= scalar;
    lhv.data_[2] *= scalar;
    return lhv;
}

Vector& operator-=(Vector& lhv, const Vector& rhv) {
    lhv += (-rhv);
    return lhv;
}

Vector& operator/=(Vector& lhv, double scalar) {
    lhv *= (1.0 / scalar);
    return lhv;
}

Vector operator+(const Vector& lhv, const Vector& rhv) {
    Vector res = lhv;
    res += rhv;
    return res;
}

Vector operator-(const Vector& lhv, const Vector& rhv) {
    Vector res = lhv;
    res -= rhv;
    return res;
}

Vector operator*(const Vector& lhv, double scalar) {
    Vector res = lhv;
    res *= scalar;
    return res;
}

Vector operator*(double scalar, const Vector& rhv) {
    Vector res = rhv;
    res *= scalar;
    return res;
}

Vector operator/(const Vector& lhv, double scalar) {
    Vector res = lhv;
    res /= scalar;
    return res;
}

bool operator==(const Vector& lhv, const Vector& rhv) {
    return (lhv[0] == rhv[0]) && (lhv[1] == rhv[1]) && (lhv[2] == rhv[2]);
}

bool operator!=(const Vector& lhv, const Vector& rhv) {
    return !(lhv == rhv);
}

double DotProduct(const Vector& a, const Vector& b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

// count minors, expansion along the first row - i j k
Vector CrossProduct(const Vector& a, const Vector& b) {
    return Vector(a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]);
}

double Length(const Vector& v) {
    return sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}
