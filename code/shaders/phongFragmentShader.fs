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
	uniform sampler2D diffuseTexture;
	
	void main() {
		vec4 textureColor = /*texture(diffuseTexture, _outUvs)*/texture(diffuseTexture, _outUvs * vec2(1.0, 
	-1.0))*0.5;
		
		
	
		out_Color = vec4(_lightIntensity, 1.0) * textureColor;
		
	}