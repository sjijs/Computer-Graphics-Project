#ifndef CUDA_VEC3_CUH
#define CUDA_VEC3_CUH

#include <cuda_runtime.h>
#include <curand_kernel.h>
#include <cmath>

// CUDA兼容的vec3类 - 使用__device__/__host__修饰符
class cuda_vec3 {
public:
    float e[3];

    __host__ __device__ cuda_vec3() : e{0, 0, 0} {}
    __host__ __device__ cuda_vec3(float e0, float e1, float e2) : e{e0, e1, e2} {}

    __host__ __device__ float x() const { return e[0]; }
    __host__ __device__ float y() const { return e[1]; }
    __host__ __device__ float z() const { return e[2]; }

    __host__ __device__ cuda_vec3 operator-() const { return cuda_vec3(-e[0], -e[1], -e[2]); }
    __host__ __device__ float operator[](int i) const { return e[i]; }
    __host__ __device__ float& operator[](int i) { return e[i]; }

    __host__ __device__ cuda_vec3& operator+=(const cuda_vec3& v) {
        e[0] += v.e[0];
        e[1] += v.e[1];
        e[2] += v.e[2];
        return *this;
    }

    __host__ __device__ cuda_vec3& operator*=(float t) {
        e[0] *= t;
        e[1] *= t;
        e[2] *= t;
        return *this;
    }

    __host__ __device__ cuda_vec3& operator/=(float t) {
        return *this *= 1.0f / t;
    }

    __host__ __device__ float length() const {
        return sqrtf(length_squared());
    }

    __host__ __device__ float length_squared() const {
        return e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
    }

    __host__ __device__ bool near_zero() const {
        const float s = 1e-8f;
        return (fabsf(e[0]) < s) && (fabsf(e[1]) < s) && (fabsf(e[2]) < s);
    }

    // GPU随机向量生成
    __device__ static cuda_vec3 random(curandState* local_rand_state) {
        return cuda_vec3(curand_uniform(local_rand_state),
                        curand_uniform(local_rand_state),
                        curand_uniform(local_rand_state));
    }

    __device__ static cuda_vec3 random(curandState* local_rand_state, float min, float max) {
        return cuda_vec3(min + (max - min) * curand_uniform(local_rand_state),
                        min + (max - min) * curand_uniform(local_rand_state),
                        min + (max - min) * curand_uniform(local_rand_state));
    }
};

// Type aliases
using cuda_point3 = cuda_vec3;
using cuda_color = cuda_vec3;

// === Vec3 Utility Functions ===

__host__ __device__ inline cuda_vec3 operator+(const cuda_vec3& u, const cuda_vec3& v) {
    return cuda_vec3(u.e[0] + v.e[0], u.e[1] + v.e[1], u.e[2] + v.e[2]);
}

__host__ __device__ inline cuda_vec3 operator-(const cuda_vec3& u, const cuda_vec3& v) {
    return cuda_vec3(u.e[0] - v.e[0], u.e[1] - v.e[1], u.e[2] - v.e[2]);
}

__host__ __device__ inline cuda_vec3 operator*(const cuda_vec3& u, const cuda_vec3& v) {
    return cuda_vec3(u.e[0] * v.e[0], u.e[1] * v.e[1], u.e[2] * v.e[2]);
}

__host__ __device__ inline cuda_vec3 operator*(float t, const cuda_vec3& v) {
    return cuda_vec3(t * v.e[0], t * v.e[1], t * v.e[2]);
}

__host__ __device__ inline cuda_vec3 operator*(const cuda_vec3& v, float t) {
    return t * v;
}

__host__ __device__ inline cuda_vec3 operator/(const cuda_vec3& v, float t) {
    return (1.0f / t) * v;
}

__host__ __device__ inline float dot(const cuda_vec3& u, const cuda_vec3& v) {
    return u.e[0] * v.e[0] + u.e[1] * v.e[1] + u.e[2] * v.e[2];
}

__host__ __device__ inline cuda_vec3 cross(const cuda_vec3& u, const cuda_vec3& v) {
    return cuda_vec3(u.e[1] * v.e[2] - u.e[2] * v.e[1],
                     u.e[2] * v.e[0] - u.e[0] * v.e[2],
                     u.e[0] * v.e[1] - u.e[1] * v.e[0]);
}

__host__ __device__ inline cuda_vec3 unit_vector(const cuda_vec3& v) {
    return v / v.length();
}

__host__ __device__ inline cuda_vec3 reflect(const cuda_vec3& v, const cuda_vec3& n) {
    return v - 2.0f * dot(v, n) * n;
}

__host__ __device__ inline cuda_vec3 refract(const cuda_vec3& uv, const cuda_vec3& n, float etai_over_etat) {
    float cos_theta = fminf(dot(-uv, n), 1.0f);
    cuda_vec3 r_out_perp = etai_over_etat * (uv + cos_theta * n);
    cuda_vec3 r_out_parallel = -sqrtf(fabsf(1.0f - r_out_perp.length_squared())) * n;
    return r_out_perp + r_out_parallel;
}

// GPU随机采样函数
__device__ inline cuda_vec3 random_in_unit_sphere(curandState* local_rand_state) {
    cuda_vec3 p;
    do {
        p = cuda_vec3::random(local_rand_state, -1.0f, 1.0f);
    } while (p.length_squared() >= 1.0f);
    return p;
}

__device__ inline cuda_vec3 random_unit_vector(curandState* local_rand_state) {
    return unit_vector(random_in_unit_sphere(local_rand_state));
}

__device__ inline cuda_vec3 random_on_hemisphere(const cuda_vec3& normal, curandState* local_rand_state) {
    cuda_vec3 on_unit_sphere = random_unit_vector(local_rand_state);
    if (dot(on_unit_sphere, normal) > 0.0f)
        return on_unit_sphere;
    else
        return -on_unit_sphere;
}

__device__ inline cuda_vec3 random_in_unit_disk(curandState* local_rand_state) {
    cuda_vec3 p;
    do {
        p = cuda_vec3(curand_uniform(local_rand_state) * 2.0f - 1.0f,
                     curand_uniform(local_rand_state) * 2.0f - 1.0f,
                     0.0f);
    } while (p.length_squared() >= 1.0f);
    return p;
}

// Gamma校正
__host__ __device__ inline float linear_to_gamma(float linear_component) {
    if (linear_component > 0)
        return sqrtf(linear_component);
    return 0;
}

#endif
