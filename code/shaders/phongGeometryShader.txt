#version 330
	layout(triangles) in;
	layout(triangle_strip, max_vertices = 3) out;
	uniform float translation;
	in vec4 FragPos[];
	in vec4 Normal[];
	in vec2 outUvs[];
	//vec4 eyePos;
	out vec4 _FragPos;
	out vec4 _Normal;
	out vec2 _outUvs;
	//out vec4 geomPos;
	//uniform mat4 projMat;
	//uniform vec3 vertexPositions[3];
	//vec3 num_Verts[3];
	
	void main() {
		for (int i = 0; i < 3; i++) {
			//gl_Position = projMat * gl_in[i].gl_Position;
		
			//geomPos = epicenter(gl_in[0].gl_Position,
			//gl_in[1].gl_Position,
			//gl_in[2].gl_Position);
		
			//num_Verts[i] = vertexPositions[i];
			//eyePos = (gl_in[i].gl_Position + vec4(num_Verts[i], 1.0));
			//gl_Position = projMat * eyePos;
			_FragPos = FragPos[i];
			_Normal = Normal[i];
			_outUvs = outUvs[i];
			//EmitVertex();
			//EndPrimitive();
		}
	
		// Metemos las posiciones en variables con nombres menos engorrosos
		vec4 p1 = gl_in[0].gl_Position;
		vec4 p2 = gl_in[1].gl_Position;
		vec4 p3 = gl_in[2].gl_Position;
	
		// Calculem dos vectors de la cara mitjancant els tres vertexs donats
		vec3 vec_1 = vec3(p2.x - p1.x, p2.y - p1.y, p2.z - p1.z);
		vec3 vec_2 = vec3(p1.x - p3.x, p1.y - p3.y, p1.z - p3.z);
	
		// Calcular el vector normal fent cross product dels dos vectors que pertanyen a la cara
		vec3 norm_vec = vec3( vec_1.y*vec_2.z - vec_1.z*vec_2.y, -1* (vec_1.x*vec_2.z - vec_1.z*vec_2.x), vec_1.x*vec_2.y - vec_1.y*vec_2.x );
	
		// Calculem el denominador per normalitzar el vector
		float denominator = sqrt(pow(norm_vec.x, 2) + pow(norm_vec.y, 2) + pow(norm_vec.z, 2));
	
		// Normalitzem i retornem el vector
		vec3 normal = vec3(norm_vec.x/denominator, norm_vec.y/denominator, norm_vec.z/denominator);
	
		normal *= translation;
	
		// Movem els 3 vertexs en la normal del triangle
		gl_Position = gl_in[0].gl_Position + vec4(normal, 1.f);
		EmitVertex();
	
		gl_Position = gl_in[1].gl_Position + vec4(normal, 1.f);
		EmitVertex();
		
		gl_Position = gl_in[2].gl_Position + vec4(normal, 1.f);
		EmitVertex();
		
		EndPrimitive();
	}