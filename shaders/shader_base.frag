#version 450
#extension GL_ARB_gpu_shader_int64 : enable

#define MAX_STEPS 32
#define MAX_DIST 1000.
#define SURF_DIST .01

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

layout( push_constant ) uniform PushConstants {
	vec3 avatarPos ;
	float time ;
} pc;

layout(set = 0, binding = 2) uniform sampler2D albedoSampler;
layout(set = 0, binding = 3) uniform sampler2D depthInfoSampler;
layout(set = 0, binding = 4) uniform sampler2D normalSampler;
layout(set = 0, binding = 5) uniform sampler2D worldCoordSampler;
layout(set = 0, binding = 6) uniform sampler2D colorMapRT;
layout(set = 0, binding = 7) uniform sampler2D depthMapRT;

layout (location = 0 ) in vec2 texCoords;
layout (location = 1 ) in vec3 lightPosIn ;
layout (location = 2 ) in vec3 camPos;
layout (location = 3 ) in mat3 invRotCam ;
layout (location = 7 ) in mat4 invProjMatrix ;
layout (location = 13 ) in mat4 invViewMatrix ;
layout (location = 20 ) in mat3 camRot;

layout (location = 0) out vec4 outColor;

void main() {

	vec3 normal = texture(normalSampler, texCoords).rgb;
	vec3 worldPosition =  texture(worldCoordSampler, texCoords).rgb;
	vec3 albColor =  texture(albedoSampler, texCoords).rgb;
	float depthToPoint = length ( worldPosition - camPos ) ;
	vec3 RT_color = texture ( colorMapRT, texCoords).rgb;
	float RT_dist = texture ( depthMapRT, texCoords).r;
	float depthInfoP = texture ( depthInfoSampler, texCoords).r;

	vec3 lightDirectionView = normalize ( lightPosIn - vec3(worldPosition));

	float diffuse = max(0.0f, dot(normal, lightDirectionView));
	vec3 scatteredLight = vec3(albColor * diffuse) ;
	

	//if ( RT_dist < 100.0f && RT_dist < depthInfoP  ) 
	if ( RT_dist < 100.0f && RT_dist < depthInfoP    ) 

		outColor = vec4 (RT_color, 1.0f );

	
	else {
		outColor = vec4 (scatteredLight, 1.0f );
	}
	
		
}