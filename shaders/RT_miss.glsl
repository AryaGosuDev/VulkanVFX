#version 460
#extension GL_NV_ray_tracing : require

#include "RT_genData.glsl"

layout(location = 0) rayPayloadInNV RayGenData genData;

void main() {
    genData.color = vec3(0.0, 0.0, 0.0);
    genData.depth = tMax;
}
