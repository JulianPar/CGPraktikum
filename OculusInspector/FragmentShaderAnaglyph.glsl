#ifdef GL_ES
// Set default precision to medium
precision mediump int;
precision mediump float;
#endif

uniform sampler2D texture;
uniform mat4 L,R;

varying vec2 tex;

void main() {
    vec2 t1,t2;
    t1.x = 0.5*tex.x;
    t1.y = tex.y;
    t2.x = 0.5*tex.x+0.5;
    t2.y = tex.y;

    vec4 c1 = texture2D(texture,t1);
    vec4 c2 = texture2D(texture,t2);
    gl_FragColor = vec4(L*c1+R*c2);
}

