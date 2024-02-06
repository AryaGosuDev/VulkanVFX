#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
	mat4 normalMatrix ;
	vec4 lightPos ;
} ubo;
   
layout (location = 0 ) in vec3 color;
layout (location = 1 ) in vec3 VertexNormal;
layout (location = 2 ) in vec3 position;

layout (location = 0 ) out vec3 Position; // adding position, so we know where we are

void main() {

    vec4 worldPosition = ubo.model * vec4(position, 1.0);
    Position = worldPosition.xyz ;
    gl_Position = gl_Position = ubo.proj * ubo.view * worldPosition;
}