#version 330
in vec4 vert_Normal;
in vec2 vert_Tex;
out vec4 out_Color;
uniform mat4 mv_Mat;
uniform vec4 color;
uniform sampler2D tex;
void main() {
	out_Color = texture(tex, vert_Tex * vec2(1.0, 
	-1.0))*0.5+ vec4(color.xyz * dot(vert_Normal, 
	mv_Mat*vec4(0.0, 1.0, 0.0, 0.0))*0.3 + 
	color.xyz * 0.2, 1.0 );
}