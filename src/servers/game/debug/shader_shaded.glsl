
@ctype mat4 mat4
@ctype vec4 vec4
@ctype vec3 vec3

// vertex
@vs vs
uniform vs_params {
    mat4 vp;
    mat4 model;
    mat4 normalMat;
};

in vec4 in_pos;
in vec4 in_normal;

out vec4 fs_worldPos;
out vec4 fs_normal;

void main() {
    fs_worldPos = model * in_pos;
    fs_normal = normalMat * vec4(in_normal.xyz, 0);
    gl_Position = vp * model * in_pos;
}
@end

@fs fs
uniform fs_params {
    vec3 color;
    vec3 lightPos;
};

in vec4 fs_worldPos;
in vec4 fs_normal;

out vec4 frag_color;

void main() {
    vec3 normal = normalize(fs_normal.xyz);
    vec3 pos = fs_worldPos.xyz;

    vec3 lightDir = normalize(lightPos - pos);
    float diffuse = max(0, dot(lightDir, normal));
    frag_color = vec4(color * diffuse, 1.0);
}
@end

@program shaded vs fs