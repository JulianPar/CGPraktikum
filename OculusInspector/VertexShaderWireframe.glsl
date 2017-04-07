#ifdef GL_ES
// Set default precision to medium
precision mediump int;
precision mediump float;
#endif

attribute vec3 a_position;

uniform mat4 uMVMat;
uniform mat4 uPMat;

void main() {
    gl_Position = uPMat * uMVMat * vec4(a_position,1.0);
}
