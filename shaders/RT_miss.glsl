#version 450
#extension GL_NV_ray_tracing : require
layout(location = 0) rayPayloadInNV vec3 payload;

void main() {
    payload = vec3(0.0, 0.0, 0.0);  // Background color
}
