#version 330
	layout(points) in;
	layout(triangle_strip, max_vertices = 3) out;		
	out vec4 eyePos;
	out vec4 centerEyePos;
	out vec4 _FragPos;
	out vec3 _Normal;
	out vec2 _outUvs;
	uniform mat4 projMat;
	uniform vec3 vertexPositions[3];
	vec4 num_Verts[3];
	in vec4 FragPos[];
	in vec3 Normal[];
	in vec2 outUvs[];
	
	void main() {
		vec3 n = normalize(-gl_in[0].gl_Position.xyz);
		vec3 up = vec3(0.0, 1.0, 0.0);
		vec3 u = normalize(cross(up, n));
		vec3 v = normalize(cross(n, u));
		num_Verts[0] = vec4(-vertexPositions[0].x*u - vertexPositions[0].x*v, 0.0);
		num_Verts[1] = vec4( vertexPositions[1].y*u - vertexPositions[1].y*v, 0.0);
		num_Verts[2] = vec4(-vertexPositions[2].z*u + vertexPositions[2].z*v, 0.0);
		centerEyePos = gl_in[0].gl_Position;
		
		for (int i = 0; i < 3; i++) {
			eyePos = (gl_in[0].gl_Position + num_Verts[i]);
			gl_Position = projMat * eyePos;
			_FragPos = FragPos[i];
			_Normal = Normal[i];
			_outUvs = outUvs[i];
			EmitVertex();
		}
		EndPrimitive();
	}