
#ifdef GL_ES
// Set default precision to medium
precision mediump int;
precision mediump float;
#endif

attribute vec3 a_position;
attribute vec3 a_color;

uniform mat4 uMVMat;
uniform mat4 uPMat;
uniform mat3 uNMat;

varying vec3 vColor;

void main(){
    vec4 p = uMVMat * vec4(a_position, 1.0);
    gl_Position = uPMat * p;
    vColor = a_color;
}
