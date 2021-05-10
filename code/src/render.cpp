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
#include <ctime>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int x, y, n;
unsigned char* data;

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
	float FOV = glm::radians(65.f);
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

	float panv[3] = { 0.f, -5.f, -16.f };
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
	glm::vec4 lightColor = glm::vec4(1.f, 1.f, 1.f, 1.f);
	glm::vec4 lightPosition = glm::vec4(0.f, 5.f, -10.f, 0.0f);
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
float startTime, actualTime, deltaTime;


#pragma region ShaderRegion

class Shader {
private:
	GLuint program;
	GLuint* shaders;
	int shadersSize = 0;
	unsigned int textureID;

	/*const char* vertShader;
	const char* fragShader;
	const char* geomShader;*/


	GLuint compileShaderFromFile(const char* shaderPath, GLenum shaderType, const char* name = "") {
		//char* shaderStr = new char;

		//Checker for the file being open
		FILE* file = fopen(shaderPath, "rb");
		fseek(file, 0, SEEK_END);
		long fsize = ftell(file);
		fseek(file, 0, SEEK_SET);

		char* shaderStr = (char*)malloc(fsize + 1);
		fread(shaderStr, fsize, 1, file);
		fclose(file);

		shaderStr[fsize] = 0;

		//printf("\n\n%s: %s\n", name, shaderStr);


		GLuint shader = glCreateShader(shaderType);
		glShaderSource(shader, 1, &shaderStr, NULL);
		glCompileShader(shader);
		GLint res;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &res);
		if (res == GL_FALSE) {
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &res);
			char* buff = new char[res];
			glGetShaderInfoLog(shader, res, &res, buff);
			fprintf(stderr, "Error Shader %s: %s", name, buff);	//Error aqui
			delete[] buff;
			glDeleteShader(shader);
			return 0;
		}
		return shader;
	}

public:
	//unsigned int id;

	Shader() {};
	Shader(const char* vertexPath, const char* fragmentPath) {
		shadersSize = 2;
		shaders = new GLuint[shadersSize];
		shaders[0] = compileShaderFromFile(vertexPath, GL_VERTEX_SHADER, "vertexShader");
		shaders[1] = compileShaderFromFile(fragmentPath, GL_FRAGMENT_SHADER, "fragmentShader");

		program = glCreateProgram();
		glAttachShader(program, shaders[0]);
		glAttachShader(program, shaders[1]);

		glBindAttribLocation(program, 0, "in_Position");
		glBindAttribLocation(program, 1, "in_Normal");
		glBindAttribLocation(program, 2, "uvs");

		linkProgram(program);
	}
	Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath) {
		shadersSize = 3;
		shaders = new GLuint[shadersSize];
		shaders[0] = compileShaderFromFile(vertexPath, GL_VERTEX_SHADER, "vertexShader");
		shaders[1] = compileShaderFromFile(fragmentPath, GL_FRAGMENT_SHADER, "fragmentShader");
		shaders[2] = compileShaderFromFile(geometryPath, GL_GEOMETRY_SHADER, "geometryShader");


		program = glCreateProgram();
		glAttachShader(program, shaders[0]);
		glAttachShader(program, shaders[1]);
		glAttachShader(program, shaders[2]);

		glBindAttribLocation(program, 0, "in_Position");
		glBindAttribLocation(program, 1, "in_Normal");
		glBindAttribLocation(program, 2, "uvs");

		linkProgram(program);
	}
	Shader(const Shader& s) {
		textureID = s.textureID;

		//delete[] shaders;
		shadersSize = s.shadersSize;
		shaders = new GLuint[shadersSize];
		for (int i = 0; i < shadersSize; i++) {
			shaders[i] = s.shaders[i];
		}

		program = s.program;
	}

	GLuint GetProgram()
	{
		return program;
	}

	void AddTextureID(const char* texturePath) {
		data = stbi_load(texturePath, &x, &y, &n, 4);
		//stbi_image_free(data);
		glGenTextures(1, &textureID); // Create the handle of the texture
		glBindTexture(GL_TEXTURE_2D, textureID); //Bind it
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data); //Load the data
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	void SetTextureID(unsigned int _textureID) {
		textureID = _textureID;
	}
	static unsigned int CreateTextureID(const char* texturePath) {
		unsigned int textureID;

		data = stbi_load(texturePath, &x, &y, &n, 4);
		//stbi_image_free(data);
		glGenTextures(1, &textureID); // Create the handle of the texture
		glBindTexture(GL_TEXTURE_2D, textureID); //Bind it
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data); //Load the data
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		return textureID;
	}

	void UseProgram() {
		glUseProgram(program);
	}

	void UseTexture() {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glUniform1i(glGetUniformLocation(program, "diffuseTexture"), 0);
	}

	GLuint GetShader(int shaderPos) {
		if (shaderPos >= shadersSize || shaderPos < 0)
			return NULL;

		return shaders[shaderPos];
	}

	int GetShadersSize() {
		return shadersSize;
	}

	void Delete() {
		glDeleteProgram(program);
		glDeleteShader(shaders[0]);
		glDeleteShader(shaders[1]);
		glDeleteShader(shaders[2]);

		glDeleteTextures(1, &textureID);
	}

	//setters for uniforms
	void SetUniformsMats(const glm::mat4& objMat) {
		glUniformMatrix4fv(glGetUniformLocation(program, "objMat"), 1, GL_FALSE, glm::value_ptr(objMat));
		glUniformMatrix4fv(glGetUniformLocation(program, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
		glUniformMatrix4fv(glGetUniformLocation(program, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
	}

	void SetUniformsLights(const glm::vec3& objectColor) {
		glUniform4f(glGetUniformLocation(program, "color"), 1.f, 0.1f, 1.f, 0.f);
		glUniform4f(glGetUniformLocation(program, "objectColor"), objectColor.r, objectColor.g, objectColor.b, 0.0f);
		glUniform4f(glGetUniformLocation(program, "lightColor"), Light::lightColor.r, Light::lightColor.g, Light::lightColor.b, 1.0f);
		glUniform4f(glGetUniformLocation(program, "lightPos"), Light::lightPosition.x, Light::lightPosition.y, Light::lightPosition.z, Light::lightPosition.w);
		glUniform4f(glGetUniformLocation(program, "viewPos"), RV::panv[0], RV::panv[1], RV::panv[2], 0);
		glm::vec3 vertexArray[3] = { glm::vec3(-1.f, -1.f, 2.f), glm::vec3(0.f, 3.f, 1.f), glm::vec3(2.f, 1.f, 0.f) };
		glUniform3fv(glGetUniformLocation(program, "vertexPositions"), 3, glm::value_ptr(vertexArray[0]));
		glUniform4f(glGetUniformLocation(program, "in_Color"), objectColor.r, objectColor.g, objectColor.b, 0.0f);
	}
	void SetUniformsLights(const glm::vec3& objectColor, const glm::vec3& lightColor) {
		glUniform4f(glGetUniformLocation(program, "color"), 1.f, 0.1f, 1.f, 0.f);
		glUniform4f(glGetUniformLocation(program, "objectColor"), objectColor.r, objectColor.g, objectColor.b, 0.0f);
		glUniform4f(glGetUniformLocation(program, "lightColor"), lightColor.r, lightColor.g, lightColor.b, 1.0f);
		glUniform4f(glGetUniformLocation(program, "lightPos"), Light::lightPosition.x, Light::lightPosition.y, Light::lightPosition.z, Light::lightPosition.w);
		glUniform4f(glGetUniformLocation(program, "viewPos"), RV::panv[0], RV::panv[1], RV::panv[2], 0);
		glm::vec3 vertexArray[3] = { glm::vec3(-1.f, -1.f, 2.f), glm::vec3(0.f, 3.f, 1.f), glm::vec3(2.f, 1.f, 0.f) };
		glUniform3fv(glGetUniformLocation(program, "vertexPositions"), 3, glm::value_ptr(vertexArray[0]));
		glUniform4f(glGetUniformLocation(program, "in_Color"), objectColor.r, objectColor.g, objectColor.b, 0.0f);
	}

};

#pragma endregion

/////////////////////////////////////////////////
#pragma region Object
class Object {
public:
	//Enum class for diferent object models
	static enum class Type { CHARACTER, CUBE, QUAD, COUNT };

	std::vector< unsigned int > vertexIndices, uvIndices, normalIndices;
	std::vector< glm::vec3 > temp_vertices;
	std::vector< glm::vec2 > temp_uvs;
	std::vector< glm::vec3 > temp_normals;

	std::vector< glm::vec3 > vertices;
	std::vector< glm::vec2 > uvs;
	std::vector< glm::vec3 > normals; // Won't be used at the moment.

	Shader shader;

	GLuint objectVao;
	GLuint objectVbo[3];

	//GLuint objectShaders[3];
	//GLuint objectProgram;
	//GLuint textureID;

	//Setup Variables
	bool available = false;
	bool enabled = true;
	glm::mat4 objMat = glm::mat4(1.f);
	glm::vec3 initPos;
	float rotation;
	glm::vec3 rotationAxis;
	glm::vec3 modelSize;
	glm::vec4 objectColor;

#pragma region cubeShaders
	//Vertex Shader for the objects
	const char* cube_vertShader =
		"#version 330\n\
			in vec3 in_Position;\n\
			in vec3 in_Normal;\n\
			in vec2 uvs;\n\
			in vec4 geomPos;\n\
			out vec2 outUvs;\n\
			out vec4 Normal;\n\
			out vec4 FragPos;\n\
			out vec4 vert_Normal;\n\
			out vec4 LightPos;\n\
			uniform mat4 objMat;\n\
			uniform mat4 mv_Mat;\n\
			uniform mat4 mvpMat;\n\
			uniform vec4 lightPos;\n\
			void main() {\n\
				gl_Position = mvpMat * objMat * vec4(in_Position, 1.f);\n\
				vert_Normal = mv_Mat * objMat * vec4(in_Normal, 0.0);\n\
				Normal = mat4(transpose(inverse(mvpMat * objMat))) * vert_Normal;\n\
				LightPos = mv_Mat * lightPos;\n\
				FragPos = objMat * vec4(in_Position, 1.0);\n\
				outUvs = uvs;\n\
		}";

	//Fragment Shader for the objects
	const char* cube_fragShader =
		"#version 330\n\
			in vec4 _Normal;\n\
			in vec4 _FragPos;\n\
			in vec2 _outUvs;\n\
			//in vec3 geomPos;\n\
			out vec4 out_Color;\n\
			uniform mat4 mv_Mat;\n\
			uniform vec4 lightPos;\n\
			uniform vec4 viewPos;\n\
			uniform vec4 lightColor;\n\
			uniform vec4 objectColor;\n\
			uniform sampler2D diffuseTexture;\n\
			void main() {\n\
				////////////////// -Ambient\n\
				float ambientStrength = 0.2f;\n\
				vec4 ambient = ambientStrength * lightColor;\n\
				////////////////// -Diffuse\n\
				vec4 normalizedNormal = normalize(_Normal);\n\
				vec4 lightDir = normalize(lightPos - _FragPos);\n\
				float diffWithoutColor = max(dot(normalizedNormal, lightDir), 0.0f);\n\
				vec4 diffuse = diffWithoutColor * lightColor;\n\
				////////////////// -Specular\n\
				float specularStrength = 1.0f;\n\
				vec4 viewDir = normalize(viewPos - _FragPos);\n\
				vec4 reflectDir = reflect(-lightDir, normalizedNormal);\n\
				float specWithoutColor = pow(max(dot(viewDir, reflectDir), 0.0), 32);\n\
				vec4 specular = specularStrength * specWithoutColor * lightColor;\n\
				////////////////// -Result\n\
				vec4 result = ambient;\n\
				//result += diffuse;\n\
				//result += specular;\n\
				result *= objectColor;\n\
				vec4 textureColor = texture(diffuseTexture, _outUvs);\n\
				out_Color = result /*+ textureColor*/;\n\
		}";

	const char* cube_geomShader =
		"#version 330\n\
		layout(triangles) in;\n\
		layout(triangle_strip, max_vertices = 3) out;\n\
		uniform float translation;\n\
		in vec4 FragPos[];\n\
		in vec4 Normal[];\n\
		in vec2 outUvs[];\n\
		//vec4 eyePos;\n\
		out vec4 _FragPos;\n\
		out vec4 _Normal;\n\
		out vec2 _outUvs;\n\
		//out vec4 geomPos;\n\
		//uniform mat4 projMat;\n\
		//uniform vec3 vertexPositions[3];\n\
		//vec3 num_Verts[3];\n\
		void main() {\n\
		 for (int i = 0; i < 3; i++) {\n\
				//gl_Position = projMat * gl_in[i].gl_Position;\n\
				\n\
				//geomPos = epicenter(gl_in[0].gl_Position,\n\
				//gl_in[1].gl_Position,\n\
				//gl_in[2].gl_Position);\n\
				\n\
				//num_Verts[i] = vertexPositions[i];\n\
				//eyePos = (gl_in[i].gl_Position + vec4(num_Verts[i], 1.0)); \n\
				//gl_Position = projMat * eyePos;\n\
				_FragPos = FragPos[i];\n\
				_Normal = Normal[i];\n\
				_outUvs = outUvs[i];\n\
				//EmitVertex();\n\
		 //EndPrimitive(); \n\
		 }\n\
		// Metemos las posiciones en variables con nombres menos engorrosos\n\
		vec4 p1 = gl_in[0].gl_Position;\n\
		vec4 p2 = gl_in[1].gl_Position; \n\
		vec4 p3 = gl_in[2].gl_Position;\n\
	\n\
		// Calculem dos vectors de la cara mitjancant els tres vertexs donats\n\
		vec3 vec_1 = vec3(p2.x - p1.x, p2.y - p1.y, p2.z - p1.z);\n\
		vec3 vec_2 = vec3(p1.x - p3.x, p1.y - p3.y, p1.z - p3.z);\n\
	\n\
		// Calcular el vector normal fent cross product dels dos vectors que pertanyen a la cara\n\
		vec3 norm_vec = vec3( vec_1.y*vec_2.z - vec_1.z*vec_2.y, -1* (vec_1.x*vec_2.z - vec_1.z*vec_2.x), vec_1.x*vec_2.y - vec_1.y*vec_2.x );\n\
	\n\
		// Calculem el denominador per normalitzar el vector\n\
		float denominator = sqrt(pow(norm_vec.x, 2) + pow(norm_vec.y, 2) + pow(norm_vec.z, 2));\n\
	\n\
		// Normalitzem i retornem el vector\n\
		vec3 normal = vec3(norm_vec.x/denominator, norm_vec.y/denominator, norm_vec.z/denominator);\n\
	\n\
		normal *= translation;\n\
	\n\
		// Movem els 3 vertexs en la normal del triangle\n\
		gl_Position = gl_in[0].gl_Position + vec4(normal, 1.f);\n\
		EmitVertex();\n\
	\n\
		gl_Position = gl_in[1].gl_Position + vec4(normal, 1.f);\n\
		EmitVertex();\n\
	\n\
		gl_Position = gl_in[2].gl_Position + vec4(normal, 1.f);\n\
		EmitVertex();\n\
	\n\
			 EndPrimitive(); \n\
		}";
#pragma endregion

#pragma region cubeShadersBB
	const char* cube_vertShaderBB =
		"#version 330\n\
			in vec3 in_Position;\n\
			in vec3 in_Normal;\n\
			in vec2 uvs;\n\
			in vec4 geomPos;\n\
			out vec2 outUvs;\n\
			out vec4 Normal;\n\
			out vec4 FragPos;\n\
			out vec4 vert_Normal;\n\
			out vec4 LightPos;\n\
			uniform mat4 objMat;\n\
			uniform mat4 mv_Mat;\n\
			uniform mat4 mvpMat;\n\
			uniform vec4 lightPos;\n\
			uniform vec4 in_Color;\n\
			out Vertex	{ vec4 color; } vertex;\n\
			void main() {\n\
				gl_Position = mvpMat * objMat * vec4(in_Position, 1.f);\n\
				vertex.color = in_Color;\n\
				vert_Normal = mv_Mat * objMat * vec4(in_Normal, 0.0);\n\
				Normal = mat4(transpose(inverse(mvpMat * objMat))) * vert_Normal;\n\
				LightPos = mv_Mat * lightPos;\n\
				FragPos = objMat * vec4(in_Position, 1.0);\n\
				outUvs = uvs;\n\
		}";

	const char* cube_fragShaderBB =
		"#version 330\n\
			in vec4 _Normal;\n\
			in vec4 _FragPos;\n\
			in vec2 _outUvs;\n\
			in vec2 Vertex_UV;\n\
			in vec4 Vertex_Color;\n\
			//in vec3 geomPos;\n\
			out vec4 out_Color;\n\
			uniform mat4 mv_Mat;\n\
			uniform vec4 lightPos;\n\
			uniform vec4 viewPos;\n\
			uniform vec4 lightColor;\n\
			uniform vec4 objectColor;\n\
			uniform sampler2D diffuseTexture;\n\
			void main() {\n\
				vec2 uv = Vertex_UV.xy;\n\
				uv.y *= -1.0;\n\
				////////////////// -Ambient\n\
				float ambientStrength = 0.2f;\n\
				vec4 ambient = ambientStrength * lightColor;\n\
				////////////////// -Diffuse\n\
				vec4 normalizedNormal = normalize(_Normal);\n\
				vec4 lightDir = normalize(lightPos - _FragPos);\n\
				float diffWithoutColor = max(dot(normalizedNormal, lightDir), 0.0f);\n\
				vec4 diffuse = diffWithoutColor * lightColor;\n\
				////////////////// -Specular\n\
				float specularStrength = 1.0f;\n\
				vec4 viewDir = normalize(viewPos - _FragPos);\n\
				vec4 reflectDir = reflect(-lightDir, normalizedNormal);\n\
				float specWithoutColor = pow(max(dot(viewDir, reflectDir), 0.0), 32);\n\
				vec4 specular = specularStrength * specWithoutColor * lightColor;\n\
				////////////////// -Result\n\
				vec4 result = ambient;\n\
				//result += diffuse;\n\
				//result += specular;\n\
				result *= objectColor;\n\
				vec4 textureColor = texture(diffuseTexture, _outUvs);\n\
				//out_Color = result /*+ textureColor*/;\n\
				out_Color = textureColor * Vertex_Color;\n\
		}";

	const char* cube_geomShaderBB =
		"#version 330\n\
		layout (points) in;\n\
		layout(triangle_strip) out;\n\
		layout(max_vertices = 4) out;\n\
\n\
		uniform mat4 mvpMat;\n\
\n\
		uniform float point_size;\n\
\n\
		in Vertex\n\
		{\n\
		  vec4 color;\n\
		} vertex[];\n\
\n\
\n\
		out vec2 Vertex_UV;\n\
		out vec4 Vertex_Color;\n\
\n\
		void main(void)\n\
		{\n\
			vec4 P = gl_in[0].gl_Position;\n\
\n\
			// a: Esquerra-Inferior \n\
			vec2 va = P.xy + vec2(-0.5, -0.5) * point_size;\n\
			gl_Position = mvpMat * vec4(va, P.zw);\n\
			Vertex_UV = vec2(0.0, 0.0);\n\
			Vertex_Color = vertex[0].color;\n\
			EmitVertex();\n\
\n\
			// b: Esquerra-Superior\n\
			vec2 vb = P.xy + vec2(-0.5, 0.5) * point_size;\n\
			gl_Position = mvpMat * vec4(vb, P.zw);\n\
			Vertex_UV = vec2(0.0, 1.0);\n\
			Vertex_Color = vertex[0].color;\n\
			EmitVertex();\n\
\n\
			// d: Dreta-Inferior\n\
			vec2 vd = P.xy + vec2(0.5, -0.5) * point_size;\n\
			gl_Position = mvpMat * vec4(vd, P.zw);\n\
			Vertex_UV = vec2(1.0, 0.0);\n\
			Vertex_Color = vertex[0].color;\n\
			EmitVertex();\n\
\n\
			// c: Dreta-Superior\n\
			vec2 vc = P.xy + vec2(0.5, 0.5) * point_size;\n\
			gl_Position = mvpMat * vec4(vc, P.zw);\n\
			Vertex_UV = vec2(1.0, 1.0);\n\
			Vertex_Color = vertex[0].color; \n\
			EmitVertex();\n\
\n\
			EndPrimitive();\n\
		}";
#pragma endregion

	bool loadOBJ(const char* path,
		std::vector < glm::vec3 >& out_vertices,
		std::vector < glm::vec2 >& out_uvs,
		std::vector < glm::vec3 >& out_normals
	) {
		//Checker for the file being open
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

			//reading of vertex
			if (strcmp(lineHeader, "v") == 0) {
				glm::vec3 vertex;
				fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
				temp_vertices.push_back(vertex);
			}
			//reading for uv
			else if (strcmp(lineHeader, "vt") == 0) {
				glm::vec2 uv;
				fscanf(file, "%f %f\n", &uv.x, &uv.y);
				temp_uvs.push_back(uv);
			}
			//reading for normals
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

	//Setup function for objects
	void setupObject(Type _type,
		const Shader& _shader,
		glm::vec3 _initPos = glm::vec3(0.f, 0.f, 0.5f),
		float _rotation = 0.0f,
		glm::vec3 _rotationAxis = glm::vec3(1.f, 1.f, 1.f),
		glm::vec3 _modelSize = glm::vec3(1.f, 1.f, 1.f),
		glm::vec4 _objectColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f))
	{
		//Model loading depending on the passed parameter
		switch (_type) {
		case Type::CHARACTER:
			available = loadOBJ("resources/dragon.obj.txt", vertices, uvs, normals);
			break;
		case Type::CUBE:
			available = loadOBJ("resources/cube.obj.txt", vertices, uvs, normals);
			break;
		case Type::QUAD:
			available = true;
			vertices.push_back(glm::vec3(0.f, 4.f, 0.f));
			uvs.push_back(glm::vec2(0.5f, 0.5f));
			normals.push_back(glm::vec3(0.f, 0.f, 1.f));

		default:;

		}
		initPos = _initPos;
		modelSize = _modelSize;
		objectColor = _objectColor;
		rotation = _rotation;
		rotationAxis = _rotationAxis;

		if (available) {

			glGenVertexArrays(1, &objectVao);
			glBindVertexArray(objectVao);
			glGenBuffers(3, objectVbo);


			glBindBuffer(GL_ARRAY_BUFFER, objectVbo[0]);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
			glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
			glEnableVertexAttribArray(0);

			glBindBuffer(GL_ARRAY_BUFFER, objectVbo[1]);
			glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
			glVertexAttribPointer((GLuint)1, 3, GL_FLOAT, GL_FALSE, 0, 0);
			glEnableVertexAttribArray(1);

			glBindBuffer(GL_ARRAY_BUFFER, objectVbo[2]);
			glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);
			glVertexAttribPointer((GLuint)2, 2, GL_FLOAT, GL_FALSE, 0, 0);
			glEnableVertexAttribArray(2);


			glBindVertexArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

			shader = _shader;
		}
	}

	void cleanObject() {
		glDeleteBuffers(3, objectVbo);
		glDeleteVertexArrays(1, &objectVao);

		shader.Delete();

	}

	//void updateObject(const glm::mat4& transform) {
	//	if (available && enabled) {
	//		glUseProgram(objectProgram);
	//		objMat = transform;
	//		glUseProgram(0);
	//	}
	//}

	//Object drawing function
	void drawObject(Type _type) {
		if (available && enabled) {
			glBindVertexArray(objectVao);

			shader.UseProgram();
			shader.UseTexture();

			objMat = glm::translate(glm::mat4(), initPos) * glm::rotate(glm::mat4(), rotation, rotationAxis) * glm::scale(glm::mat4(), modelSize);
			shader.SetUniformsMats(objMat);
			shader.SetUniformsLights(objectColor);

			if (_type == Type::CHARACTER)
				glUniform1f(glGetUniformLocation(shader.GetProgram(), "deltaTime"), deltaTime);
			else
				glUniform1f(glGetUniformLocation(shader.GetProgram(), "deltaTime"), 0);

			if (_type == Type::CHARACTER || _type == Type::CUBE)
				glDrawArrays(GL_TRIANGLES, 0, vertices.size() * 3);
			else if (_type == Type::QUAD)
				glDrawArrays(GL_POINTS, 0, 1);

			glUseProgram(0);
			glBindVertexArray(0);
		}
	}
	//Object drawing function with position & light color to update them at GLrender()
	void drawObject(Type _type, glm::vec3 currentPos, glm::vec4 _lightColor) {
		if (available && enabled) {
			glBindVertexArray(objectVao);

			shader.UseProgram();
			shader.UseTexture();

			objMat = glm::translate(glm::mat4(), currentPos) * glm::rotate(glm::mat4(), rotation, rotationAxis) * glm::scale(glm::mat4(), modelSize);
			shader.SetUniformsMats(objMat);
			shader.SetUniformsLights(objectColor, _lightColor);

			if(_type == Type::CHARACTER)
				glUniform1f(glGetUniformLocation(shader.GetProgram(), "deltaTime"), deltaTime);
			else
				glUniform1f(glGetUniformLocation(shader.GetProgram(), "deltaTime"), 0);

			if (_type == Type::CHARACTER || _type == Type::CUBE)
				glDrawArrays(GL_TRIANGLES, 0, vertices.size() * 3);
			else if (_type == Type::QUAD)
				glDrawArrays(GL_POINTS, 0, 1);

			glUseProgram(0);
			glBindVertexArray(0);
		}
	}
};
#pragma endregion

/////////////////////////////////////////////////

GLuint program;
GLuint VAO;
GLuint VBO;

//Objects declaration
Object babyCharacter;
Object brotherCharacter;
Object sisterCharacter;
Object mommyCharacter;
Object daddyCharacter;
Object ground;
Object light;
Object Quad;

void GLinit(int width, int height) {
	glViewport(0, 0, width, height);
	glClearColor(0.2f, 0.2f, 0.2f, 1.f);
	glClearDepth(1.f);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	startTime = actualTime = clock();
	deltaTime = 0;

	RV::_projection = glm::perspective(RV::FOV, (float)width / (float)height, RV::zNear, RV::zFar);

	// Setup shaders & geometry
	//Axis::setupAxis();
	//Cube::setupCube();

	//Init Shaders
	Shader phongShader = Shader("shaders/phongVertexShader.txt", "shaders/phongFragmentShader.txt", "shaders/phongGeometryShader.txt");
	phongShader.AddTextureID("resources/grassTexture.png");

	Shader staticPhongShader = Shader("shaders/phongVertexShader.txt", "shaders/phongFragmentShader.txt", "shaders/phongGeometryShader.txt");
	staticPhongShader.AddTextureID("resources/grassTexture.png");

	Shader BBShader = Shader("shaders/BBVertexShader.txt", "shaders/BBFragmentShader.txt", "shaders/BBGeometryShader.txt");
	BBShader.AddTextureID("resources/Sun.png");

	//Objects inicialization
	babyCharacter.setupObject(Object::Type::CHARACTER, phongShader, glm::vec3(0.0f, 0.0f, 0.0f), 0.f, glm::vec3(1.f, 1.f, 1.f), glm::vec3(0.2f, 0.2f, 0.2f), glm::vec4(0.7f, 0.2f, 0.95f, 0.0f));
	brotherCharacter.setupObject(Object::Type::CHARACTER, phongShader, glm::vec3(-7.0f, 0.0f, -20.0f), glm::radians(20.f), glm::vec3(0.f, 1.f, 0.f), glm::vec3(0.3f, 0.3f, 0.3f), glm::vec4(0.4f, 0.2f, 0.65f, 0.0f));
	sisterCharacter.setupObject(Object::Type::CHARACTER, phongShader, glm::vec3(7.0f, 0.0f, -20.0f), glm::radians(-20.f), glm::vec3(0.f, 1.f, 0.f), glm::vec3(0.3f, 0.3f, 0.3f), glm::vec4(0.65f, 0.2f, 0.45f, 0.0f));
	mommyCharacter.setupObject(Object::Type::CHARACTER, phongShader, glm::vec3(-20.0f, 0.0f, -20.0f), glm::radians(40.f), glm::vec3(0.f, 1.f, 0.f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec4(0.0f, 0.0f, 0.7f, 0.0f));
	daddyCharacter.setupObject(Object::Type::CHARACTER, phongShader, glm::vec3(20.0f, 0.0f, -20.0f), glm::radians(-40.f), glm::vec3(0.f, 1.f, 0.f), glm::vec3(0.9f, 0.9f, 0.9f), glm::vec4(0.7f, 0.0f, 0.0f, 0.0f));
	
	ground.setupObject(Object::Type::CUBE, staticPhongShader, glm::vec3(0.0f, -1.0f, 0.0f), 0.f, glm::vec3(1.f, 1.f, 1.f), glm::vec3(100.0f, 1.0f, 100.0f), glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
	light.setupObject(Object::Type::CUBE, staticPhongShader, glm::vec3(Light::lightPosition.x, Light::lightPosition.y, Light::lightPosition.z), 0.f, glm::vec3(1.f, 1.f, 1.f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec4(Light::lightColor.r, Light::lightColor.g, Light::lightColor.b, 1.0f));
		
	Quad.setupObject(Object::Type::QUAD, BBShader);
	/////////////////////////////////////////////////////TODO
	GLuint vertex_shader;
	GLuint fragment_shader;

#pragma region Commented
	
	////Compile the shaders
	//vertex_shader =
	//	glCreateShader(GL_VERTEX_SHADER);
	//glShaderSource(vertex_shader, 1,
	//	vertex_shader_source, NULL);
	//glCompileShader(vertex_shader);

	//fragment_shader =
	//	glCreateShader(GL_FRAGMENT_SHADER);
	//glShaderSource(fragment_shader, 1,
	//	fragment_shader_source, NULL);
	//glCompileShader(fragment_shader);

	////Create the program from the shaders
	//program = glCreateProgram();
	//glAttachShader(program, vertex_shader);
	//glAttachShader(program, fragment_shader);
	//glLinkProgram(program);

	////Delete the shaders
	//glDeleteShader(vertex_shader);
	//glDeleteShader(fragment_shader);

	//glGenVertexArrays(1, &VAO);
	//glBindVertexArray(VAO);

	//glGenBuffers(1, &VBO);

	//glBindBuffer(GL_ARRAY_BUFFER, VBO);
	//glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//glVertexAttribPointer(
	//	0,
	//	3,
	//	GL_FLOAT,
	//	GL_FALSE,
	//	3 * sizeof(float),
	//	(void*)0);

	//glEnableVertexAttribArray(0);

	//glBindVertexArray(0);
#pragma endregion

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
bool dollyEffectActive = true;
void InverseDollyEffect()
{
	//Function to change the Field of View depending on the distance to the focus point & to the width of the scene
	float width = 16.f;
	RV::FOV = 2.f * glm::atan(0.5f * width / glm::abs(RV::panv[2] + 0.5f));
}

void GLrender(float dt) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	actualTime = clock();
	//deltaTime += (actualTime - startTime) / 10000;
	startTime = actualTime;

	//Dolly effect
	if (dollyEffectActive)
		InverseDollyEffect();
	else
		RV::FOV = glm::radians(65.f);
	//Update of the perspective view for the dolly effect
	RV::_projection = glm::perspective(RV::FOV, (float)800 / (float)600, RV::zNear, RV::zFar);

	//Setting of the camera
	RV::_modelView = glm::mat4(1.f);
	RV::_modelView = glm::translate(RV::_modelView, glm::vec3(RV::panv[0], RV::panv[1], RV::panv[2]));
	RV::_modelView = glm::rotate(RV::_modelView, RV::rota[1], glm::vec3(1.f, 0.f, 0.f));
	RV::_modelView = glm::rotate(RV::_modelView, RV::rota[0], glm::vec3(0.f, 1.f, 0.f));

	RV::_MVP = RV::_projection * RV::_modelView;

	//Drawing of the scene objects
	//babyCharacter.drawObject(Object::Type::CHARACTER);
	//brotherCharacter.drawObject(Object::Type::CHARACTER);
	//sisterCharacter.drawObject(Object::Type::CHARACTER);
	mommyCharacter.drawObject(Object::Type::CHARACTER);
	daddyCharacter.drawObject(Object::Type::CHARACTER);
	//ground.drawObject(Object::Type::CUBE);
	//light.drawObject(Object::Type::CUBE, glm::vec3(Light::lightPosition.x, Light::lightPosition.y, Light::lightPosition.z), glm::vec4(Light::lightColor.r, Light::lightColor.g, Light::lightColor.b, 1.0f));
	Quad.drawObject(Object::Type::QUAD);
	ImGui::Render();
}

void GUI() {
	bool show = true;
	ImGui::Begin("Physics Parameters", &show, 0);
	//
	{
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		/////////////////////////////////////////////////////TODO
		//Dolly effect checkbox & slider
		ImGui::Checkbox("Dolly Effect", &dollyEffectActive);
		ImGui::SliderFloat("Dolly Effect Slider", &RV::panv[2], -16.f, -6.f);
		//Light position slider modifiers
		ImGui::SliderFloat("Light X Position", &Light::lightPosition.x, -20.f, 20.f);
		ImGui::SliderFloat("Light Y Position", &Light::lightPosition.y, -20.f, 20.f);
		ImGui::SliderFloat("Light Z Position", &Light::lightPosition.z, -20.f, 20.f);
		//Light color slider modifiers
		ImGui::SliderFloat("Light R Color", &Light::lightColor.r, 0.f, 1.f);
		ImGui::SliderFloat("Light G Color", &Light::lightColor.g, 0.f, 1.f);
		ImGui::SliderFloat("Light B Color", &Light::lightColor.b, 0.f, 1.f);
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