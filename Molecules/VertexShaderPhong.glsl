#ifdef GL_ES
// Set default precision to medium
precision mediump int;
precision mediump float;
#endif

attribute vec3 a_position;
attribute vec3 a_normal;

uniform mat4 uMVMat;
uniform mat4 uPMat;
uniform mat3 uNMat;
uniform vec3 uColor;

varying vec3 vNormal;
varying vec3 vPos;

void main(){
    vec4 p = uMVMat * vec4(a_position, 1.0);
    gl_Position = uPMat * p;
    vPos = vec3(p) / p.w;
    vNormal = normalize(uNMat * a_normal);
}

