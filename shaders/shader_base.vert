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
layout (location = 2 ) out vec3 camPos;
layout (location = 3 ) out mat3 invRotCam ;
layout (location = 7 ) out mat4 invProjMatrix;
layout (location = 13 ) out mat4 invViewMatrix ;
layout (location = 20 ) out mat3 camRot;

void main() {
    mat3 invRot = transpose(mat3(ubo.view));
    invProjMatrix = inverse( ubo.proj);
    invViewMatrix = inverse( ubo.view);
    vec3 negTrans = -vec3(ubo.view[3]);
    camPos = invRot * negTrans;

    camRot = mat3(ubo.view);
    invRotCam = invRot;

    lightPosOut = vec3(ubo.lightPos);
	texCoordsOut = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(texCoordsOut * 2.0f - 1.0f, 0.0f, 1.0f);
}  