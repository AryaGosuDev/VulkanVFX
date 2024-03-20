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

layout(set = 0, binding = 2) uniform sampler2D albedoSampler;
layout(set = 0, binding = 3) uniform sampler2D depthInfoSampler;
layout(set = 0, binding = 4) uniform sampler2D normalSampler;
layout(set = 0, binding = 5) uniform sampler2D worldCoordSampler;
layout(set = 0, binding = 6) uniform sampler2D lightDepthMap;

layout (location = 0 ) in vec2 texCoords;
layout (location = 1 ) in vec3 lightPosIn ;

layout (location = 0) out vec4 outColor;

void main() {

	vec3 normal = texture(normalSampler, texCoords).rgb;
	vec3 worldPosition =  texture(worldCoordSampler, texCoords).rgb;
	vec3 albColor =  texture(albedoSampler, texCoords).rgb;

	vec3 lightDirectionView = normalize ( lightPosIn - vec3(worldPosition));

	float diffuse = max(0.0f, dot(normal, lightDirectionView));
	vec3 scatteredLight = vec3(albColor * diffuse) ;

	outColor = vec4 (scatteredLight, 1.0f );

	//float depthVal = texture(depthInfoSampler, texCoords).r;
	//outColor = vec4 (depthVal/10.,0.,0., 1.0f );
}