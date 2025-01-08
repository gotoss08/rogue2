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

void crt2(out vec4 fragColor, in vec2 fragCoord )
{
    float brightness = 1.5;
    vec2 uv = fragCoord;
    uv = 2.*uv -1.;
    vec2 uvA = vec2(uv.x * iResolution.x/iResolution.y,uv.y);

    float curvature = 5.;
    vec2 offset = uv.yx/curvature;
    uv = uv + uv *offset*offset;
    uv = uv *0.5 +0.5;

    vec4 img = texture(texture0, vec2(uv.x - 0.005
                                                ,uv.y));
    vec4 img2 = texture(texture0, vec2(uv.x + 0.
                                                ,uv.y));
    vec4 img3 = texture(texture0, vec2(uv.x + 0.005
                                                , uv.y));


    float lines = abs(sin(300.*uv.y +iTime)) *brightness;


    img.xyz *= vec3(1.,0.,0.); // red
    img2.xyz *= vec3(0.,1.,0.); // green
    img3.xyz *= vec3(0.,0.,1.); // blue
    vec3 col = img.xyz+img2.xyz+img3.xyz;
    if (uv.x <= 0.0f || 1.0 <= uv.x || uv.y <= 0.0 || 1.0 <= uv.y){
        col = vec3(0.);
    }

    col -= smoothstep(0.2,6.,length(uvA))*2.*(col.x+col.y+col.z)/0.99;

    col*= lines;
    // Output to screen
    fragColor = vec4( col,1.0);
}

void main()
{

    // Texel color fetching from texture sampler
    vec2 texelCoord = vec2(fragTexCoord.x, 1.0 - fragTexCoord.y);
    vec4 texelColor = texture(texture0, texelCoord);

    crt2(texelColor, texelCoord);

    // NOTE: Implement here your fragment shader code

    // final color is the color from the texture
    //    times the tint color (colDiffuse)
    //    times the fragment color (interpolated vertex color)
    finalColor = texelColor;

}
