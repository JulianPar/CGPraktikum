#ifdef GL_ES
// Set default precision to medium
precision mediump int;
precision mediump float;
#endif

attribute vec3 a_position;
attribute vec2 a_tex;
attribute vec3 a_normal;

uniform mat4 uMVMat;
uniform mat4 uPMat;
uniform mat3 uNMat;

varying float vIntensity;
varying vec2 vtex;

void main() {
    vtex = a_tex;
    vec4 p = uPMat * uMVMat * vec4(a_position,1.0);
    gl_Position = p;
    vec3 n = normalize(uNMat * a_normal);
    //vIntensity = abs(n.z);
    vIntensity = 0.9;
}
