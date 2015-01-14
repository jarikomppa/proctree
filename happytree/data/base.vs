#version 330

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexTexCoord;

out vec3 color;
out vec2 texcoord;

uniform mat4 RotationMatrix;
uniform bool EnableLighting;

void main()
{
    vec3 lightdir = vec3(2,-1,0);
    float v = dot(vertexNormal,lightdir);
    v = (v + 1) * 0.25 + 0.5;
    if (EnableLighting)
        color = vec3(v);
    else
        color = vec3(1,1,1);
    texcoord = vertexTexCoord;
    gl_Position = RotationMatrix * vec4(vertexPosition, 1.0);
}