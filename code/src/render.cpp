#include <GL\glew.h>
#include <glm\gtc\type_ptr.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <cstdio>
#include <cassert>

#include <imgui\imgui.h>
#include <imgui\imgui_impl_sdl_gl3.h>

#include "GL_framework.h"

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>

///////// fw decl
namespace ImGui {
	void Render();
}
namespace Axis {
	void setupAxis();
	void cleanupAxis();
	void drawAxis();
}
////////////////

namespace RenderVars {
	const float FOV = glm::radians(65.f);
	const float zNear = 1.f;
	const float zFar = 500.f;

	glm::mat4 _projection;
	glm::mat4 _modelView;
	glm::mat4 _MVP;
	glm::mat4 _inv_modelview;
	glm::vec4 _cameraPoint;

	struct prevMouse {
		float lastx, lasty;
		MouseEvent::Button button = MouseEvent::Button::None;
		bool waspressed = false;
	} prevMouse;

	float panv[3] = { 0.f, -5.f, -15.f };
	float rota[2] = { 0.f, 0.f };
}
namespace RV = RenderVars;

static const GLchar* vertex_shader_source[] =
{
	"#version 330\n\
	layout (location = 0) in vec3 aPos;\n\
	void main() {\n\
	\n\
	gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0); \n\
	}"
};

static const GLchar* fragment_shader_source[] =
{
	"#version 330\n\
	\n\
	out vec4 color;\n\
	uniform vec4 aCol;\n\
	\n\
	void main() {\n\
	 color = aCol;\n\
	}"
};

float vertices[] = {
	-0.5f, -0.5f, 0.0f,
	0.5f, -0.5f, 0.0f,
	0.0f, 0.5f, 0.0f
};


void GLResize(int width, int height) {
	glViewport(0, 0, width, height);
	if (height != 0) RV::_projection = glm::perspective(RV::FOV, (float)width / (float)height, RV::zNear, RV::zFar);
	else RV::_projection = glm::perspective(RV::FOV, 0.f, RV::zNear, RV::zFar);
}

void GLmousecb(MouseEvent ev) {
	if (RV::prevMouse.waspressed && RV::prevMouse.button == ev.button) {
		float diffx = ev.posx - RV::prevMouse.lastx;
		float diffy = ev.posy - RV::prevMouse.lasty;
		switch (ev.button) {
		case MouseEvent::Button::Left: // ROTATE
			RV::rota[0] += diffx * 0.005f;
			RV::rota[1] += diffy * 0.005f;
			break;
		case MouseEvent::Button::Right: // MOVE XY
			RV::panv[0] += diffx * 0.03f;
			RV::panv[1] -= diffy * 0.03f;
			break;
		case MouseEvent::Button::Middle: // MOVE Z
			RV::panv[2] += diffy * 0.05f;
			break;
		default: break;
		}
	}
	else {
		RV::prevMouse.button = ev.button;
		RV::prevMouse.waspressed = true;
	}
	RV::prevMouse.lastx = ev.posx;
	RV::prevMouse.lasty = ev.posy;
}

//////////////////////////////////////////////////
GLuint compileShader(const char* shaderStr, GLenum shaderType, const char* name = "") {
	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &shaderStr, NULL);
	glCompileShader(shader);
	GLint res;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &res);
	if (res == GL_FALSE) {
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &res);
		char* buff = new char[res];
		glGetShaderInfoLog(shader, res, &res, buff);
		fprintf(stderr, "Error Shader %s: %s", name, buff);
		delete[] buff;
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}
void linkProgram(GLuint program) {
	glLinkProgram(program);
	GLint res;
	glGetProgramiv(program, GL_LINK_STATUS, &res);
	if (res == GL_FALSE) {
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &res);
		char* buff = new char[res];
		glGetProgramInfoLog(program, res, &res, buff);
		fprintf(stderr, "Error Link: %s", buff);
		delete[] buff;
	}
}

///////////////////////////////////////////////// LIGHT SOURCE
namespace Light
{
	glm::vec4 lightColor = glm::vec4(1.f, 1.f, 1.f, 0.f);
}

////////////////////////////////////////////////// AXIS
namespace Axis {
	GLuint AxisVao;
	GLuint AxisVbo[3];
	GLuint AxisShader[2];
	GLuint AxisProgram;

	float AxisVerts[] = {
		0.0, 0.0, 0.0,
		1.0, 0.0, 0.0,
		0.0, 0.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 0.0, 0.0,
		0.0, 0.0, 1.0
	};
	float AxisColors[] = {
		1.0, 0.0, 0.0, 1.0,
		1.0, 0.0, 0.0, 1.0,
		0.0, 1.0, 0.0, 1.0,
		0.0, 1.0, 0.0, 1.0,
		0.0, 0.0, 1.0, 1.0,
		0.0, 0.0, 1.0, 1.0
	};
	GLubyte AxisIdx[] = {
		0, 1,
		2, 3,
		4, 5
	};
	const char* Axis_vertShader =
		"#version 330\n\
in vec3 in_Position;\n\
in vec4 in_Color;\n\
out vec4 vert_color;\n\
uniform mat4 mvpMat;\n\
void main() {\n\
	vert_color = in_Color;\n\
	gl_Position = mvpMat * vec4(in_Position, 1.0);\n\
}";
	const char* Axis_fragShader =
		"#version 330\n\
in vec4 vert_color;\n\
out vec4 out_Color;\n\
void main() {\n\
	out_Color = vert_color;\n\
}";

	void setupAxis() {
		glGenVertexArrays(1, &AxisVao);
		glBindVertexArray(AxisVao);
		glGenBuffers(3, AxisVbo);

		glBindBuffer(GL_ARRAY_BUFFER, AxisVbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 24, AxisVerts, GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, AxisVbo[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 24, AxisColors, GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)1, 4, GL_FLOAT, false, 0, 0);
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, AxisVbo[2]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLubyte) * 6, AxisIdx, GL_STATIC_DRAW);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		AxisShader[0] = compileShader(Axis_vertShader, GL_VERTEX_SHADER, "AxisVert");
		AxisShader[1] = compileShader(Axis_fragShader, GL_FRAGMENT_SHADER, "AxisFrag");

		AxisProgram = glCreateProgram();
		glAttachShader(AxisProgram, AxisShader[0]);
		glAttachShader(AxisProgram, AxisShader[1]);
		glBindAttribLocation(AxisProgram, 0, "in_Position");
		glBindAttribLocation(AxisProgram, 1, "in_Color");
		linkProgram(AxisProgram);
	}
	void cleanupAxis() {
		glDeleteBuffers(3, AxisVbo);
		glDeleteVertexArrays(1, &AxisVao);

		glDeleteProgram(AxisProgram);
		glDeleteShader(AxisShader[0]);
		glDeleteShader(AxisShader[1]);
	}
	void drawAxis() {
		glBindVertexArray(AxisVao);
		glUseProgram(AxisProgram);
		glUniformMatrix4fv(glGetUniformLocation(AxisProgram, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RV::_MVP));
		glDrawElements(GL_LINES, 6, GL_UNSIGNED_BYTE, 0);

		glUseProgram(0);
		glBindVertexArray(0);
	}
}

////////////////////////////////////////////////// CUBE
namespace Cube {
	GLuint cubeVao;
	GLuint cubeVbo[3];
	GLuint cubeShaders[2];
	GLuint cubeProgram;
	glm::mat4 objMat = glm::mat4(1.f);

	extern const float halfW = 0.5f;
	int numVerts = 24 + 6; // 4 vertex/face * 6 faces + 6 PRIMITIVE RESTART

						   //   4---------7
						   //  /|        /|
						   // / |       / |
						   //5---------6  |
						   //|  0------|--3
						   //| /       | /
						   //|/        |/
						   //1---------2
	glm::vec3 verts[] = {
		glm::vec3(-halfW, -halfW, -halfW),
		glm::vec3(-halfW, -halfW,  halfW),
		glm::vec3(halfW, -halfW,  halfW),
		glm::vec3(halfW, -halfW, -halfW),
		glm::vec3(-halfW,  halfW, -halfW),
		glm::vec3(-halfW,  halfW,  halfW),
		glm::vec3(halfW,  halfW,  halfW),
		glm::vec3(halfW,  halfW, -halfW)
	};
	glm::vec3 norms[] = {
		glm::vec3(0.f, -1.f,  0.f),
		glm::vec3(0.f,  1.f,  0.f),
		glm::vec3(-1.f,  0.f,  0.f),
		glm::vec3(1.f,  0.f,  0.f),
		glm::vec3(0.f,  0.f, -1.f),
		glm::vec3(0.f,  0.f,  1.f)
	};

	glm::vec3 cubeVerts[] = {
		verts[1], verts[0], verts[2], verts[3],
		verts[5], verts[6], verts[4], verts[7],
		verts[1], verts[5], verts[0], verts[4],
		verts[2], verts[3], verts[6], verts[7],
		verts[0], verts[4], verts[3], verts[7],
		verts[1], verts[2], verts[5], verts[6]
	};
	glm::vec3 cubeNorms[] = {
		norms[0], norms[0], norms[0], norms[0],
		norms[1], norms[1], norms[1], norms[1],
		norms[2], norms[2], norms[2], norms[2],
		norms[3], norms[3], norms[3], norms[3],
		norms[4], norms[4], norms[4], norms[4],
		norms[5], norms[5], norms[5], norms[5]
	};
	GLubyte cubeIdx[] = {
		0, 1, 2, 3, UCHAR_MAX,
		4, 5, 6, 7, UCHAR_MAX,
		8, 9, 10, 11, UCHAR_MAX,
		12, 13, 14, 15, UCHAR_MAX,
		16, 17, 18, 19, UCHAR_MAX,
		20, 21, 22, 23, UCHAR_MAX
	};

#pragma region cube_Shaders
	const char* cube_vertShader =
		"#version 330\n\
			in vec3 in_Position;\n\
			in vec3 in_Normal;\n\
			out vec4 vert_Normal;\n\
			uniform mat4 objMat;\n\
			uniform mat4 mv_Mat;\n\
			uniform mat4 mvpMat;\n\
			\n\
			void main() {\n\
				gl_Position = mvpMat * objMat * vec4(in_Position, 1.0);\n\
				vert_Normal = mv_Mat * objMat * vec4(in_Normal, 0.0);\n\
			}";

	const char* cube_fragShader =
		"#version 330"
		"in vec4 vert_Normal;"
		"out vec4 out_Color;"
		"uniform mat4 mv_Mat;"
		"uniform vec4 color;"
		"uniform vec4 ambient;"

		"void main() {"
		"out_Color = color * ambient;"
		"}";
#pragma endregion

	void setupCube() {
		glGenVertexArrays(1, &cubeVao);
		glBindVertexArray(cubeVao);
		glGenBuffers(3, cubeVbo);

		glBindBuffer(GL_ARRAY_BUFFER, cubeVbo[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVerts), cubeVerts, GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, cubeVbo[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(cubeNorms), cubeNorms, GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(1);

		glPrimitiveRestartIndex(UCHAR_MAX);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeVbo[2]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIdx), cubeIdx, GL_STATIC_DRAW);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		cubeShaders[0] = compileShader(cube_vertShader, GL_VERTEX_SHADER, "cubeVert");
		cubeShaders[1] = compileShader(cube_fragShader, GL_FRAGMENT_SHADER, "cubeFrag");

		cubeProgram = glCreateProgram();
		glAttachShader(cubeProgram, cubeShaders[0]);
		glAttachShader(cubeProgram, cubeShaders[1]);
		glBindAttribLocation(cubeProgram, 0, "in_Position");
		glBindAttribLocation(cubeProgram, 1, "in_Normal");
		linkProgram(cubeProgram);
	}
	void cleanupCube() {
		glDeleteBuffers(3, cubeVbo);
		glDeleteVertexArrays(1, &cubeVao);

		glDeleteProgram(cubeProgram);
		glDeleteShader(cubeShaders[0]);
		glDeleteShader(cubeShaders[1]);
	}
	void updateCube(const glm::mat4& transform) {
		objMat = transform;
	}
	void drawCube() {
		glEnable(GL_PRIMITIVE_RESTART);
		glBindVertexArray(cubeVao);
		glUseProgram(cubeProgram);
		glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "objMat"), 1, GL_FALSE, glm::value_ptr(objMat));
		glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
		glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
		glUniform4f(glGetUniformLocation(cubeProgram, "color"), 0.1f, 1.f, 1.f, 0.f);
		glDrawElements(GL_TRIANGLE_STRIP, numVerts, GL_UNSIGNED_BYTE, 0);

		glUseProgram(0);
		glBindVertexArray(0);
		glDisable(GL_PRIMITIVE_RESTART);
	}
	void drawTwoCubes()
	{
		float currentTime = ImGui::GetTime();
		float cubeColor = currentTime;
		float cubeScale = 1 - (float)sin(currentTime);
		if ((float)sin(cubeColor) < 0) cubeColor = 0;
		if (cubeScale < 1.f) cubeScale = 1.f;

		glEnable(GL_PRIMITIVE_RESTART);
		//glm::mat4 TranslationMatrix = glm::translate(glm::mat4(), glm::vec3(-1.0f, 2.0f, 3.0f));
		//objMat = TranslationMatrix;

		glBindVertexArray(cubeVao);
		glUseProgram(cubeProgram);
		glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "objMat"), 1, GL_FALSE, glm::value_ptr(objMat));
		glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
		glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
		/*glUniform4f(glGetUniformLocation(cubeProgram, "color"), 0.1f, 0.5f+0.5f*sin(currentTime), 1.f, 0.f);
		glDrawElements(GL_TRIANGLE_STRIP, numVerts, GL_UNSIGNED_BYTE, 0);*/

		////Translation matrix
		//TranslationMatrix = glm::translate(glm::mat4(), glm::vec3(-1.0f, 2.0f, 3.0f));
		////Scale Matrix
		//glm::mat4 ScaleMatrix = glm::scale(glm::mat4(), glm::vec3(2, 2, 2));
		////Rotation Matrix
		//glm::mat4 RotationMatrix = glm::rotate(glm::mat4(), currentTime * 3.f, glm::vec3(0.0f, 1.0f, 0.0f));

		////Secondary Translation Matrix
		//glm::mat4 TranslationMatrix2 = glm::translate(glm::mat4(), glm::vec3(3.0f, 0.0f, 0.0f));

		////Les operacions amb matrius es fan d'esquerra a dreta -->
		////la llògica es fa de dreta a esquerra <--
		//objMat = TranslationMatrix * RotationMatrix * TranslationMatrix2 * ScaleMatrix;

		glUniformMatrix4fv(glGetUniformLocation(cubeProgram, "objMat"), 1, GL_FALSE, glm::value_ptr(objMat));
		glUniform4f(glGetUniformLocation(cubeProgram, "color"), 0.1f, 1.f, 1.f, 0.f);
		glUniform4f(glGetUniformLocation(cubeProgram, "ambient"), Light::lightColor.r * 0.4f, Light::lightColor.g * 0.4f, Light::lightColor.b * 0.4f, 0.0f);
		glUniform4f(glGetUniformLocation(cubeProgram, "diffuse"), 1.0f, 1.0f, 1.0f, 1.0f);
		glUniform4f(glGetUniformLocation(cubeProgram, "directional_light"), 0.0f, 3.0f, 0.0f, 1.0f);
		glUniform3f(glGetUniformLocation(cubeProgram, "CameraPos"), -RV::panv[0], -RV::panv[1], -RV::panv[2]);
		glUniform4f(glGetUniformLocation(cubeProgram, "specular"), 1.0f, 1.0f, 1.0f, 1.0f);


		glDrawElements(GL_TRIANGLE_STRIP, numVerts, GL_UNSIGNED_BYTE, 0);
		
		glUseProgram(0);
		glBindVertexArray(0);
		glDisable(GL_PRIMITIVE_RESTART);
	}
}

/////////////////////////////////////////////////

#pragma region Object
class Object {
public:
	static enum class Type { DRAGON, CUBE, COUNT };
	
	std::vector< unsigned int > vertexIndices, uvIndices, normalIndices;
	std::vector< glm::vec3 > temp_vertices;
	std::vector< glm::vec2 > temp_uvs;
	std::vector< glm::vec3 > temp_normals;

	std::vector< glm::vec3 > vertices;
	std::vector< glm::vec2 > uvs;
	std::vector< glm::vec3 > normals; // Won't be used at the moment.

	GLuint objectVao;
	GLuint objectVbo[3];
	GLuint objectShaders[2];
	GLuint objectProgram;

	bool available = false;
	bool enabled = true;
	glm::mat4 objMat = glm::mat4(1.f);
	glm::vec3 initPos;


#pragma region cubeShaders
	const char* cube_vertShader =
		"#version 330\n\
			in vec3 in_Position;\n\
			in vec3 in_Normal;\n\
			out vec4 vert_Normal;\n\
			uniform mat4 objMat;\n\
			uniform mat4 mv_Mat;\n\
			uniform mat4 mvpMat;\n\
			out vec4 Normal;\n\
			out vec4 LightPos;\n\
			out vec4 FragPos;\n\
			uniform vec4 lightPos;\n\
			uniform mat4 model;\n\
			uniform mat4 view;\n\
			uniform mat4 projection;\n\
			void main() {\n\
				gl_Position = mvpMat * objMat * vec4(in_Position, 1.0);\n\
				vert_Normal = mv_Mat * objMat * vec4(in_Normal, 0.0);\n\
				Normal = mat4(transpose(inverse(mvpMat * objMat))) * vert_Normal;\n\
				LightPos = mvpMat * lightPos;\n\
				FragPos = mvpMat * objMat * vec4(in_Position, 1.0);\n\
		}";

	const char* cube_fragShader =
		"#version 330\n\
			in vec4 vert_Normal;\n\
			in vec4 Normal;\n\
			in vec4 FragPos;\n\
			out vec4 out_Color;\n\
			uniform mat4 mv_Mat;\n\
			uniform vec4 lightPos;\n\
			uniform vec4 viewPos;\n\
			uniform vec4 lightColor;\n\
			uniform vec4 objectColor;\n\
			void main() {\n\
				////////////////// -Ambient\n\
				float ambientStrength = 0.2f;\n\
				vec4 ambient = ambientStrength * lightColor;\n\
				////////////////// -Diffuse\n\
				vec4 normalizedNormal = normalize(Normal);\n\
				vec4 lightDir = normalize(lightPos - FragPos);\n\
				float diffWithoutColor = max(dot(normalizedNormal, lightDir), 0.0f);\n\
				vec4 diffuse = diffWithoutColor * lightColor;\n\
				////////////////// -Specular\n\
				float specularStrength = 1.0f;\n\
				vec4 viewDir = normalize(viewPos - FragPos);\n\
				vec4 reflectDir = reflect(-lightDir, normalizedNormal);\n\
				float specWithoutColor = pow(max(dot(viewDir, reflectDir), 0.0), 32);\n\
				vec4 specular = specularStrength * specWithoutColor * lightColor;\n\
				////////////////// -Result\n\
				vec4 result = ambient;\n\
				result += diffuse;\n\
				result += specular;\n\
				result *= objectColor;\n\
				out_Color = result;\n\
		}";
#pragma endregion

	bool loadOBJ(const char* path,
		std::vector < glm::vec3 >& out_vertices,
		std::vector < glm::vec2 >& out_uvs,
		std::vector < glm::vec3 >& out_normals
	) {
		FILE* file = fopen(path, "r");
		if (file == NULL) {
			printf("Impossible to open the file !\n");
			return false;
		}

		while (1) {

			char lineHeader[128];
			// read the first word of the line
			int res = fscanf(file, "%s", lineHeader);
			if (res == EOF) {
				break; // EOF = End Of File. Quit the loop.
			}

			if (strcmp(lineHeader, "v") == 0) {
				glm::vec3 vertex;
				fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
				temp_vertices.push_back(vertex);
			}
			else if (strcmp(lineHeader, "vt") == 0) {
				glm::vec2 uv;
				fscanf(file, "%f %f\n", &uv.x, &uv.y);
				temp_uvs.push_back(uv);
			}
			else if (strcmp(lineHeader, "vn") == 0) {
				glm::vec3 normal;
				fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
				temp_normals.push_back(normal);
			}
			else if (strcmp(lineHeader, "f") == 0) {
				std::string vertex1, vertex2, vertex3;
				unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
				int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
				if (matches != 9) {
					printf("File can't be read by our simple parser : ( Try exporting with other options\n");
					return false;
				}
				vertexIndices.push_back(vertexIndex[0]);
				vertexIndices.push_back(vertexIndex[1]);
				vertexIndices.push_back(vertexIndex[2]);
				uvIndices.push_back(uvIndex[0]);
				uvIndices.push_back(uvIndex[1]);
				uvIndices.push_back(uvIndex[2]);
				normalIndices.push_back(normalIndex[0]);
				normalIndices.push_back(normalIndex[1]);
				normalIndices.push_back(normalIndex[2]);
			}

		}

		// For each vertex of each triangle
		for (unsigned int i = 0; i < vertexIndices.size(); i++) {
			// Vertex
			unsigned int vertexIndex = vertexIndices[i];
			glm::vec3 vertex = temp_vertices[vertexIndex - 1];
			out_vertices.push_back(vertex);

			// UV
			unsigned int uvsIndex = uvIndices[i];
			glm::vec2 uv = temp_uvs[uvsIndex - 1];
			out_uvs.push_back(uv);

			// Normal
			unsigned int normalIndex = normalIndices[i];
			glm::vec3 normal = temp_normals[normalIndex - 1];
			out_normals.push_back(normal);

		}

		return true;
	}

	void setupObject(Type _type, glm::vec3 _initPos) {
		switch (_type) {
		case Type::DRAGON:
			available = loadOBJ("resources/dragon.obj.txt", vertices, uvs, normals);
			break;
		case Type::CUBE:
			available = loadOBJ("resources/cube.obj.txt", vertices, uvs, normals);
			break;

		default:;

		}
		initPos = _initPos;
		//glUniformMatrix4fv(glGetUniformLocation(objectProgram, "objMat"), 1, GL_FALSE, glm::value_ptr(glm::translate(glm::mat4(), _initPos)));

		if (available) {
			glGenVertexArrays(1, &objectVao);
			glBindVertexArray(objectVao);
			glGenBuffers(3, objectVbo);


			glBindBuffer(GL_ARRAY_BUFFER, objectVbo[0]);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
			glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
			/*glVertexAttribPointer(
				0,
				3,
				GL_FLOAT,
				GL_FALSE,
				3 * sizeof(float),
				(void*)0);*/
			glEnableVertexAttribArray(0);

			glBindBuffer(GL_ARRAY_BUFFER, objectVbo[1]);
			glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
			glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
			glEnableVertexAttribArray(1);


			glBindVertexArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			
			objectShaders[0] = compileShader(cube_vertShader, GL_VERTEX_SHADER, "cubeVert");
			objectShaders[1] = compileShader(cube_fragShader, GL_FRAGMENT_SHADER, "cubeFrag");
			
			objectProgram = glCreateProgram();
			glAttachShader(objectProgram, objectShaders[0]);
			glAttachShader(objectProgram, objectShaders[1]);
			glBindAttribLocation(objectProgram, 0, "in_Position");
			glBindAttribLocation(objectProgram, 1, "in_Normal");
			linkProgram(objectProgram);

		}
	}

	void cleanObject() {
		glDeleteBuffers(3, objectVbo);
		glDeleteVertexArrays(1, &objectVao);

		glDeleteProgram(objectProgram);
		glDeleteShader(objectShaders[0]);
		glDeleteShader(objectShaders[1]);
	}

	void updateObject(const glm::mat4& transform) {	// Optativo de momento
		if (available && enabled) {
			glUseProgram(objectProgram);
			objMat = transform;
			glUseProgram(0);
		}
	}
	void drawObject() {
		if (available && enabled) {
			
			//glEnable(GL_PRIMITIVE_RESTART);
			glBindVertexArray(objectVao);
			glUseProgram(objectProgram);
			glUniformMatrix4fv(glGetUniformLocation(objectProgram, "objMat"), 1, GL_FALSE, glm::value_ptr(objMat));
			glUniformMatrix4fv(glGetUniformLocation(objectProgram, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
			glUniformMatrix4fv(glGetUniformLocation(objectProgram, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
			glUniform4f(glGetUniformLocation(objectProgram, "color"), 1.f, 0.1f, 1.f, 0.f);
			glUniform4f(glGetUniformLocation(objectProgram, "objectColor"), 1.f, 1.0f, 1.0f, 0.0f);
			glUniform4f(glGetUniformLocation(objectProgram, "lightColor"), 0.0f, 1.0f, 0.0f, 1.0f);
			glUniform4f(glGetUniformLocation(objectProgram, "lightPos"), 0.f, 0.f, -10.f, 0.0f);
			glUniform4f(glGetUniformLocation(objectProgram, "viewPos"), RV::_cameraPoint.x, RV::_cameraPoint.y, RV::_cameraPoint.z, 0);

			glUniformMatrix4fv(glGetUniformLocation(objectProgram, "objMat"), 1, GL_FALSE, glm::value_ptr(glm::translate(glm::mat4(), initPos)));

			//glDrawElements(GL_TRIANGLE_STRIP, vertices.size(), GL_UNSIGNED_BYTE, 0);
			glDrawArrays(GL_TRIANGLES, 0, vertices.size() * 3);

			glUseProgram(0);
			glBindVertexArray(0);
			//glDisable(GL_PRIMITIVE_RESTART);
		}
	}

	//bool loadObject() {
	//	bool res = loadOBJ("resources/cube.obj.txt", vertices, uvs, normals);

	//	//glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

	//	return res;
	//}

};
#pragma endregion

/////////////////////////////////////////////////

GLuint program;
GLuint VAO;
GLuint VBO;

Object obj1;

void GLinit(int width, int height) {
	glViewport(0, 0, width, height);
	glClearColor(0.2f, 0.2f, 0.2f, 1.f);
	glClearDepth(1.f);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	RV::_projection = glm::perspective(RV::FOV, (float)width / (float)height, RV::zNear, RV::zFar);

	// Setup shaders & geometry
	Axis::setupAxis();
	Cube::setupCube();

	obj1.setupObject(Object::Type::DRAGON, glm::vec3(0, 20, 0));

	/////////////////////////////////////////////////////TODO
	GLuint vertex_shader;
	GLuint fragment_shader;

	//Compile the shaders
	vertex_shader =
		glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1,
		vertex_shader_source, NULL);
	glCompileShader(vertex_shader);

	fragment_shader =
		glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1,
		fragment_shader_source, NULL);
	glCompileShader(fragment_shader);

	//Create the program from the shaders
	program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);

	//Delete the shaders
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &VBO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(
		0,
		3,
		GL_FLOAT,
		GL_FALSE,
		3 * sizeof(float),
		(void*)0);

	glEnableVertexAttribArray(0);

	glBindVertexArray(0);


	/////////////////////////////////////////////////////////
}

void GLcleanup() {
	Axis::cleanupAxis();
	Cube::cleanupCube();

	/////////////////////////////////////////////////////TODO
	// Do your cleanup code here
	// ...
	// ...
	// ...
	/////////////////////////////////////////////////////////
}

void GLrender(float dt) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	RV::_modelView = glm::mat4(1.f);
	RV::_modelView = glm::translate(RV::_modelView, glm::vec3(RV::panv[0], RV::panv[1], RV::panv[2]));
	RV::_modelView = glm::rotate(RV::_modelView, RV::rota[1], glm::vec3(1.f, 0.f, 0.f));
	RV::_modelView = glm::rotate(RV::_modelView, RV::rota[0], glm::vec3(0.f, 1.f, 0.f));

	RV::_MVP = RV::_projection * RV::_modelView;

	Axis::drawAxis();
	//Cube::drawTwoCubes();

	obj1.drawObject();

	//float currentTime = ImGui::GetTime();

	//const GLfloat color[] = { (float)sin(currentTime) * 0.5f + 0.5f, (float)cos(currentTime) * 0.5f + 0.5f, 0.0f, 1.0f };
	//glClearBufferfv(GL_COLOR, 0, color);

	//GLuint unifLocation = glGetUniformLocation(program, "aCol");
	//glUniform4f(unifLocation, (float)sin(currentTime) * 0.5f + 0.5f, (float)cos(currentTime) * 0.5f + 0.5f, 0.0f, 1.0f);
	//		
	//glUseProgram(program);
	//glPointSize(40.0f);
	//glBindVertexArray(VAO);
	//glDrawArrays(GL_TRIANGLES, 0, 3);

	/////////////////////////////////////////////////////TODO
	// Do your render code here

	/////////////////////////////////////////////////////////

	ImGui::Render();
}

void GUI() {
	bool show = true;
	ImGui::Begin("Physics Parameters", &show, 0);

	{
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		/////////////////////////////////////////////////////TODO
		// Do your GUI code here....
		// ...
		// ...
		// ...
		/////////////////////////////////////////////////////////
	}
	// .........................

	ImGui::End();

	// Example code -- ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
	bool show_test_window = false;
	if (show_test_window) {
		ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
		ImGui::ShowTestWindow(&show_test_window);
	}
}

