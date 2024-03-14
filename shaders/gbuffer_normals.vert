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

layout (location = 0 ) out vec3 fragColor;
layout (location = 1 ) out vec3 Normal;
layout (location = 2 ) out vec4 Position; // adding position, so we know where we are
layout (location = 3 ) out vec3 LightPos;
layout (location = 4 ) out vec3 NormalView;
 
void main() {

	// Apply the reflect view matrix to the vertex position
    vec4 reflectVertexPosition = ubo.view * vec4(position, 1.0);
    Position = reflectVertexPosition.xyzw;
     
    // Output the transformed vertex position
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(position, 1.0);
    // Perform any other necessary vertex shader calculations
	
	LightPos = vec3( ubo.view * ubo.lightPos);
    fragColor = color;
}