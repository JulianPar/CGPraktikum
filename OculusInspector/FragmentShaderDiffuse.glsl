#ifdef GL_ES
// Set default precision to medium
precision mediump int;
precision mediump float;
#endif

uniform sampler2D texture;
uniform float uHasTex;
uniform vec4  uDiffuse;

varying float vIntensity;
varying vec2  vtex;

void main() {
    vec4 color = (uHasTex == 0.0)? uDiffuse : texture2D(texture,vtex);
    gl_FragColor = vec4(vIntensity*color.r,vIntensity*color.g,vIntensity*color.b,1.0);
    //gl_FragColor = vec4(uDiffuse.r,uDiffuse.g,uDiffuse.b,1.0);
}

