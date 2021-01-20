#version 430 core

layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexTexCoord;
layout(location = 2) in vec3 vertexNormal;

uniform mat4 modelViewProjectionMatrix;
uniform mat4 modelMatrix;

out VS_OUT {
    vec2 texCoords;
    vec3 normal;
} vs_out;

out vec3 interpNormal;
out vec2 TexCoords;



void main()
{
    vs_out.texCoords = vertexTexCoord;
    vs_out.normal = vertexNormal;
	gl_Position = modelViewProjectionMatrix * vec4(vertexPosition, 1.0);    
	interpNormal = (modelMatrix * vec4(vertexNormal, 0.0)).xyz;
    TexCoords = vertexTexCoord;
}
