#version 330
	in vec2 _outUvs;
	in vec3 _outNormal;
	in vec3 _lightIntensity;
	out vec4 out_Color;
	uniform mat4 mv_Mat;
	uniform vec4 lightPos;
	uniform vec4 viewPos;
	uniform vec4 lightColor;
	uniform vec4 objectColor;
	uniform float alpha;
	uniform sampler2D diffuseTexture;
	
	void main() {
		vec4 textureColor = texture(diffuseTexture, _outUvs * vec2(1.0, -1.0))*1.0;
		float A;
		if(alpha > 0) A = alpha - 0.01;
		else A = alpha;
		if(textureColor.a != 1) textureColor.a = alpha;
		
	
		out_Color = vec4(_lightIntensity, 1.0) * textureColor;
		
	}