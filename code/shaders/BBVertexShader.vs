#version 330
			in vec3 in_Position;
			in vec3 in_Normal;
			in vec2 uvs;
			in vec4 geomPos;
			out vec2 outUvs;
			out vec4 Normal;
			out vec4 FragPos;
			out vec4 vert_Normal;
			out vec4 LightPos;
			uniform mat4 objMat;
			uniform mat4 mv_Mat;
			uniform mat4 mvpMat;
			uniform vec4 lightPos;
			uniform vec4 in_Color;
			out Vertex	{ vec4 color; } vertex;
			void main() {
				gl_Position = vec4(in_Position, 1.f);
				vertex.color = in_Color;
				vert_Normal = mv_Mat * objMat * vec4(in_Normal, 0.0);
				Normal = mat4(transpose(inverse(mvpMat * objMat))) * vert_Normal;
				LightPos = mv_Mat * lightPos;
				FragPos = objMat * vec4(in_Position, 1.0);
				outUvs = uvs;
		}