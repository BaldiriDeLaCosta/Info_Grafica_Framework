#version 330
		layout (points) in;
		layout(triangle_strip, max_vertices = 4) out;

		uniform mat4 mv_Mat;
		uniform mat4 mvpMat;

		in Vertex
		{
		  vec4 color;
		} vertex[];


		out vec2 Vertex_UV;
		out vec4 Vertex_Color;
		
		in vec4 Normal[];
		in vec4 FragPos[];
		out vec4 _Normal;
		out vec4 _FragPos;
		

		void main(void)
		{
			//vec4 P = gl_in[0].gl_Position;
			vec3 right = vec3(mv_Mat[0][0], mv_Mat[1][0], mv_Mat[2][0]);
			vec3 up = vec3(mv_Mat[0][1], mv_Mat[1][1], mv_Mat[2][1]);
			
			vec3 P = gl_in[0].gl_Position.xyz;
			
			_Normal = Normal[0];
			_FragPos = FragPos[0];
			
			// b: Esquerra-Superior
			vec4 vb = vec4(P + right * 5.0 + up * -5.0, 1.0);
			gl_Position = mvpMat * vb;
			//gl_Position = mvpMat * P + right * (-2.0) + up * (2.0);
			Vertex_UV = vec2(0.0, 1.0);
			Vertex_Color = vertex[0].color;
			EmitVertex();

			// a: Esquerra-Inferior 
			vec4 va = vec4(P + right * 5.0 + up * 5.0, 1.0);
			gl_Position = mvpMat * va;
			//gl_Position = mvpMat * P + right * (-2.0) + up * (-2.0);
			Vertex_UV = vec2(0.0, 0.0);
			Vertex_Color = vertex[0].color;
			EmitVertex();

			// c: Dreta-Superior
			vec4 vc = vec4(P + right * -5.0 + up * -5.0, 1.0);
			gl_Position = mvpMat * vc;
			//gl_Position = P + right * (2.0) + up * (2.0);
			Vertex_UV = vec2(1.0, 1.0);
			Vertex_Color = vertex[0].color; 
			EmitVertex();

			// d: Dreta-Inferior
			vec4 vd = vec4(P + right * -5.0 + up * 5.0, 1.0);
			gl_Position = mvpMat * vd;
			//gl_Position = P + right * (2.0) + up * (-2.0);
			Vertex_UV = vec2(1.0, 0.0);
			Vertex_Color = vertex[0].color;
			EmitVertex();
			

			EndPrimitive();
		}