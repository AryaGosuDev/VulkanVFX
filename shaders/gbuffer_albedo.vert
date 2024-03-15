#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
	mat4 normalMatrix ;
	vec4 lightPos ;
    mat4 orthoProj;
	mat4 LightDepthView;
} ubo;
    
layout (location = 0 ) in vec3 color;
layout (location = 1 ) in vec3 VertexNormal;
layout (location = 2 ) in vec3 position;

layout (location = 0 ) out vec3 fragColor;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(position, 1.) ;
    fragColor = color;
}