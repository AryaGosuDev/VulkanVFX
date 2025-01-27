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

layout (location = 0 ) out vec3 Position; // adding position, so we know where we are

void main() {
    mat4 modelTemp = ubo.model ;

    mat4 I = mat4 ( vec4(1.0,0.0,0.0,0.0) ,vec4(0.0,1.0,0.0,0.0),vec4(0.0,0.0,1.0,0.0),vec4(0.0,0.0,0.0,1.0) ) ;

    const int avatarBool = ( pc.avatarMotion == true) ? 1 : 0;
    float isMotionTrue = 1.0 - abs(sign(avatarBool - 1));
    
    modelTemp = (ubo.model * (I * (1.0 - isMotionTrue )))   + ( (pc.avatarPos * (I * isMotionTrue)) * pc.rotation ) ;
    //if ( pc.avatarMotion == true ) modelTemp = pc.avatarPos * pc.rotation;
    
    vec4 worldPosition = modelTemp * vec4(position, 1.0);
    Position = worldPosition.xyz ;
    gl_Position = ubo.proj * ubo.view * worldPosition;
}