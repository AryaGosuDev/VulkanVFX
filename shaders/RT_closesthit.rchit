#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#include "RT_genData.h"

vec3 palette( float t ) {
    vec3 a = vec3(0.5, 0.5, 0.5);
    vec3 b = vec3(0.5, 0.5, 0.5);
    vec3 c = vec3(1.0, 1.0, 1.0);
    vec3 d = vec3(0.263,0.416,0.557);

    return a + b*cos( 6.28318*(c*t+d) );
}

layout(push_constant) uniform PushConstants {
    vec3 avatarPos;
    float timeStamp;
} pc;

layout(location = 0) rayPayloadInEXT RayGenData genData;
hitAttributeEXT vec3 attribs;

void main() {
    vec3 rayO = gl_WorldRayOriginEXT;
    vec3 rayD = gl_WorldRayDirectionEXT;
    vec3 hitPos = attribs; // get intersection position

    // calculate UV coordinates based on the hit position on the sphere
    vec3 normal = normalize(hitPos - vec3(2.0, 2.0, 2.0)); // normal at the hit point
    float u = 0.5 + atan(normal.z, normal.x) / (2.0 * 3.141592653589793);
    float v = 0.5 - asin(normal.y) / 3.141592653589793;
    vec2 uv = vec2(u, v);

    vec2 uv0 = uv * 2.0; // scale UVs for more patterns
    vec3 finalColor = vec3(0.0);

    for (float i = 0.0; i < 4.0; i++) {
        uv = fract(uv * 1.5) - 0.5;

        float d = length(uv) * exp(-length(uv0));

        vec3 col = palette(length(uv0) + i * .4 + pc.timeStamp * .1);

        d = sin(d * 8.0 + pc.timeStamp * .1) / 8.0;
        d = abs(d);

        d = pow(0.01 / (d  ), 1.2);

        finalColor += col * d;
    }

    //genData.color = vec3(cos(pc.timeStamp * .01 ), 0.0, sin ( pc.timeStamp * .01 )); // blue color
    genData.color = finalColor ;
    genData.depth = gl_HitTEXT;
}
