#version 330
	in vec3 skyboxDir;
	out vec4 out_Color;
	uniform samplerCube cubeMap;
	
	void main() {
		vec4 textureColor = texture(cubeMap, skyboxDir);
		
		out_Color = textureColor;
	}