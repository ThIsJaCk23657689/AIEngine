#version 330 core
out vec4 FragColor;

struct Material {
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float shininess;

	sampler2D diffuse_texture;
	sampler2D specular_texture;
	sampler2D emission_texture;

	bool enableDiffuseTexture;
    bool enableSpecularTexture;
	bool enableEmission;
    bool enableEmissionTexture;
};

struct Light {
	vec3 position;
	vec3 direction;
	
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;

	float constant;
	float linear;
	float quadratic;

	float cutoff;
	float outerCutoff;
	float exponent;

	bool enable;
	int caster;
};

struct Fog {
	int mode;
	int depthType;
	float density;
	float f_start;
	float f_end;
	bool enable;
	vec4 color;
};

struct Plane {
	vec3 position;
	vec3 normal;
};

// 0 Direction Light; 1 2 3 4 Point Light; 5 6 Spot Light;
#define NUM_LIGHTS 7

in VS_OUT {
    vec3 NaivePos;
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
} fs_in;

uniform vec3 viewPos;
uniform bool useBlinnPhong;
uniform bool useLighting;
uniform bool useDiffuseTexture;
uniform bool useSpecularTexture;
uniform bool useEmission;
uniform bool useGamma;
uniform float GammaValue;

uniform bool isCubeMap;
uniform bool enableCulling;
uniform samplerCube skybox;
uniform sampler2D shadowMap;

uniform Material material;
uniform Light lights[NUM_LIGHTS];
uniform Plane clippingPlanes[6];
uniform Fog fog;

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	projCoords = projCoords * 0.5 + 0.5;

	if (projCoords.z > 1.0) {
		return 0.0;
	}

	float closestDepth = texture(shadowMap, projCoords.xy).r;
	float currentDepth = projCoords.z;
	float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
	float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
	for(int x = -1; x <= 1; ++x) {
		for(int y = -1; y <= 1; ++y) {
			float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
			shadow += currentDepth > pcfDepth ? 1.0 : 0.0;        
		}    
	}
	shadow /= 9.0;

	return shadow;
}

vec3 CalcLight(Light light, vec3 normal, vec3 viewDir, vec4 texel_ambient, vec4 texel_diffuse, vec4 texel_specular) {

	vec3 ambient = vec3(0.0);
	vec3 diffuse = vec3(0.0);
	vec3 specular = vec3(0.0);

	vec3 lightDir = vec3(0.0);
	if (light.caster == 0) {
		// Direction Light
		lightDir = normalize(-light.direction);
	} else {
		lightDir = normalize(light.position - fs_in.FragPos);
	}

	float diff = max(dot(normal, lightDir), 0.0);

	float spec = 0.0;
	if (useBlinnPhong) {
		vec3 halfway = normalize(lightDir + viewDir);
		spec = pow(max(dot(normal, halfway), 0.0), material.shininess);
	} else {
		vec3 reflectDir = reflect(-lightDir, normal);
		spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	}

	ambient = light.ambient * texel_ambient.rgb;
	diffuse = light.diffuse * diff * texel_diffuse.rgb;
	specular = light.specular * spec * texel_specular.rgb;

	if (light.caster == 0) {
		// Direction Light
		if (isCubeMap) {
			ambient = light.diffuse * texel_ambient.rgb;
			diffuse = light.diffuse * texel_diffuse.rgb;
			specular *= 0.0f;
		}
	} else {
		// Point Light or Spot Light
		if (isCubeMap) {
			ambient *= 0.0f;
			diffuse *= 0.0f;
			specular *= 0.0f;
		} else {
			float distance = length(light.position - fs_in.FragPos);
			float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

			ambient *= attenuation;
			diffuse *= attenuation;
			specular *= attenuation;

			if (light.caster == 2) {
				// Spot Light
				float intensity = 0.0f;
				float theta = dot(lightDir, normalize(-light.direction));
				float epsilon = light.cutoff - light.outerCutoff;
				intensity = clamp((theta - light.outerCutoff) / epsilon, 0.0, 1.0);

				ambient *= intensity;
				diffuse *= intensity;
				specular *= intensity;
			}
		}
	}

	// ?p?????v
    float shadow = ShadowCalculation(fs_in.FragPosLightSpace, normal, lightDir);       
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular));    

	return lighting;
}

void main() {         
	vec3 norm = normalize(fs_in.Normal);
	vec3 viewDir = normalize(viewPos - fs_in.FragPos);

	vec4 texel_ambient = vec4(0.0);
	vec4 texel_diffuse = vec4(0.0);
	vec4 texel_specular = vec4(0.0);
	if (isCubeMap) {
		// ???????????? cubemap
		texel_ambient = texture(skybox, normalize(fs_in.NaivePos));
		texel_diffuse = texture(skybox, normalize(fs_in.NaivePos));
		texel_specular = texture(skybox, normalize(fs_in.NaivePos));
	} else if (useDiffuseTexture && material.enableDiffuseTexture) {
		// ?p?G???}?????????? ?B ?????????????K???? => ??????????
		texel_ambient = texture(material.diffuse_texture, fs_in.TexCoords);
		texel_diffuse = texture(material.diffuse_texture, fs_in.TexCoords);
		if (useSpecularTexture && material.enableSpecularTexture) {
			texel_specular = texture(material.specular_texture, fs_in.TexCoords);
		} else {
			texel_specular = texture(material.diffuse_texture, fs_in.TexCoords);
		}
	} else {
		// ????????
		texel_ambient = material.ambient;
		texel_diffuse = material.diffuse;
		if (useSpecularTexture && material.enableSpecularTexture) {
			texel_specular = texture(material.specular_texture, fs_in.TexCoords);
		} else {
			texel_specular = material.specular;
		}
	}

	// Culling with view volume
	if (enableCulling) {
		for(int i = 0; i < 6; i++) {
			vec3 temp_1 = normalize(clippingPlanes[i].normal);
			vec3 temp_2 = normalize(fs_in.FragPos - clippingPlanes[i].position);
			float angle = dot(temp_1, temp_2);
			if(angle > 0) {
				discard;
				break;
			}
		}
	}

	// ?O?_?}??????
	if (!useLighting) {
		// ?h?z??
		if (texel_diffuse.a < 0.1) {
			discard;
		}

		FragColor = texel_diffuse;
	} else {
		// ?p??????
		vec3 illumination = vec3(0.0);

		for (int i = 0; i < NUM_LIGHTS; i++) {
			if (!lights[i].enable) {
				continue;
			}
			illumination += CalcLight(lights[i], norm, viewDir, texel_ambient, texel_diffuse, texel_specular);
		}

		// ?}?????o??
		if (useEmission && material.enableEmission) {
			// ???}?????o?????? ?B ?????????}?????o??
			if (material.enableEmissionTexture) {
				// ???????o??????
				illumination += texture(material.emission_texture, fs_in.TexCoords).rgb;
			} else {
				// ???????o???C??
				illumination += texel_diffuse.rgb * 1.5;
			}
		}

		// ?h?z??
		if (texel_diffuse.a < 0.1) {
			discard;
		}

		// Foggy Effect
		vec4 PreColor = vec4(clamp(illumination, 0.0, 1.0), texel_diffuse.a);
		vec4 FinalColor = vec4(0.0);
		float distance = 0.0;
		float fogFactor = 0.0;

		if (fog.depthType == 0) {
			// Plane Based
			distance = abs((viewPos - fs_in.FragPos).z);
		} else {
			// Range Based
			distance = length(viewPos - fs_in.FragPos);
		}

		if (fog.enable) {
			if (fog.mode == 0) {
			// Foggy Effect Linear
			fogFactor = clamp((fog.f_end - distance) / (fog.f_end - fog.f_start), 0.0, 1.0);
			} else if (fog.mode == 1) {
				// Foggy Effect EXP
				fogFactor = clamp(1.0 / exp(fog.density * distance), 0.0, 1.0);
			} else if (fog.mode == 2) {
				// Foggy Effect EXP2
				fogFactor = clamp(1.0 / exp(fog.density * distance * distance), 0.0, 1.0);
			}
			FinalColor = mix(fog.color, PreColor, fogFactor);
		} else {
			// Close Foggy Effect
			FinalColor = PreColor;
		}

		// ?{??????
		if (useGamma) {
			FinalColor = vec4(pow(FinalColor.xyz, vec3(GammaValue)), FinalColor.w);
		}

		FragColor = FinalColor;
	}
}