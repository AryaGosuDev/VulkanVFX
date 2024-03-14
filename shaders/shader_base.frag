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

layout (location = 0 ) in vec2 texCoords;

layout (location = 0) out vec4 outColor;

void main() {
	//vec3 lightDirectionView = normalize ( lightPos - vec3(Position));
	//float lightDistance = length(lightPos - vec3(Position));

	//vec3 halfVector = normalize(lightDirectionView + vec3( vec3(ufbo.EyeDirection) - vec3(Position) )  );

	//float diffuse = max(0.0f, dot(Normal, lightDirectionView));
	//vec3 scatteredLight = vec3(ufbo.LightColor * diffuse) ;
	vec3 albedo = texture(albedoSampler, texCoords).rgb;
	outColor = vec4 (albedo, 1.0f );
}