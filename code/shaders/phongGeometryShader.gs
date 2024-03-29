#version 330
	layout(triangles) in;
	layout(triangle_strip, max_vertices = 3) out;
	uniform float translation;
	uniform float deltaTime;
	in vec2 outUvs[];
	in vec3 outNormal[];
	in vec3 lightIntensity[];
	out vec2 _outUvs;
	out vec3 _outNormal;
	out vec3 _lightIntensity;
	
	void main() {
		// Fiquem les posicions en variables amb noms menys engorrosos
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
	
		normal *= deltaTime;
	
		// Movem els 3 vertexs en la normal del triangle
        gl_Position = gl_in[0].gl_Position + vec4(normal, 1.f);
        _outUvs = outUvs[0];
		_outNormal = outNormal[0];
		_lightIntensity = lightIntensity[0];
        EmitVertex();

        gl_Position = gl_in[1].gl_Position + vec4(normal, 1.f);
        _outUvs = outUvs[1];
		_outNormal = outNormal[1];
		_lightIntensity = lightIntensity[1];
        EmitVertex();

        gl_Position = gl_in[2].gl_Position + vec4(normal, 1.f);
        _outUvs = outUvs[2];
		_outNormal = outNormal[2];
		_lightIntensity = lightIntensity[2];
        EmitVertex();

        EndPrimitive();
	}
	
	
	
	