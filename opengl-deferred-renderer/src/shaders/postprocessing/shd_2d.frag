#version 330 core
out vec4 FragColor;
  
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform bool grayscale;

const float redWeight = 0.2126;
const float greenWeight = 0.7152;
const float blueWeight = 0.0722;

const float offset = 1.0/300.0;
const float gamma = 2.2; //const for "now", this should probably go in a uniform

vec4 get_grayscale(vec4 fragColor);

void main()
{ 
    vec2 offsets[9] = vec2[](
        vec2(-offset,  offset), // top-left
        vec2( 0.0f,    offset), // top-center
        vec2( offset,  offset), // top-right
        vec2(-offset,  0.0f),   // center-left
        vec2( 0.0f,    0.0f),   // center-center
        vec2( offset,  0.0f),   // center-right
        vec2(-offset, -offset), // bottom-left
        vec2( 0.0f,   -offset), // bottom-center
        vec2( offset, -offset)  // bottom-right    
    );

    float kernel[9] = float[](
        0.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 0.0
    );

    vec3 sampleTex[9];
    for (int i = 0; i < 9; i++) {
        sampleTex[i] = vec3(texture(screenTexture, TexCoords.st+offsets[i]));
    }
    vec3 col = vec3(0.0);
    for (int i = 0; i < 9; i++)
        col+=sampleTex[i]*kernel[i];

    FragColor = vec4(col, 1.0);
    if (grayscale)
        FragColor = get_grayscale(FragColor);
    
    
    FragColor = vec4(pow(FragColor.rgb, vec3(1.0/gamma)), FragColor.a);
}

vec4 get_grayscale(vec4 fragColor) {
    float average = (redWeight*fragColor.r+greenWeight*fragColor.g+blueWeight*fragColor.b)/3.0;
    return vec4(average, average, average, 1.0);
}