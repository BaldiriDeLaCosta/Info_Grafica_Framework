#version 330
	layout(location = 0) in vec3 in_Position;
	layout(location = 1)in vec3 in_Normal;
	layout(location = 2)in vec2 uvs;
	
	out vec2 _outUvs;
	
	uniform mat4 objMat;
	uniform mat4 mv_Mat;
	uniform mat4 mvpMat;

	void main() {
		_outUvs = uvs;
		gl_Position = mvpMat * objMat * vec4(in_Position, 1.0);
}
	
	
	
	
	
	