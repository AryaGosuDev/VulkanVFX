#version 460
#extension GL_EXT_ray_tracing : require

#include "RayGenData.glsl"

layout(location = 0) rayPayloadInNV RayGenData genData;
layout(location = 1) hitAttributeEXT vec3 attribs;

void main() {
    // In this simple example, we just set the color to red and store the depth
    genData.color = vec3(0.0, 0.0, 1.0); // Red color
    genData.depth = gl_HitTNV;
}
