#version 330 core
out vec4 FragColor;

uniform vec3 objectColor; // We will pass the color from C++

void main()
{
    FragColor = vec4(objectColor, 1.0);
}