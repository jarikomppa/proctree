#version 330

in vec3 color;
in vec2 texcoord;

layout(location = 0) out vec4 FragColor;
uniform sampler2D tex;
uniform bool EnableTexture;

void main()
{
    vec4 t;
    if (EnableTexture)
    {
	    t = texture(tex, texcoord);
	    if (t.a < 0.5) discard;
	}
	else
	{
	    t = vec4(1);
	}
	FragColor = vec4(color, 1.0) * t;
}