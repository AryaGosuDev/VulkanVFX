#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#include "RT_genData.h"

layout(location = 0) rayPayloadInEXT RayGenData genData;

void main() {
    // In this simple example, we just set the color to red and store the depth
    genData.color = vec3(0.0, 0.0, 1.0); // Red color
    genData.depth = gl_HitTEXT;
}
