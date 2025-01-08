#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec3 iResolution;
uniform float iTime;

// Output fragment color
out vec4 finalColor;

// NOTE: Add here your custom variables

#define PI 3.1415926535897932384626433832795

vec2 curvature = vec2(3.0, 3.0);

float brightness = 4.0;
float vignetteOpacity = 1.0;
float lineOpacity = 1.0;

vec2 curveRemapUV(vec2 uv)
{
    uv = uv * 2.0 - 1.0;

    vec2 offset = abs(uv.yx) / vec2(curvature.x, curvature.y);

    uv = uv + uv * offset * offset;
    uv = uv * 0.5 + 0.5;

    return uv;
}

vec4 scanLineIntensity(float uv, float resolution, float opacity)
{
    float intensity = sin(uv * resolution * PI * 2.0);

    intensity = ((0.5 * intensity) + 0.5) * 0.9 + 0.1;

    return vec4(vec3(pow(intensity, opacity)), 1.0);
}

vec4 vignetteIntensity(vec2 uv, vec2 resolution, float opacity)
{
    float intensity = uv.x * uv.y * (1.0 - uv.x) * (1.0 - uv.y);

    return vec4(vec3(clamp(pow((resolution.x / 4.0) * intensity, opacity), 0.0, 1.0)), 1.0);
}

void crt1(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 uv = fragCoord;
    vec2 remappedUV = curveRemapUV(uv);

    if (remappedUV.x < 0.0 || remappedUV.y < 0.0 || remappedUV.x > 1.0 || remappedUV.y > 1.0){
        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        fragColor = texture(texture0, remappedUV);

        fragColor *= vignetteIntensity(remappedUV, iResolution.xy, vignetteOpacity);
        fragColor *= scanLineIntensity(remappedUV.x, iResolution.y, lineOpacity);
        fragColor *= scanLineIntensity(remappedUV.y, iResolution.x, lineOpacity);
        fragColor *= vec4(vec3(brightness), 1.0);
    }
}

void main()
{

    // Texel color fetching from texture sampler
    vec2 texelCoord = vec2(fragTexCoord.x, 1.0 - fragTexCoord.y);
    vec4 texelColor = texture(texture0, texelCoord);

    crt1(texelColor, texelCoord);

    // NOTE: Implement here your fragment shader code

    // final color is the color from the texture
    //    times the tint color (colDiffuse)
    //    times the fragment color (interpolated vertex color)
    finalColor = texelColor;

}
