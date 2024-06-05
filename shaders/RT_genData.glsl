#version 460
#extension GL_EXT_ray_tracing : require

struct RayGenData {
    vec3 color;
    float depth;
};

float tMin = 0.001;
float tMax = 10000.0;