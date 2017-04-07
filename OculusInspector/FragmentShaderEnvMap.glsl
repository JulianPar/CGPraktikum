#ifdef GL_ES
// Set default precision to medium
precision mediump int;
precision mediump float;
#endif

uniform samplerCube cubemap;

varying vec3 v_position;

void main() {
    gl_FragColor = textureCube(cubemap,v_position);
}

