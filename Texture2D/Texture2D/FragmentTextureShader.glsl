#ifdef GL_ES
// Set default precision to medium
precision mediump int;
precision mediump float;
#endif

const vec3 lightPos = vec3(1000.0,1000.0,1000.0);
varying vec3 vNormal;
varying vec3 vPos;

uniform vec3 uAmbient;
uniform vec3 uDiffuse;
uniform vec3 uSpecular;
uniform float uShininess;

uniform mat4 uMVMat;
uniform mat4 uPMat;

void main() {

    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(lightPos - vPos);
    vec3 reflectDir = reflect(lightDir, normal);
    vec3 viewDir = normalize(-vPos);

    float lambertian = max(dot(lightDir,normal), 0.0);
    float specular = 0.0;


    gl_FragColor = vec4(uAmbient * 1.5 + lambertian*uDiffuse * 0.1 + specular*uSpecular * 0.0 , 1.0);
}


