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

layout( push_constant ) uniform mainCharLocation {
	mat4 avatarPos;
	mat4 rotation;
	bool avatarMotion;
} pc;
   
layout (location = 0 ) in vec3 color;
layout (location = 1 ) in vec3 VertexNormal;
layout (location = 2 ) in vec3 position;

layout (location = 0) out float inFragDepth ;
 
void main() {
    mat4 modelTemp = ubo.model ;
    if ( pc.avatarMotion == true ) modelTemp = pc.avatarPos * pc.rotation;
    gl_Position = ubo.proj * ubo.view * modelTemp * vec4(position, 1.0);
    inFragDepth = gl_Position.z ;
}