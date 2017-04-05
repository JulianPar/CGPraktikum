#ifdef GL_ES
// Set default precision to medium
precision mediump int;
precision mediump float;
#endif

const vec3 lightPos = vec3(1000.0,1000.0,1000.0); //Kamerakoordinaten
const vec3 lightPos2 = vec3(-2000.0,0.0,1000.0); //Weltkoordinaten
const vec4 lp2 = vec4(-100,0,0,1);

varying vec3 vNormal;
varying vec3 vPos;

uniform vec3 uAmbient;
uniform vec3 uDiffuse;
uniform vec3 uSpecular;
uniform float uShininess;

uniform mat4 uMVMat;
uniform mat4 uPMat;

void main() {

    vec4 lp2 = uPMat*uMVMat*lp2;
    vec3 lightPos2 = vec3(lp2);
    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(lightPos - vPos);
    vec3 lightDir2 = normalize(lightPos2 - vPos);
    vec3 reflectDir = reflect(lightDir, normal);
    vec3 reflectDir2 = reflect(lightDir2, normal);
    vec3 viewDir = normalize(-vPos);

    float lambertian = max(dot(lightDir,normal), 0.0)+max(dot(lightDir2,normal), 0.0);;
    float specular = 0.0;
    float specular2 = 0.0;

    if (lambertian > 0.0) {
       float specAngle = max(dot(reflectDir, viewDir), 0.0);
       float specAngle2 = max(dot(reflectDir2, viewDir), 0.0);
       specular = pow(specAngle,uShininess);
       specular2 = pow(specAngle2,uShininess);
    }

    gl_FragColor = vec4(uAmbient + lambertian*uDiffuse + specular*uSpecular + specular2*uSpecular, 1.0);
}

