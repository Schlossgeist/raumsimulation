#SHADER_VERTEX

in vec4 position;
in vec4 sourceColour;
in vec2 textureCoordIn;

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 positionMatrix;
uniform mat4 rotationMatrix;

out vec4 destinationColour;
out vec2 textureCoordOut;

void main()
{
    destinationColour = sourceColour;
    textureCoordOut = textureCoordIn;
    gl_Position = projectionMatrix * viewMatrix * positionMatrix * position;
}

#SHADER_FRAGMENT

out vec4 destinationColour;
out vec2 textureCoordOut;

void main()
{
   vec4 colour = vec4(0.50, 0.00, 0.00, 0.75);
   gl_FragColor = colour;
}
