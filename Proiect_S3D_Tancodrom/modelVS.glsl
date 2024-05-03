#version 330 core

layout (location = 0) in vec3 InPosition;
layout (location = 1) in vec3 InNormal;
layout (location = 2) in vec2 InTextCoords;

out vec2 MidTexCoords;

uniform mat4 ProjectionMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ModelMatrix;

void main()
{
    gl_Position = ProjectionMatrix * ViewMatrix * ModelMatrix * vec4(InPosition, 1.0f);
    MidTexCoords = InTextCoords;
}