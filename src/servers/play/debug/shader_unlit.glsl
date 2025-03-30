@ctype mat4 mat4
@ctype vec4 vec4
@ctype vec3 vec3

// vertex
@vs vs
uniform vs_params {
    mat4 mvp;
    vec3 color;
};

in vec4 in_pos;
in vec4 in_normal;

out vec4 fs_color;

void main() {
    fs_color = vec4(color, 1.0);
    gl_Position = mvp * in_pos;
}
@end

@fs fs
in vec4 fs_color;

out vec4 frag_color;

void main() {
    frag_color = fs_color;
}
@end

@program unlit vs fs