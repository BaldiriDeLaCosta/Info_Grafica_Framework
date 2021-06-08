#version 330
	in vec2 _outUvs;
	out vec4 out_Color;
	uniform sampler2D diffuseTexture;
	
	void main() {
	//out_Color =texture(tex, vert_Tex * vec2(1.0, -1.0))*0.5+ vec4(color.xyz * dot(vert_Normal, mv_Mat*vec4(0.0, 1.0, 0.0, 0.0))*0.3 + color.xyz * 0.2, 1.0 );
		out_Color = texture(diffuseTexture, _outUvs);
}