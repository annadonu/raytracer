#pragma once

#include <array>
#include <cstddef>
#include <cmath>

class Vector {
public:
    Vector();
    Vector(double x, double y, double z);

    double& operator[](size_t ind);
    double operator[](size_t ind) const;

    void Normalize();

    friend Vector& operator+=(Vector& lhv, const Vector& rhv);
    friend Vector& operator*=(Vector& lhv, double scalar);

private:
    std::array<double, 3> data_;
};

Vector operator-(const Vector& vec);

Vector& operator-=(Vector& lhv, const Vector& rhv);
Vector& operator/=(Vector& lhv, double scalar);

Vector operator+(const Vector& lhv, const Vector& rhv);
Vector operator-(const Vector& lhv, const Vector& rhv);
Vector operator*(const Vector& lhv, double scalar);
Vector operator*(double scalar, const Vector& rhv);
Vector operator/(const Vector& lhv, double scalar);

bool operator==(const Vector& lhv, const Vector& rhv);
bool operator!=(const Vector& lhv, const Vector& rhv);

double DotProduct(const Vector& a, const Vector& b);
Vector CrossProduct(const Vector& a, const Vector& b);
double Length(const Vector& v);
