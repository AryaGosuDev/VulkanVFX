#version 450
#extension GL_NV_ray_tracing : require
layout(location = 0) rayPayloadNV vec3 payload;

void main() {
    vec3 rayOrigin = ...;
    vec3 rayDirection = ...;
    traceRayNV(..., rayOrigin, rayDirection, ...);
}
