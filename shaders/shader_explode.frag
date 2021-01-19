#version 430 core
//out vec4 FragColor;

//in vec2 TexCoords;

uniform vec3 objectColor;
uniform vec3 lightDir;

in vec3 interpNormal;

//uniform sampler2D texture_diffuse1;

void main()
{
    //FragColor = texture(texture_diffuse1, TexCoords);
    vec3 normal = normalize(interpNormal);
	float diffuse = max(dot(normal, -lightDir), 0.0);
	gl_FragColor = vec4(objectColor * diffuse, 1.0);
}
