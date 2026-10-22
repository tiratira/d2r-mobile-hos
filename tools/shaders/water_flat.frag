#version 450
// Optional diagnostic: glslangValidator -V tools/shaders/water_flat.frag -o /tmp/dm-water-flat.spv
layout(location = 0) out vec4 color;
void main() { color = vec4(0.0, 0.65, 1.0, 1.0); }
