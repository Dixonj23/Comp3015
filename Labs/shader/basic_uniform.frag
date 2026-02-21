#version 460

in vec3 Position;
in vec3 Normal;
in vec2 TexCoord;

uniform sampler2D RenderTex;
layout (location = 0) out vec4 FragColor;


uniform struct LightInfo{
    vec4 Position;
    vec3 La;
    vec3 L;
} Light;

uniform struct MaterialInfo {
    vec3 Kd;
    vec3 Ka;
    vec3 Ks;
    float Shininess;
} Material;



vec3 blinnPhong(vec3 position, vec3 n)
{
    vec3 diffuse = vec3(0), spec = vec3(0);
    vec3 texColor = texture(RenderTex, TexCoord).rgb;
    vec3 ambient = Light.La * texColor;
    vec3 s = normalize(Light.Position.xyz - position);
    float sDotN = max(dot(s,n), 0.0);
    diffuse = texColor * sDotN;
    if (sDotN > 0.0){
        vec3 v = normalize(-position.xyz);
        vec3 h = normalize(v + s);
        spec = Material.Ks * pow(max(dot(h,n), 0.0), Material.Shininess);
    }

    return ambient + (diffuse + spec) * Light.L;

}

void main()
{
    FragColor = vec4(blinnPhong(Position, normalize(Normal)), 1.0);
}
