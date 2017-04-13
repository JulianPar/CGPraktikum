#ifdef GL_ES
// Set default precision to medium
precision mediump int;
precision mediump float;
#endif

//const vec3 lightPos = vec3(1000.0,1000.0,1000.0);
varying vec3 vColor;

void main() {
    gl_FragColor=vec4(vColor,1.0);
}
