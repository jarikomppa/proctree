#version 330

in vec3 color;
in vec2 texcoord;
in vec4 origcoord;

layout(location = 0) out vec4 FragColor;
uniform sampler2D tex;
uniform sampler2D shadowmap;
uniform bool EnableTexture;
uniform bool EnableShadows;
uniform mat4 ShadowMatrix;


void main()
{
    vec4 t, s;

    t = texture(tex, texcoord);

    if (t.a < 0.5)
        discard;
    
    if (!EnableTexture)
    {
        t = vec4(1);
    }

    if (EnableShadows)
    {
        vec4 shadowcoord = ShadowMatrix * origcoord;
        shadowcoord /= shadowcoord.w;
        float shadowdepth = texture2D(shadowmap, shadowcoord.st).z;
        float shade = 1.0;
        shadowcoord.z += 0.05;

        if (shadowcoord.w > 0.0 && shadowdepth < shadowcoord.z)
        {
            shade = 0.80;
        }
    }

    else
    {
        s = vec4(1);
    }

    FragColor = vec4(color, 1.0) * t * s;
}