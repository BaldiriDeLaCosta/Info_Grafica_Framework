#version 330
	layout(points) in;
	layout(triangle_strip, max_vertices = 24) out;

	out vec3 skyboxDir;
	
	uniform mat4 mv_Mat;
	uniform mat4 mvpMat;
	
	vec4 vertex[] = vec4[8] (
		vec4(-600.0, -600.0, -600.0, 1.0),
		vec4(-600.0, -600.0,  600.0, 1.0),
		vec4( 600.0, -600.0,  600.0, 1.0),
		vec4( 600.0, -600.0, -600.0, 1.0),
		vec4(-600.0,  600.0, -600.0, 1.0),
		vec4(-600.0,  600.0,  600.0, 1.0),
		vec4( 600.0,  600.0,  600.0, 1.0),
		vec4( 600.0,  600.0, -600.0, 1.0)
	);
	
	vec4 cubeVertex[] = vec4[24] (
		vertex[1], vertex[2], vertex[0], vertex[3],
		vertex[5], vertex[4], vertex[6], vertex[7],
		vertex[1], vertex[0], vertex[5], vertex[4],
		vertex[2], vertex[6], vertex[3], vertex[7],
		vertex[0], vertex[3], vertex[4], vertex[7],
		vertex[1], vertex[5], vertex[2], vertex[6]
	);
	
	
	void main() {
		for(int i = 0; i < 6; i++){
			for(int j = 0; j < 4; j++){
				vec3 offset = cubeVertex[i*4 + j].xyz;
				gl_Position = mvpMat * mv_Mat * (gl_in[0].gl_Position + vec4(offset, 1));
				skyboxDir = cubeVertex[i*4 + j].xyz;
				
				EmitVertex();
			}
			EndPrimitive();
		}
	}
	
	
	
	