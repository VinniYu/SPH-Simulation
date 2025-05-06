#ifndef SETTINGS_H
#define SETTINGS_H

#include <cmath>
#include <vector>
#include <iostream>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


typedef float REAL;

struct vec2 {
    REAL x;
    REAL y;

    vec2() : x(0.0), y(0.0) {}
    vec2(REAL X, REAL Y) : x(X), y(Y) {}

    vec2 operator+(const vec2& other) const {
        return vec2(x + other.x, y + other.y);
    }

    vec2 operator-(const vec2& other) const {
        return vec2(x - other.x, y - other.y);
    }

    vec2 operator*(REAL scalar) const {
        return vec2(x * scalar, y * scalar);
    }

    vec2 operator/(REAL scalar) const {
        return vec2(x / scalar, y / scalar);
    }

    vec2& operator+=(const vec2& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    vec2& operator-=(const vec2& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    REAL dot(const vec2& other) const {
        return x*other.x + y*other.y;
    }

    REAL length() const {
        return sqrt(x*x + y*y);
    }

    vec2 normalize() const {
        REAL len = length();
        if (len < 1e-8) return vec2(0.0, 0.0);
        return vec2(x / len, y / len);
    }
};

struct vec3 {
    REAL x;
    REAL y;
    REAL z;

    vec3() : x(0.0), y(0.0), z(0.0) {}
    vec3(REAL X, REAL Y, REAL Z) : x(X), y(Y), z(Z) {}

    vec3 operator+(const vec3& other) const {
        return vec3(x + other.x, y + other.y, z + other.z);
    }

    vec3 operator-(const vec3& other) const {
        return vec3(x - other.x, y - other.y, z - other.z);
    }

    vec3 operator*(REAL scalar) const {
        return vec3(x * scalar, y * scalar, z* scalar);
    }

    REAL dot(const vec3& other) const {
        return x*other.x + y*other.y + z*other.z;
    }

    REAL length() const {
        return sqrt(x*x + y*y + z*z);
    }

    vec3 normalize() const {
        REAL len = length();
        if (len < 1e-8) return vec3(0.0, 0.0, 0.0);
        return vec3(x / len, y / len, z / len);
    }
};


#endif