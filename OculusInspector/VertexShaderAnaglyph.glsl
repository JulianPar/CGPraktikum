#ifdef GL_ES
// Set default precision to medium
precision mediump int;
precision mediump float;
#endif

attribute vec3 a_position;

uniform mat4 uMVMat;
uniform mat4 uPMat;

varying vec2 tex;

void main() {
    vec4 pos = vec4(a_position,1.0);
    tex.x = 0.5*pos.x+0.5;
    tex.y = 0.5*pos.y+0.5;

    gl_Position = uPMat * uMVMat * pos;
}

