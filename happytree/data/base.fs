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


float getshadowsample(vec4 coord)
{
    float shadowdepth = texture(shadowmap, coord.xy).r;

    if (coord.w > 0.0 && 
        shadowdepth < coord.z &&
        coord.x > 0.0 &&
        coord.y > 0.0 &&
        coord.x < 1.0 &&
        coord.y < 1.0)
    {
        return 0.0;
    }
    return 1.0;
}

vec2 poissonDisk[16] = vec2[](
vec2( -0.94201624, -0.39906216 ),
vec2( 0.94558609, -0.76890725 ),
vec2( -0.094184101, -0.92938870 ),
vec2( 0.34495938, 0.29387760 ),
vec2( -0.91588581, 0.45771432 ),
vec2( -0.81544232, -0.87912464 ),
vec2( -0.38277543, 0.27676845 ),
vec2( 0.97484398, 0.75648379 ),
vec2( 0.44323325, -0.97511554 ),
vec2( 0.53742981, -0.47373420 ),
vec2( -0.26496911, -0.41893023 ),
vec2( 0.79197514, 0.19090188 ),
vec2( -0.24188840, 0.99706507 ),
vec2( -0.81409955, 0.91437590 ),
vec2( 0.19984126, 0.78641367 ),
vec2( 0.14383161, -0.14100790 )
);

// Returns a random number based on a vec3 and an int.
float random(vec3 seed, int i)
{
    vec4 seed4 = vec4(seed,i);
    float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
    return fract(sin(dot_product) * 43758.5453);
}

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
        float shade = 0;
        for (int i=0;i<16;i++)
        {
            //int index = int(16.0*random(floor(origcoord.xyz*1000.0), i))%16;
            vec4 temp = shadowcoord;
            temp.xy += poissonDisk[i]/500.0;
            shade += getshadowsample(temp);
        }
        shade = shade / 16.0;
        shade = (1.0 + shade) / 2.0;
         
        s = vec4(shade,shade,shade,1);
    }

    else
    {
        s = vec4(1);
    }

    FragColor = vec4(color, 1.0) * t * s;
}