#version 330 core

layout ( location = 0 ) in vec3 vertex_position;
layout ( location = 4 ) in vec4 position;
layout (location = 5) in vec2 texCoord; 

uniform mat4 M_v;
uniform mat4 M_p;
uniform mat4 transformation;
uniform float particleSize;
out float lifeTime;
out vec2 interpTexCoord;

void main()
{
   vec4 position_viewspace = M_v * transformation * vec4( position.xyz , 1 );
   position_viewspace.xy += particleSize * (vertex_position.xy - vec2(0.5f));
   gl_Position = M_p * position_viewspace;
   lifeTime = position.w;
   interpTexCoord = texCoord;
};