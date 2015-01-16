#version 330

in vec2 texcoord;

layout(location = 0) out vec4 FragColor;
uniform sampler2D tex;

void main()
{
    FragColor = texture(tex, texcoord.st);    
}