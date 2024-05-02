#version 450
//#extension GL_EXT_shader_realtime_clock : require
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
layout(set = 0, binding = 6) uniform sampler2D lightDepthMap;

layout (location = 0 ) in vec2 texCoords;
layout (location = 1 ) in vec3 lightPosIn ;
layout (location = 2 ) in vec3 camPos;
layout (location = 3 ) in mat3 invRotCam ;
layout (location = 7 ) in mat4 invProjMatrix ;
layout (location = 13 ) in mat4 invViewMatrix ;
layout (location = 20 ) in mat3 camRot;

layout (location = 0) out vec4 outColor;

vec3 palette( float t ) {
    vec3 a = vec3(0.5, 0.5, 0.5);
    vec3 b = vec3(0.5, 0.5, 0.5);
    vec3 c = vec3(1.0, 1.0, 1.0);
    vec3 d = vec3(0.263,0.416,0.557);

    return a + b*cos( 6.28318*(c*t+d) );
}

vec3 finalColorPattern ( vec2 uv0 ) {
		vec3 finalColor ;
		for (float i = 0.0; i < 4.0; i++) {
			vec2 uv = fract(uv0 * 1.5) - 0.5;

			float d = length(uv) * exp(-length(uv0));

			vec3 col = palette(length(uv0) + i*.4 + pc.time*.001);

			d = sin(d*8. +  pc.time*0.001)/8.;
			d = abs(d);

			d = pow(0.01 / d, 1.2);

			finalColor += col * d;
		}
		return finalColor;
}

// Disk SDF
float sdDisk( in vec3 p, in vec3 n, in float r ) {
    vec3 d = dot(p, n) * n;
    vec3 o = p - d;
    o -= normalize( o ) * min( length( o ), r );
    return length( d + o );
}

float GetDist(vec3 p) {
	//uint64_t time = clockRealtimeEXT();
	float displacement = sin(5.0 * p.x + .01*pc.time ) * sin(5.0 * p.y + .01 * pc.time) * sin(5.0 * p.z + .01 * pc.time) * 0.25;
	vec3 sphereCenter = vec3 ( 0.0, 0.0, 5.0 );
	return (length(p - sphereCenter) - 1.0) -displacement ;
}

float RayMarch(vec3 rayO, vec3 rayD) {
	float dO=0.0;
   
    for(int i=0; i<MAX_STEPS; i++) {
		vec3 p = rayO + rayD*dO;
		//float dS = GetDist(p);
		float dS = sdDisk ( p - vec3(0.0f,3.0f,0.0f) , normalize(vec3( 0.0,1.0,0.0 )) , 1.0f);
		if ( dS < SURF_DIST ) return dO;
		dO += dS;
		if(dO > MAX_DIST ) break;
    }
   
    return dO;
}

vec3 getCameraSpaceCoord(vec2 fragCoord) {
    // Convert fragment coordinates to normalized device coordinates (NDC)
    //vec2 ndc = (fragCoord / pc.screenResolution) * 2.0 - 1.0;
	vec2 ndc = (fragCoord) * 2.0 - 1.0;
    //ndc.y *= -1;  // Flip y if necessary depending on the coordinate system

    // Apply inverse projection to get coordinates in camera space
    vec4 cameraSpacePos = invProjMatrix * vec4(ndc.x, ndc.y, 1.0, 1.0); // z = 1 for forward direction
    cameraSpacePos /= cameraSpacePos.w; // Convert from homogeneous coordinates

    return cameraSpacePos.xyz;
}

vec3 getRayDirection(vec2 fragCoord) {

    vec3 cameraSpaceCoord = getCameraSpaceCoord(fragCoord);

    // Transform camera space coordinate to world space to get the world position of the pixel
    vec3 pixelWorldPosition = (invRotCam * cameraSpaceCoord).xyz;

    // The ray direction is the vector from the camera position to the pixel position in world space
    vec3 rayDirection = normalize(pixelWorldPosition - camPos);
    
    return rayDirection;
}

void main() {

	vec3 normal = texture(normalSampler, texCoords).rgb;
	vec3 worldPosition =  texture(worldCoordSampler, texCoords).rgb;
	vec3 albColor =  texture(albedoSampler, texCoords).rgb;
	float depthToPoint = length ( worldPosition - camPos ) ;

	vec3 lightDirectionView = normalize ( lightPosIn - vec3(worldPosition));

	float diffuse = max(0.0f, dot(normal, lightDirectionView));
	vec3 scatteredLight = vec3(albColor * diffuse) ;
	vec2 tempScreenRes = vec2 ( 1600.0, 1200.0 );

	vec3 rayO = camPos ; // get cameras position
	vec2 uv = (texCoords - 0.5) * 2.0;
	//vec2 uv = (texCoords * 2.0) - 1.0;
	//vec2 uv = (2.0f * texCoords - tempScreenRes)/tempScreenRes.y;
	float aspectRatio = 1600.0 /1200.0;
	//vec3 rayDir = normalize(vec3(uv, 1.0));
	vec3 rayDir = normalize(vec3(uv.x   , uv.y , 1.0));
	//vec3 newRayDir = getRayDirection(gl_FragCoord.xy);
	float d = RayMarch(rayO, rayDir);

	//hit sdf
	if( d < MAX_DIST && d < depthToPoint  ) {
		vec3 finalColor = finalColorPattern ( uv ) ;
		vec3 blendedColor = mix(scatteredLight, vec3(0.0,0.0,1.0), 0.5);
		outColor = vec4(finalColor,1.0f);
		//outColor = vec4(blendedColor,1.0f);
	}
	else {
		outColor = vec4 (scatteredLight, 1.0f );
		//outColor = vec4 (rayDir, 1.0 );
	}
}
/*
uniform mat4 invViewMatrix;  // Inverse of the view matrix
uniform mat4 invProjMatrix;  // Inverse of the projection matrix
uniform vec2 screenResolution;  // Screen resolution (width, height)

vec3 getCameraSpaceCoord(vec2 fragCoord) {
    // Convert fragment coordinates to normalized device coordinates (NDC)
    vec2 ndc = (fragCoord / screenResolution) * 2.0 - 1.0;
    ndc.y *= -1;  // Flip y if necessary depending on the coordinate system

    // Apply inverse projection to get coordinates in camera space
    vec4 cameraSpacePos = invProjMatrix * vec4(ndc.x, ndc.y, 1.0, 1.0); // z = 1 for forward direction
    cameraSpacePos /= cameraSpacePos.w; // Convert from homogeneous coordinates

    return cameraSpacePos.xyz;
}

vec3 getRayDirection(vec2 fragCoord) {
    vec3 cameraPosition = (invViewMatrix * vec4(0.0, 0.0, 0.0, 1.0)).xyz; // Camera position in world space
    vec3 cameraSpaceCoord = getCameraSpaceCoord(fragCoord);

    // Transform camera space coordinate to world space to get the world position of the pixel
    vec3 pixelWorldPosition = (invViewMatrix * vec4(cameraSpaceCoord, 1.0)).xyz;

    // The ray direction is the vector from the camera position to the pixel position in world space
    vec3 rayDirection = normalize(pixelWorldPosition - cameraPosition);
    
    return rayDirection;
}

void main() {
    vec3 rayDir = getRayDirection(gl_FragCoord.xy);
    // Now use rayDir for ray tracing or ray marching
}
*/
