#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform CameraUniformBuffer {
	mat4 view;
	mat4 proj;
	vec3 cameraDir;
	float time;
} cbo;

struct PointLight {
	vec4 lightPos;
	vec4 color;
	vec4 attenuation;
};

#define lightCount 5

//Lighting information
layout(binding = 2) uniform PointLightsBuffer {
	PointLight lights[lightCount];
} plb;

//texture sampling
layout(binding = 3) uniform sampler2D waterTex;

layout(location = 0) in vec3 inFragPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inColor;
layout(location = 4) in float inTime;

layout(location = 0) out vec4 outColor;


void main() {
	vec4 texColor1 = texture(waterTex, vec2(inTexCoord.x, inTexCoord.y));
	vec4 texColor2 = texture(waterTex, vec2(inTexCoord.x + 0.1f + sin(inTime/5.0f)/8.0f, inTexCoord.y + 0.1f + sin(inTime/4.0f)/6.0f));
	vec4 texColor3 = texture(waterTex, vec2(inTexCoord.x , inTexCoord.y + sin(inTime/3.0f)/10.0f));
	vec4 texColor = vec4(0.35)*(texColor1 + texColor2 + texColor3);
	
	vec3 viewVec = normalize(-cbo.cameraDir);

	vec3 normalVec = normalize(inNormal);

	vec3 pointLightContrib = vec3(0.0f);
	for(int i = 0; i < lightCount; i++){
		vec3 lightVec = normalize(plb.lights[i].lightPos.xyz - inFragPos).xyz;
		vec3 reflectVec = reflect(-lightVec, normalVec);
			vec3 halfwayVec = normalize(viewVec + plb.lights[i].lightPos.xyz);	
		
		float distance = length(plb.lights[i].lightPos.xyz - inFragPos);
		float attenuation = 1.0f/(plb.lights[i].attenuation.x + plb.lights[i].attenuation.y*distance + plb.lights[i].attenuation.z*distance*distance);

		vec3 diffuse = max(dot(normalVec, lightVec), 0.0) * vec3(1.0f) * attenuation * plb.lights[i].color.xyz;
		vec3 specular = pow(max(dot(viewVec, reflectVec), 0.0), 16.0) * vec3(1.0f)* attenuation * plb.lights[i].color.xyz;
		pointLightContrib += (diffuse + specular);
	}

	vec3 dirLight = normalize(vec3(0, 60, 25));
		vec3 dirHalfway = normalize(dirLight + viewVec);
	vec3 dirReflect = reflect(-dirLight, normalVec);
	vec3 dirDiffuse = max(dot(normalVec, dirLight), 0.0f)* vec3(1.0f);
	vec3 dirSpecular = pow(max(dot(viewVec, dirReflect), 0.0), 16.0f)* vec3(1.0f);
	vec3 dirContrib = (dirDiffuse + dirSpecular)* vec3(1.0f);

    outColor = texColor * vec4((pointLightContrib + dirContrib) * inColor, 1.0f);
	//outColor = texColor * vec4(pointLightContrib * inColor, 1.0f);
	
}
