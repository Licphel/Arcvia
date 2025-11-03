#pragma once

namespace arc {

// immutable 2d vector
struct vec2 {
    double x = 0;
    double y = 0;

    vec2();
    vec2(double x, double y);

    vec2 operator+(const vec2& v) const;
    vec2 operator-(const vec2& v) const;
    vec2 operator*(const vec2& v) const;
    vec2 operator/(const vec2& v) const;
    vec2 operator+(double s) const;
    vec2 operator-(double s) const;
    vec2 operator*(double s) const;
    vec2 operator/(double s) const;

    double length() const;
    double length_powered() const;
    static vec2 from(double len, double rad);
    vec2 normal() const;
    static double dot(const vec2& v1, const vec2& v2);
};

// immutable 3d vector
struct vec3 {
    double x = 0;
    double y = 0;
    double z = 0;

    vec3();
    vec3(double x, double y, double z);

    vec3 operator+(const vec3& v) const;
    vec3 operator-(const vec3& v) const;
    vec3 operator*(const vec3& v) const;
    vec3 operator/(const vec3& v) const;
    vec3 operator+(double s) const;
    vec3 operator-(double s) const;
    vec3 operator*(double s) const;
    vec3 operator/(double s) const;

    double length() const;
    double length_powered() const;
    vec3 normal() const;
    static double dot(const vec3& v1, const vec3& v2);
};

// 2x3 matrix for 2d transformations (called an affine matrix)
struct transform {
    float m00 = 1.0, m01 = 0.0, m02 = 0.0;
    float m10 = 0.0, m11 = 1.0, m12 = 0.0;

    transform();
    transform(float m00, float m01, float m02, float m10, float m11, float m12);

    transform& identity();
    transform& load(const transform& o);
    transform& orthographic(float left, float right, float bottom, float top);
    transform& mul(const transform& o);
    transform& invert();
    transform& translate(float tx, float ty);
    transform& scale(float sx, float sy);
    transform& rotate(float rad);
    transform& shear(float shx, float shy);
    void apply(float& x, float& y) const;
    void apply(vec2& v) const;
    float det() const;
};

// axis-aligned rectangle (called quad)
struct quad {
    double x = 0, y = 0, width = 0, height = 0;

    quad();
    quad(double x, double y, double w, double h);

    static quad center(double x, double y, double w, double h);
    static quad corner(double x, double y, double w, double h);
    // get intersection of two quads
    static quad intersection_of(const quad& q1, const quad& q2);
    // check if two quads intersect
    static bool intersect(const quad& q1, const quad& q2);
    static bool contain(const quad& q, const vec2& v);

    quad& translate(double nx, double ny);
    quad& locate_center(double nx, double ny);
    quad& locate_corner(double nx, double ny);
    quad& resize(double nw, double nh);
    // inflate the quad based on its center
    quad& inflate(double nw, double nh);
    quad& scale(double nw, double nh);

    double center_x() const;
    double center_y() const;
    double corner_x() const;
    double corner_y() const;
    double prom_x() const;
    double prom_y() const;
    double area() const;
};

}  // namespace arc
