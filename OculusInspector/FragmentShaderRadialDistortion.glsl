#ifdef GL_ES
// Set default precision to medium
precision mediump int;
precision mediump float;
#endif

uniform sampler2D texture;
uniform float aspect;

varying vec2 tex;

void main() {
    float alpha = 6.0, beta = 0.0;
    float a2 = aspect*aspect;
    vec2 m;
    m.x = (tex.x < 0.5)? 0.25 : 0.75;
    m.y = 0.5;
    float r2 = (tex.x-m.x)*(tex.x-m.x)+(tex.y-m.y)*(tex.y-m.y)/a2;
    vec2 t = (tex-m)*(1.0+alpha*r2+beta*r2*r2)+m;
 
    if (tex.x < 0.5 && t.x >= 0.5) discard;
    if (tex.x > 0.5 && t.x <= 0.5) discard;
    if (t.y < 0.0 || t.y > 1.0) discard;
    if (t.x < 0.0 || t.x > 1.0) discard;

    gl_FragColor = texture2D(texture,t);
}

