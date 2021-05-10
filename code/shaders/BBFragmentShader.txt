#version 330
			in vec4 _Normal;
			in vec4 _FragPos;
			in vec2 _outUvs;
			in vec2 Vertex_UV;
			in vec4 Vertex_Color;
			//in vec3 geomPos;
			out vec4 out_Color;
			uniform mat4 mv_Mat;
			uniform vec4 lightPos;
			uniform vec4 viewPos;
			uniform vec4 lightColor;
			uniform vec4 objectColor;
			uniform sampler2D diffuseTexture;
			void main() {
				vec2 uv = Vertex_UV.xy;
				uv.y *= -1.0;
				////////////////// -Ambient
				float ambientStrength = 0.2f;
				vec4 ambient = ambientStrength * lightColor;
				////////////////// -Diffuse
				vec4 normalizedNormal = normalize(_Normal);
				vec4 lightDir = normalize(lightPos - _FragPos);
				float diffWithoutColor = max(dot(normalizedNormal, lightDir), 0.0f);
				vec4 diffuse = diffWithoutColor * lightColor;
				////////////////// -Specular
				float specularStrength = 1.0f;
				vec4 viewDir = normalize(viewPos - _FragPos);
				vec4 reflectDir = reflect(-lightDir, normalizedNormal);
				float specWithoutColor = pow(max(dot(viewDir, reflectDir), 0.0), 32);
				vec4 specular = specularStrength * specWithoutColor * lightColor;
				////////////////// -Result
				vec4 result = ambient;
				//result += diffuse;
				//result += specular;
				result *= objectColor;
				vec4 textureColor = texture(diffuseTexture, Vertex_UV);
						if(textureColor.a < 0.15)
							discard;
				//out_Color = result /*+ textureColor*/;
				out_Color = textureColor * Vertex_Color;
		}