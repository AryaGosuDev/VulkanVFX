#version 460
#extension GL_EXT_ray_tracing : require

#include "RT_genData.glsl"

layout(location = 0) rayPayloadInNV RayGenData genData;
layout(location = 1) hitAttributeEXT vec3 attribs;

// Define the sphere parameters
const vec3 sphereCenter = vec3(0.0, 0.0, 0.0);
const float sphereRadius = 1.0;

void main() {
    vec3 rayO = gl_WorldRayOriginEXT;
    vec3 rayD = gl_WorldRayDirectionEXT;

    vec3 oc = rayO - sphereCenter;
    float b = dot(oc, rayDir);
    float c = dot(oc, oc) - sphereRadius * sphereRadius;
    float discriminant = b * b - c;

    if (discriminant > 0.0) {
        float t = -b - sqrt(discriminant);
        if (t > 0.0) // Report intersection
            reportIntersectionEXT(t, 0);
    }
}
