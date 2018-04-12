#version 450
#extension GL_ARB_separate_shader_objects : enable

const int PointLightCount = 10;
const int SpotLightCount = 10;
const float PI = 3.14159265359;

layout(set = 0, binding = 0) uniform GlobalData {
	float time;
} global;

layout(set = 0, binding = 1) uniform CameraData {
	mat4 projView;
	mat4 view;
	vec3 cameraDir;
	vec3 cameraPos;
} cam;

struct DirectionalLight {
	vec3 direction;
	float intensity;
	vec3 color;
};

struct PointLight {
	vec3 position;
	float intensity;
	vec3 color;
};

struct SpotLight {
	vec3 position;
	vec3 color;
	float attenuation;
	float cutoff;
};

layout(set = 1, binding = 0) uniform DirectionalLightData {
	DirectionalLight light[1];
} sun;

layout(set = 1, binding = 1) uniform PointLightData {
	PointLight lights[PointLightCount];
} point; 

layout(set = 1, binding = 2) uniform SpotLightData { 
	SpotLight lights[SpotLightCount];
} spot;

//texture sampling
layout(set = 2, binding = 1) uniform sampler2D texSplatMap;
layout(set = 2, binding = 2) uniform sampler2DArray texArray;

layout(location = 0) in vec3 inFragPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

vec3 DirPhongLighting(vec3 view, vec3 dir, vec3 normal, vec3 color, float intensity) {
	vec3 light = normalize(dir);
	vec3 halfway = normalize(light + view);
	vec3 reflect = reflect(-light, normal);
	vec3 diffuse = max(dot(normal, light), 0.0f)* vec3(0.8f);
	vec3 specular = pow(max(dot(view, reflect), 0.0), 2.0f)* vec3(0.05f);
	vec3 contrib = (diffuse + specular)* vec3(intensity) * color;

	return contrib;
}

void main() {
	//vec4 texColor = inColor; //splatmap not in yet, so just use vertex colors until then
	vec4 texColor = texture(texSplatMap, inTexCoord);

	float texSampleDensity = 1000.0f;

	vec4 layerFirst = texture(texArray, vec3(inTexCoord.x * texSampleDensity, inTexCoord.y * texSampleDensity, 0));
	vec4 layerSecond = texture(texArray, vec3(inTexCoord.x * texSampleDensity, inTexCoord.y * texSampleDensity, 1));
	vec4 layerThird = texture(texArray, vec3(inTexCoord.x * texSampleDensity, inTexCoord.y * texSampleDensity, 2));
	vec4 layerFourth = texture(texArray, vec3(inTexCoord.x * texSampleDensity, inTexCoord.y * texSampleDensity, 3));
	//vec4 layerFifth = texture(texArray, vec3(inTexCoord.x * texSampleDensity, inTexCoord.y * texSampleDensity, 4));

	vec4 terrainSplatter = (layerFirst * texColor.x + layerSecond*texColor.y + layerThird * texColor.z + layerFourth * (1 - texColor.r - texColor.g - texColor.b));

	vec3 viewVec = normalize(-cam.cameraDir);

	vec3 normalVec = normalize(inNormal);

	vec3 pointLightContrib = vec3(0.0f);
	for(int i = 0; i < PointLightCount; i++){
		vec3 lightVec = normalize(point.lights[i].position - inFragPos).xyz;
		vec3 reflectVec = reflect(-lightVec, normalVec);
		vec3 halfwayVec = normalize(viewVec + point.lights[i].position);	
		
		float separation = distance(point.lights[i].position, inFragPos);
		float attenuation = 1.0f/(separation*separation);

		vec3 diffuse = max(dot(normalVec, lightVec), 0.0) * vec3(1.0f) * attenuation * point.lights[i].color;
		vec3 specular = pow(max(dot(viewVec, reflectVec), 0.0), 16.0) * vec3(0.75f)* attenuation * point.lights[i].color;
		pointLightContrib += (diffuse + specular);
	}

	vec3 dirContrib = DirPhongLighting(viewVec, sun.light[0].direction, normalVec, sun.light[0].color, sun.light[0].intensity);

	//float belowWaterLevelDarkening = clamp(inFragPos.y, -1, 0);
	//outColor = (vec4(0,0,0,0) + inColor) * vec4((pointLightContrib + dirContrib), 1.0f);
	outColor = terrainSplatter * vec4((pointLightContrib + dirContrib), 1.0f);
	//outColor = vec4(pointLightContrib * inColor, 1.0f);
	
	
}