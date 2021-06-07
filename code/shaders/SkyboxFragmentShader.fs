#version 330
	in vec3 skyboxDir;
	out vec4 out_Color;
	uniform samplerCube diffuseTexture;
	
	void main() {
		vec4 textureColor = texture(diffuseTexture, skyboxDir);
		
		out_Color = textureColor;
	}