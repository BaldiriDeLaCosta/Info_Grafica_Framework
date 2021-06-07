#version 330
	layout (location = 0) in vec3 position;
	
	out vec3 texCoords;
	
	uniform mat4 mvpMat;
	uniform mat4 mv_Mat;
	
	void main() {
		texCoords = position;
		gl_Position = vec4(position, 1.0);
		
	}
	
	
	
	
	
	