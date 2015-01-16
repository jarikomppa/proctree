#version 330

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexTexCoord;

out vec2 texcoord;

uniform mat4 RotationMatrix;

void main()
{
    texcoord = vertexTexCoord;
    gl_Position = RotationMatrix * vec4(vertexPosition, 1.0);
}