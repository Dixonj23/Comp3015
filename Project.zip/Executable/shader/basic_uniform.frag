#version 460

in vec3 Position;
in mat3 TBN;
in vec2 TexCoord;
in vec3 WorldPos;
in vec3 SkyDir;

uniform sampler2D RenderTex;
layout (binding = 0) uniform sampler2D baseTexColor1;
layout (binding = 1) uniform sampler2D baseTexColor2;
layout (binding = 2) uniform sampler2D NormalMapTex;
layout (binding = 3) uniform sampler2D CorruptTex;
layout (binding = 4) uniform samplerCube SkyBoxTex;

layout (location = 0) out vec4 FragColor;

uniform bool RenderSkybox;

uniform vec3 TargetPos[3];
uniform int TargetSolved[3];
uniform float Time;
uniform float TargetSolveTime[3];
uniform float SpreadSpeed;       
uniform float StartRadius;
uniform float MaxSpreadRadius;  

uniform int ActiveLight;

uniform vec3 FogColor;
uniform float FogDensity;

uniform struct FogInfo{
    float MaxDist;
    float MinDist;
    vec3 Color;
} Fog;

#define NUM_LIGHTS 3
uniform struct LightInfo {
    vec4 Position;
    vec3 La;
    vec3 L;
    vec3 Direction;
    float Cutoff;
    float Exponent;
} Lights[NUM_LIGHTS];

uniform struct MaterialInfo {
    vec3 Kd;
    vec3 Ka;
    vec3 Ks;
    float Shininess;
} Material;



vec3 blinnPhongLight(int i, vec3 n, vec3 texColor)
{
     vec3 ambient = Lights[i].La * texColor;

    vec3 s = normalize(Lights[i].Position.xyz - Position);   // frag -> light
    float sDotN = max(dot(s, n), 0.0);

    vec3 diffuse = texColor * sDotN;

    vec3 spec = vec3(0.0);
    if (sDotN > 0.0) {
        vec3 v = normalize(-Position);       // frag -> camera (eye)
        vec3 h = normalize(v + s);
        spec = Material.Ks * pow(max(dot(h, n), 0.0), Material.Shininess);
    }

    // spotlight factor
    float cosAng = dot(normalize(-s), normalize(Lights[i].Direction));
    float spot = 0.0;
    float inner = Lights[i].Cutoff;
    float outer = inner - 0.05; // soft edge width
    float intensity = clamp((cosAng - outer) / (inner - outer), 0.0, 1.0);
    if (cosAng > Lights[i].Cutoff) spot = pow(intensity, Lights[i].Exponent);

    return ambient + spot * (diffuse + spec) * Lights[i].L;
}


void main()
{
    //skybox
    if(RenderSkybox)
    {
        vec3 texColor = texture(SkyBoxTex, SkyDir).rgb;
        FragColor = vec4(texColor, 1.0);
        return;
    }


    //base texture mixing
    vec4 c1 = texture(baseTexColor1, TexCoord);
    vec4 c2 = texture(baseTexColor2, TexCoord);
    vec3 texColor = mix(c1.rgb, c2.rgb, c2.a);

    //target corruption
    float corruptAmount = 0.0;
    vec3 corruptTex = texture(CorruptTex, TexCoord).rgb;

    for (int i = 0; i < 3; i++)
    {
        if (TargetSolved[i] == 1)
        {
            // time since THIS target was solved
            float dt = max(Time - TargetSolveTime[i], 0.0);

            // radius grows forever, but clamp so it doesn't explode
            float radius = min(MaxSpreadRadius, StartRadius + dt * SpreadSpeed);

            float d = distance(WorldPos, TargetPos[i]);

            // soft edge: 1 inside, 0 outside
            float edge = 0.25; // softness width
            float m = 1.0 - smoothstep(radius - edge, radius + edge, d);

            corruptAmount = max(corruptAmount, m);
        }
    }

    // normal mapping
    vec3 nTan = texture(NormalMapTex, TexCoord).xyz * 2.0 - 1.0;
    vec3 n = normalize(TBN * nTan);

    //lighting
    vec3 lit = vec3(0.0);
    for (int i = 0; i < NUM_LIGHTS; i++)
        lit += blinnPhongLight(i, n, texColor);

    // Target glow

    float distance = distance(WorldPos, TargetPos[ActiveLight]);

    float radius = 0.8;

    if(distance < radius)
    {
        float glow = 1.0 - (distance / radius);

        vec3 glowColor = vec3(0.6, 0.0, 0.9); // purple

        lit += glowColor * glow * 0.8;
    }
    



    // overlay corruption after lighting
    vec3 color = mix(lit, corruptTex, corruptAmount);

    //fog
    float dist = abs(Position.z);
    float fogFactor = (Fog.MaxDist-dist)/(Fog.MaxDist-Fog.MinDist);
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    // Final output
    vec3 finalColor = mix(FogColor, color, fogFactor);
    FragColor = vec4(finalColor, 1.0);
    
}
