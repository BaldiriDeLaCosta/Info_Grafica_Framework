#version 330
			layout (location = 0)in vec3 in_Position;
			in vec3 in_Normal;
			in vec2 uvs;
			in vec4 geomPos;
			out vec2 outUvs;
			out vec4 Normal;
			out vec4 FragPos;
			out vec4 vert_Normal;
			out vec4 LightPos;
			uniform mat4 objMat[10];
			uniform mat4 mv_Mat;
			uniform mat4 mvpMat;
			uniform vec4 lightPos;
			uniform vec4 in_Color;
						
			uniform vec2 offsets[10];
			out Vertex	{ vec4 color; } vertex;
			void main() {
				gl_Position = vec4(in_Position, 1.0) * objMat[gl_InstanceID];
				vertex.color = in_Color;
				vert_Normal = mv_Mat * objMat[gl_InstanceID] * vec4(in_Normal, 0.0);
				Normal = mat4(transpose(inverse(mvpMat * objMat[gl_InstanceID]))) * vert_Normal;
				LightPos = mv_Mat * lightPos;
				FragPos = objMat[gl_InstanceID] * vec4(in_Position, 1.0);
				outUvs = uvs;
		}