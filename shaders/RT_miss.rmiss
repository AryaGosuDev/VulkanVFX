#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#include "RT_genData.h"

layout(location = 0) rayPayloadInEXT RayGenData genData;

void main() {
    genData.color = vec3(0.0, 0.0, 0.0);
    genData.depth = tMax;
}
