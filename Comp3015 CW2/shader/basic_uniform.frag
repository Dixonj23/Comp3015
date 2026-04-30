#version 460

in vec3 Position;   // view-space position
in vec3 WorldPos;   // world-space position
in vec2 TexCoord;
in mat3 TBN;

layout (binding = 0) uniform sampler2D baseTexColor1;
layout (binding = 1) uniform sampler2D NormalMapTex;
layout (binding = 2) uniform samplerCube PointShadowMap;

layout (binding = 3) uniform sampler2D MetalnessTex;
layout (binding = 4) uniform sampler2D EmissiveTex;

uniform int UseMetalnessTex;
uniform int UseEmissiveTex;
 

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

uniform int UseTexture;
uniform vec3 SolidColor;
uniform float EmissiveStrength;
uniform float Time;
uniform int IsBeam;

uniform vec3 PointShadowLightPos; // world-space light position
uniform float FarPlane;

uniform struct LightInfo {
    vec4 Position;   // view-space light position
    vec3 La;
    vec3 Ld;
    vec3 Ls;
    float Constant;
    float Linear;
    float Quadratic;
} Light;

struct PointLightInfo {
    vec4 Position;   // view-space point light position
    vec3 La;
    vec3 Ld;
    vec3 Ls;
    float Constant;
    float Linear;
    float Quadratic;
};

uniform PointLightInfo CornerLights[4];

uniform struct MaterialInfo {
    vec3 Ka;
    vec3 Kd;
    vec3 Ks;
    float Shininess;
} Material;



float hash(vec2 p)
{
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float beamPattern(vec2 uv, float t)
{
    float wave1 = sin((uv.y * 18.0) - t * 6.0) * 0.5 + 0.5;
    float wave2 = sin((uv.y * 31.0) + t * 9.0) * 0.5 + 0.5;
    float wave3 = sin((uv.x * 10.0) + (uv.y * 24.0) - t * 4.0) * 0.5 + 0.5;

    float n = hash(vec2(floor(uv.y * 20.0), floor(t * 8.0)));
    float flicker = mix(0.85, 1.15, n);

    return (0.45 * wave1 + 0.35 * wave2 + 0.20 * wave3) * flicker;
}


float computeAttenuation(float constantTerm, float linearTerm, float quadraticTerm, float dist)
{
    return 1.0 / (constantTerm + linearTerm * dist + quadraticTerm * dist * dist);
}

// Simple point-light shadow lookup.
// This is the basic version to get working first.
// You can add PCF later once the shadows are visible.
float calculatePointShadow(vec3 worldPos)
{
    vec3 fragToLight = worldPos - PointShadowLightPos;
    float currentDepth = length(fragToLight);

    float closestDepth = texture(PointShadowMap, fragToLight).r;
    closestDepth *= FarPlane;

    float bias = 0.05;
    float shadow = (currentDepth - bias > closestDepth) ? 1.0 : 0.0;

    // If fragment is outside far plane, don't shadow it
    if (currentDepth > FarPlane)
        shadow = 0.0;

    return shadow;
}

vec3 evaluateMainLight(vec3 albedo, vec3 N, vec3 fragPos, vec3 viewDir, float metalness)
{
    vec3 lightVec = Light.Position.xyz - fragPos;
    float dist = length(lightVec);
    vec3 lightDir = normalize(lightVec);
    vec3 halfVec = normalize(viewDir + lightDir);

    float attenuation = computeAttenuation(
        Light.Constant,
        Light.Linear,
        Light.Quadratic,
        dist
    );

    float shadow = calculatePointShadow(WorldPos);

    vec3 ambient = Light.La * albedo * Material.Ka;

    // Make floor/low-light shadows readable
    ambient *= mix(1.0, 0.35, shadow);

    float sDotN = max(dot(lightDir, N), 0.0);
    vec3 diffuse = Light.Ld * albedo * Material.Kd * sDotN * attenuation * (1.0 - shadow);


    vec3 specColor = mix(Material.Ks, albedo, metalness);
    vec3 specular = vec3(0.0);
    if (sDotN > 0.0)
    {
        specular = Light.Ls * specColor *
                   pow(max(dot(halfVec, N), 0.0), Material.Shininess) *
                   attenuation * (1.0 - shadow);
    }

    return ambient + diffuse + specular;
}

vec3 evaluatePointLight(
    vec3 albedo,
    vec3 N,
    vec3 fragPos,
    vec3 viewDir,
    PointLightInfo light,
    float metalness
)
{
    vec3 lightVec = light.Position.xyz - fragPos;
    float dist = length(lightVec);
    vec3 lightDir = normalize(lightVec);
    vec3 halfVec = normalize(viewDir + lightDir);

    float attenuation = computeAttenuation(
        light.Constant,
        light.Linear,
        light.Quadratic,
        dist
    );

    vec3 ambient = light.La * albedo * Material.Ka;

    float sDotN = max(dot(lightDir, N), 0.0);
    vec3 diffuse = light.Ld * albedo * Material.Kd * sDotN * attenuation;

    vec3 specColor = mix(Material.Ks, albedo, metalness);
    vec3 specular = vec3(0.0);
    if (sDotN > 0.0)
    {
        specular = light.Ls * specColor *
                   pow(max(dot(halfVec, N), 0.0), Material.Shininess) *
                   attenuation;
    }

    return ambient + diffuse + specular;
}


void main()
{
    // Base colour
    vec3 albedo = (UseTexture == 1)
        ? texture(baseTexColor1, TexCoord).rgb
        : SolidColor;

    // Beam-specific animated energy pattern
    if (IsBeam == 1)
    {
        float energy = beamPattern(TexCoord, Time);
        albedo *= mix(0.7, 1.8, energy);
        albedo += vec3(0.05, 0.15, 0.25) * energy;
    }

    //Metalness
    float metalness = 0.0;
    if (UseMetalnessTex == 1)
    {
        metalness = texture(MetalnessTex, TexCoord).r;
    }

    //Emissive
    vec3 emissiveTexColor = vec3(0.0);
    if (UseEmissiveTex == 1)
    {
        emissiveTexColor = texture(EmissiveTex, TexCoord).rgb;
    }

    // Normal
    vec3 N;
    if (UseTexture == 1)
    {
        vec3 nTan = texture(NormalMapTex, TexCoord).xyz * 2.0 - 1.0;
        N = normalize(TBN * nTan);
    }
    else
    {
        N = normalize(TBN[2]);
    }

    vec3 viewDir = normalize(-Position);

    // Main reactor light + corner lights
    vec3 litColor = evaluateMainLight(albedo, N, Position, viewDir, metalness);

    for (int i = 0; i < 4; ++i)
    {
        litColor += evaluatePointLight(albedo, N, Position, viewDir, CornerLights[i], metalness);
    }

    // Emissive contribution
    litColor += albedo * EmissiveStrength;
    litColor += emissiveTexColor * EmissiveStrength;

    FragColor = vec4(litColor, 1.0);

    // Bloom extract
    float brightness = dot(litColor, vec3(0.2126, 0.7152, 0.0722));
    BrightColor = (brightness > 1.0)
        ? vec4(litColor, 1.0)
        : vec4(0.0, 0.0, 0.0, 1.0);
}