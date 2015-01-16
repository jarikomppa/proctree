#version 330

in vec2 texcoord;

layout(location = 0) out vec4 FragColor;
uniform sampler2D tex;

void main()
{
    vec4 t, s;

    t = texture(tex, texcoord);

    if (t.a < 0.5)
        discard;
    
    FragColor = vec4(1.0);
}