#version 330

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexTexCoord;

out vec3 color;
out vec2 texcoord;
out vec4 origcoord;

uniform mat4 RotationMatrix;
uniform vec3 lightdir;
uniform bool EnableLighting;

void main()
{
    float v = dot(normalize(vertexNormal), normalize(lightdir));
    v = v + 2;
    if (EnableLighting)
        color = vec3(v);
    else
        color = vec3(1,1,1);
    texcoord = vertexTexCoord;
    origcoord = vec4(vertexPosition, 1.0);
    gl_Position = RotationMatrix * vec4(vertexPosition, 1.0);
}