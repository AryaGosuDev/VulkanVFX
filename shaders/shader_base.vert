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

void main() {
	texCoordsOut = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(texCoordsOut * 2.0f - 1.0f, 0.0f, 1.0f);

    //vec2 positions[3] = vec2[3](vec2(-0.5, -0.5), vec2(0.5, -0.5), vec2(0.0, 0.5));
    //texCoordsOut = vec2( 1.0, 1.0);
    //gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}