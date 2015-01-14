#version 330

in vec3 color;
in vec2 texcoord;

layout(location = 0) out vec4 FragColor;
uniform sampler2D tex;

void main()
{
	vec4 t = texture(tex, texcoord);
	if (t.a < 0.5) discard;
	FragColor = vec4(color, 1.0) * t;
}