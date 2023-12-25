#include <bgfx_compute.sh>
#include "../shaderlib.sh"

#define PI 3.141592653589793
#define TWOPI 6.283185307179586
#define HALFPI 1.5707963267948966

#define THREADS 8

SAMPLERCUBE(s_irradiancecubemap, 0);
IMAGE2D_ARRAY_WR(s_target, rgba16f, 1);

NUM_THREADS(THREADS, THREADS, 6)
void main() {
    int imgsize = 64;
    ivec3 global = ivec3(gl_GlobalInvocationID.xyz);

    vec3 N = normalize(toWorldCoords(global, float(imgsize)));

    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    const vec3 right = normalize(cross(up, N));
    up = cross(N, right);

    vec3 colour = vec3_splat(0.0);
    uint samples = 0;
    float deltapi = TWOPI / 360.0;
    float deltatheta = HALFPI / 90.0;

    for (float phi = 0.0; phi < TWOPI; phi += deltapi) {
        for (float theta = 0.0; theta < HALFPI; theta += deltatheta) {
            vec3 temp = cos(phi) * right + sin(phi) * up;
            vec3 samplev = cos(theta) * N + sin(theta) * temp;
            colour += textureCubeLod(s_irradiancecubemap, samplev, 0).rgb * cos(theta) * sin(theta);
            samples++;
        }
    }
    imageStore(s_target, global, vec4(PI * colour / float(samples), 1.0));
}
