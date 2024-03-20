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
   
layout (location = 0 ) out vec2 texCoordsOut;
layout (location = 1 ) out vec3 lightPosOut ;

void main() {
    lightPosOut = vec3(ubo.lightPos);
	texCoordsOut = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(texCoordsOut * 2.0f - 1.0f, 0.0f, 1.0f);
}  