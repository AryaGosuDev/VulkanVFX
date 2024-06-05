#version 450
#extension GL_NV_ray_tracing : require

layout(set = 0, binding = 0) uniform UniformBufferObject{
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 normalMatrix;
    vec4 lightPos;
    mat4 orthoProj;
    mat4 LightDepthView;
} ubo;

#include "RT_genData.glsl"

layout(set = 0, binding = 1, rgba8) uniform image2D outputImage;
layout(set = 0, binding = 2, r32f) uniform image2D depthImage;
layout(set = 0, binding = 3, r32f) uniform accelerationStructureEXT topLevelAS;


layout(location = 0) rayPayloadInNV RayGenData genData;

void main() {

    vec4 rayO = inverse ( ubo.view )  * vec4(0, 0, 0, 1);
    vec2 ndc = vec2(gl_LaunchIDEXT.xy) / vec2(gl_LaunchSizeEXT.xy) * 2.0 - 1.0;
    //ndc.y = -ndc.y;

    vec4 target = inverse ( ubo.proj) * vec4(ndc.x, ndcd.y, 1, 1);
    vec4 rayD = inverse(ubo.view) * vec4(normalize(target.xyz), 0);

    uint  rayFlags = gl_RayFlagsOpaqueEXT;


    genData.color = vec3(0.0, 0.0, 0.0); // Initialize payload
    genData.depth = 1.0;


    traceRayEXT(topLevelAS,     // acceleration structure
        rayFlags,       // rayFlags
        0xFF,           // cullMask
        0,              // sbtRecordOffset
        0,              // sbtRecordStride
        0,              // missIndex
        rayO.xyz,     // ray origin
        tMin,           // ray min range
        rayD.xyz,  // ray direction
        tMax,           // ray max range
        0               // payload (location = 0)
    );

    imageStore(outputImage, ivec2(gl_LaunchIDEXT.xy), vec4(genData.color, 1.0));
    imageStore(depthImage, ivec2(gl_LaunchIDEXT.xy), depth);
}
}
