#version 450

layout(set = 0, binding = 1) uniform UniformFragBufferObject {
	vec4 Ambient;
	vec4 LightColor;
	float Shininess;
	float Strength;
	vec4 EyeDirection;
	float ConstantAttenuation; // attenuation coefficients
	float LinearAttenuation;
	float QuadraticAttenuation;
	mat4 viewMatrix ;
	mat4 eyeViewMatrix;
} ufbo;


layout (location = 0 ) in vec2 texCoords;

layout (location = 0) out vec4 outColor;

void main() {
	outColor = vec4 ( 1.0f,0.0f,0.0f, 1.0f );
}