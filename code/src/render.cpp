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
#define PI 3.141592653589793238462643383279502884L

int x, y, n;
unsigned char* data;

///////// fw decl
namespace ImGui {
	void Render();
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

	int width;
	int height;

	struct prevMouse {
		float lastx, lasty;
		MouseEvent::Button button = MouseEvent::Button::None;
		bool waspressed = false;
	} prevMouse;

	float panv[3] = { 0.f, -5.f, -16.f };
	float rota[2] = { 0.f, 0.f };
}
namespace RV = RenderVars;

void GLResize(int width, int height) {
	glViewport(0, 0, width, height);

	RV::width = width;
	RV::height = height;

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

float alpha = 0.75f;

#pragma region ShaderRegion

namespace Texture {
	std::vector<unsigned int> textureIDs;

	bool FindTexture(unsigned int IDToFind) {
		for (int i = 0; i < textureIDs.size(); i++)
		{
			if (textureIDs[i] == IDToFind)	return true;
		}
		
		return false;
	}

	bool FindTexture(unsigned int IDToFind, std::vector<unsigned int> vectorToSearch) {
		for (int i = 0; i < vectorToSearch.size(); i++)
		{
			if (vectorToSearch[i] == IDToFind)	return true;
		}

		return false;
	}

	unsigned int AddTextureID(const char* texturePath) {
		unsigned int tmpTextureID;

		data = stbi_load(texturePath, &x, &y, &n, 4);
		glGenTextures(1, &tmpTextureID); // Create the handle of the texture
		
		if (!FindTexture(tmpTextureID))
		{
			glBindTexture(GL_TEXTURE_2D, tmpTextureID); //Bind it

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data); //Load the data
			stbi_image_free(data);

			textureIDs.push_back(tmpTextureID);
		}
		else
		{
			//do nothing
		}
		
		return tmpTextureID;
	}
	unsigned int AddTextureID(GLuint texture) {
		if (!FindTexture(texture))
		{
			glBindTexture(GL_TEXTURE_2D, texture);
			textureIDs.push_back(texture);
		}
		return texture;
	}
	

}

class Shader {
private:
	GLuint program;
	GLuint programWithTexture;
	GLuint* shaders;
	int shadersSize = 0;
	std::vector<unsigned int> textureIDs;

	GLuint compileShaderFromFile(const char* shaderPath, GLenum shaderType, const char* name = "") {
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

		glDeleteShader(shaders[0]);
		glDeleteShader(shaders[2]);
		glDeleteShader(shaders[1]);
	}
	Shader(const Shader& s) {
		textureIDs = s.textureIDs;

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

	void AddSkyboxTextureID(std::vector<const char*> texturePath) {
		unsigned int tmpTextureID;
		glGenTextures(1, &tmpTextureID); // Create the handle of the texture
		if (!Texture::FindTexture(tmpTextureID))
			glBindTexture(GL_TEXTURE_CUBE_MAP, tmpTextureID); //Bind it

		for (unsigned int i = 0; i < texturePath.size(); i++)
		{
			data = stbi_load(texturePath[i], &x, &y, &n, 4);
			if (data == NULL) {
				fprintf(stderr, "Error loading image %s", texturePath[i]);
				exit(1);
			}
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		textureIDs.push_back(tmpTextureID);

	}

	void AddTextureID(const char* texturePath) {
		unsigned int newTextureID = Texture::AddTextureID(texturePath);
		if (!Texture::FindTexture(newTextureID, textureIDs))
		{
			textureIDs.push_back(newTextureID);
		}
	}

	void AddFBTextureID(unsigned int _textureID) {
		if (!Texture::FindTexture(_textureID, textureIDs))
		{
			Texture::AddTextureID(_textureID);
			textureIDs.push_back(_textureID);
		}
	}

	void SetTextureID(unsigned int _textureID) {
		if (!Texture::FindTexture(_textureID, textureIDs))
		{
			textureIDs.push_back(_textureID);
		}
	}

	void UseProgram() {
		glUseProgram(program);
	}

	void UseTexture() {
		for (int i = 0; i < textureIDs.size() && i < 10; i++)
		{
			int tmpIt = GL_TEXTURE0 + i;

			glActiveTexture(tmpIt);
			glBindTexture(GL_TEXTURE_2D, textureIDs[i]);
			glUniform1i(glGetUniformLocation(program, "diffuseTexture"), i);
		}
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

		for (int i = 0; i < textureIDs.size(); i++)
		{
			glDeleteTextures(1, &textureIDs[i]);
		}
		textureIDs.clear();
	}

	//setters for uniforms
	void SetUniformsMats(const glm::mat4& objMat) {
		glUniformMatrix4fv(glGetUniformLocation(program, "objMat"), 1, GL_FALSE, glm::value_ptr(objMat));
		glUniformMatrix4fv(glGetUniformLocation(program, "mv_Mat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_modelView));
		glUniformMatrix4fv(glGetUniformLocation(program, "mvpMat"), 1, GL_FALSE, glm::value_ptr(RenderVars::_MVP));
	}
	void SetUniformsMats(const glm::mat4* objMat) {
		for (int i = 0; i < 10; i++)
		{
			char ichar = i + 48;
			glUniformMatrix4fv(glGetUniformLocation(program, "objMat[" + ichar + ']'), 1, GL_FALSE, glm::value_ptr(objMat[i]));
		}
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
		glUniform1f(glGetUniformLocation(program, "alpha"), alpha);

	}

};

#pragma endregion

/////////////////////////////////////////////////
#pragma region Object
class Object {
public:
	//Enum class for diferent object models
	static enum class Type { CHARACTER, CUBE, QUAD, SKYBOX, REARVIEW_MIRROR, COUNT };

	std::vector< unsigned int > vertexIndices, uvIndices, normalIndices;
	std::vector< glm::vec3 > temp_vertices;
	std::vector< glm::vec2 > temp_uvs;
	std::vector< glm::vec3 > temp_normals;

	std::vector< glm::vec3 > vertices;
	std::vector< glm::vec2 > uvs;
	std::vector< glm::vec3 > normals;

	Shader shader;

	GLuint objectVao;
	GLuint objectVbo[3];
	GLuint fbo, fbo_tex;

	//Setup Variables
	bool available = false;
	bool enabled = true;
	glm::mat4 objMat = glm::mat4(1.f);
	glm::mat4* objMatArray = new glm::mat4 [10];
	glm::vec3 initPos;
	float rotation;
	glm::vec3 rotationAxis;
	glm::vec3 modelSize;
	glm::vec4 objectColor;
	glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f);
	bool turnLeft, turnRight;

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
		// Vertex
		for (unsigned int i = 0; i < vertexIndices.size(); i++) {
			unsigned int vertexIndex = vertexIndices[i];
			glm::vec3 vertex = temp_vertices[vertexIndex - 1];
			out_vertices.push_back(vertex);
		}
		// UV
		for (unsigned int i = 0; i < uvIndices.size(); i++) {
			unsigned int uvsIndex = uvIndices[i];
			glm::vec2 uv = temp_uvs[uvsIndex - 1];
			out_uvs.push_back(uv);
		}
		// Normal
		for (unsigned int i = 0; i < normalIndices.size(); i++) {
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
			available = loadOBJ("resources/Camaro.obj.txt", vertices, uvs, normals);
			break;
		case Type::CUBE:
			available = loadOBJ("resources/cube.obj.txt", vertices, uvs, normals);
			break;
		case Type::QUAD:
			available = true;
			vertices.push_back(glm::vec3(0.f, 5.f, -10.f));
			uvs.push_back(glm::vec2(0.5f, 0.5f));
			normals.push_back(glm::vec3(0.f, 0.f, 1.f));
			break;
		case Type::SKYBOX:
			available = true;
			vertices.push_back(glm::vec3(0.f, 0.f, 0.f));
			break;
		case Type::REARVIEW_MIRROR:
			available = loadOBJ("resources/cube.obj.txt", vertices, uvs, normals);
		default:;

		}
		initPos = _initPos;
		modelSize = _modelSize;
		objectColor = _objectColor;
		rotation = _rotation;
		rotationAxis = _rotationAxis;

		if (available && _type != Type::SKYBOX /*&& _type != Type::QUAD*/) {

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
		else if (available && _type == Type::SKYBOX)
		{
			glGenVertexArrays(1, &objectVao);
			glBindVertexArray(objectVao);
			glGenBuffers(1, objectVbo);


			glBindBuffer(GL_ARRAY_BUFFER, objectVbo[0]);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
			glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

			glEnableVertexAttribArray(0);

			shader = _shader;
		}
	}

	void cleanObject() {
		glDeleteBuffers(3, objectVbo);
		glDeleteVertexArrays(1, &objectVao);

		shader.Delete();

	}

	void setupFBO() {
		glGenFramebuffers(1, &fbo);

		glGenTextures(1, &fbo_tex);
		glBindTexture(GL_TEXTURE_2D, fbo_tex);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 800, 800, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_tex, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		GLuint rbo;

		glGenBuffers(1, &rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 800, 800);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindTexture(GL_RENDERBUFFER, 0);
	}

	//Object drawing function
	void drawObject(Type _type) {
		if (available && enabled) {
			glBindVertexArray(objectVao);


			shader.UseProgram();
			shader.UseTexture();

			if (_type == Type::CHARACTER || _type == Type::REARVIEW_MIRROR)
				objMat = glm::translate(glm::mat4(), pos) * glm::rotate(glm::mat4(), rotation, rotationAxis) * glm::scale(glm::mat4(), modelSize);
			else
				objMat = glm::translate(glm::mat4(), initPos) * glm::rotate(glm::mat4(), rotation, rotationAxis) * glm::scale(glm::mat4(), modelSize);
			
			shader.SetUniformsMats(objMat);
			shader.SetUniformsLights(objectColor);

			if (_type == Type::CHARACTER || _type == Type::CUBE)
				glDrawArrays(GL_TRIANGLES, 0, vertices.size() * 3);
			else if (_type == Type::SKYBOX)
				glDrawArrays(GL_POINTS, 0, 1);
			else if (_type == Type::QUAD)
				glDrawArrays(GL_POINTS, 0, 1);
			else if (_type == Type::REARVIEW_MIRROR)
			{
				glBindTexture(GL_TEXTURE_2D, fbo_tex);
				glDrawArrays(GL_TRIANGLES, 0, vertices.size() * 3);
				glBindTexture(GL_TEXTURE_2D, 0);
			}

			glUseProgram(0);
			glBindVertexArray(0);
		}
	}
	//Object drawing function with position & light color to update them at GLrender()
	void drawObject(Type _type, glm::vec3 currentPos, float rot, glm::vec3 rotAxis) {
		if (available && enabled) {
			glBindVertexArray(objectVao);

			shader.UseProgram();
			shader.UseTexture();

			objMat = glm::translate(glm::mat4(), currentPos) * glm::rotate(glm::mat4(), rot, rotAxis) * glm::scale(glm::mat4(), modelSize);
			
			shader.SetUniformsMats(objMat);
			shader.SetUniformsLights(objectColor);

			if (_type == Type::CHARACTER || _type == Type::CUBE)
				glDrawArrays(GL_TRIANGLES, 0, vertices.size() * 3);
			else if (_type == Type::QUAD)
				glDrawArrays(GL_POINTS, 0, 1);
			else if (_type == Type::REARVIEW_MIRROR)
				glDrawArrays(GL_TRIANGLES, 0, vertices.size() * 3);

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
Object carCharacter;

Object ground;
Object light;
Object Tree1;
Object Tree2;
Object Tree3;
Object Tree4;
Object skybox;
Object RearViewMirror;

int camera = 0;

void drawFrameBufferObjectTexture() {
	//Store current matrix values to reset to them later
	glm::mat4 temp_mvp = RenderVars::_MVP;
	glm::mat4 temp_mv = RenderVars::_modelView;

	//Set up framebuffer and draw objects to it
	glBindFramebuffer(GL_FRAMEBUFFER, RearViewMirror.fbo);
	glClearColor(1.f, 1.f, 1.f, 1.f);
	glViewport(0, 0, 800, 800);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	RenderVars::_MVP = RenderVars::_projection;
	RenderVars::_modelView = glm::mat4(1.f);

	RenderVars::_modelView = glm::translate(RenderVars::_modelView, glm::vec3(0, RearViewMirror.pos.y, RearViewMirror.pos.z));
	RenderVars::_modelView = glm::rotate(RenderVars::_modelView, glm::radians(180.f), glm::vec3(0, 1, 0));

	//draw objects here
	ground.drawObject(Object::Type::CUBE);

	//restore values
	RenderVars::_MVP = temp_mvp;
	RenderVars::_modelView = temp_mv;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glViewport(0, 0, RenderVars::width, RenderVars::height);
	glBindTexture(GL_TEXTURE_2D, RearViewMirror.fbo_tex);

	RearViewMirror.drawObject(Object::Type::REARVIEW_MIRROR);
}

void GLinit(int width, int height) {
	glViewport(0, 0, width, height);
	glClearColor(0.2f, 0.2f, 0.2f, 1.f);
	glClearDepth(1.f);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	RV::width = width;
	RV::height = height;

	RV::_projection = glm::perspective(RV::FOV, (float)width / (float)height, RV::zNear, RV::zFar);

	std::vector<const char*> skyboxTextureSides = {
			"resources/Skybox/Right.png",
			"resources/Skybox/Left.png",
			"resources/Skybox/Ceiling.png",
			"resources/Skybox/Floor.png",
			"resources/Skybox/Front.png",
			"resources/Skybox/Back.png",
	};

	//Init Shaders
	Shader phongShader = Shader("shaders/phongVertexShader.vs", "shaders/phongFragmentShader.fs", "shaders/phongGeometryShader.gs");
	phongShader.AddTextureID("resources/Camaro/Camaro_AlbedoTransparency_alt.png");

	Shader staticPhongShader = Shader("shaders/phongVertexShader.vs", "shaders/phongFragmentShader.fs", "shaders/phongGeometryShader.gs");
	staticPhongShader.AddTextureID("resources/grassTexture.png");

	Shader BBShader = Shader("shaders/BBVertexShader.vs", "shaders/BBFragmentShader.fs", "shaders/BBGeometryShader.gs");
	BBShader.AddTextureID("resources/trevenant.png");

	Shader skyboxShader = Shader("shaders/SkyboxVertexShader.vs", "shaders/SkyboxFragmentShader.fs", "shaders/SkyboxGeometryShader.gs");
	skyboxShader.AddSkyboxTextureID(skyboxTextureSides);

	Shader RVMirrorShader = Shader("shaders/phongVertexShader.vs", "shaders/phongFragmentShader.fs", "shaders/phongGeometryShader.gs");
	RVMirrorShader.AddFBTextureID(RearViewMirror.fbo_tex);

	//Objects inicialization
	skybox.setupObject(Object::Type::SKYBOX, skyboxShader);
	carCharacter.setupObject(Object::Type::CHARACTER, phongShader, glm::vec3(0.f, 0.f, 0.f), 0.f, glm::vec3(1.f, 1.f, 1.f), glm::vec3(0.02f, 0.02f, 0.02f), glm::vec4(0.7f, 0.2f, 0.95f, 0.0f));

	ground.setupObject(Object::Type::CUBE, staticPhongShader, glm::vec3(0.0f, -1.0f, 0.0f), 0.f, glm::vec3(1.f, 1.f, 1.f), glm::vec3(100.0f, 1.0f, 100.0f), glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
	light.setupObject(Object::Type::CUBE, staticPhongShader, glm::vec3(Light::lightPosition.x, Light::lightPosition.y, Light::lightPosition.z), 0.f, glm::vec3(1.f, 1.f, 1.f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec4(Light::lightColor.r, Light::lightColor.g, Light::lightColor.b, 1.0f));
		
	Tree1.setupObject(Object::Type::QUAD, BBShader);
	Tree2.setupObject(Object::Type::QUAD, BBShader);
	Tree3.setupObject(Object::Type::QUAD, BBShader);
	Tree4.setupObject(Object::Type::QUAD, BBShader);

	RearViewMirror.setupFBO();
	RearViewMirror.setupObject(Object::Type::REARVIEW_MIRROR, RVMirrorShader,
		glm::vec3(carCharacter.pos.x, carCharacter.pos.y, carCharacter.pos.z),
		0.f, glm::vec3(1.f, 1.f, 1.f), glm::vec3(0.4, 0.2f, 0.0f));

}

void GLcleanup() 
{
}

void GLrender(float dt) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (camera == 0)
	{
		//Setting of the camera
		RV::_modelView = glm::mat4(1.f);
		RV::_modelView = glm::translate(RV::_modelView, glm::vec3(RV::panv[0], RV::panv[1], RV::panv[2]));
		RV::_modelView = glm::rotate(RV::_modelView, RV::rota[1], glm::vec3(1.f, 0.f, 0.f));
		RV::_modelView = glm::rotate(RV::_modelView, RV::rota[0], glm::vec3(0.f, 1.f, 0.f));

		RV::_MVP = RV::_projection * RV::_modelView;
		drawFrameBufferObjectTexture();
	}
	else if (camera == 1)
	{
		RV::_modelView = glm::mat4(1.f);

		RV::_modelView = glm::rotate(RV::_modelView, -(float)PI, glm::vec3(0.f, 1.0f, 0.f));
		RV::_modelView = glm::rotate(RV::_modelView, -carCharacter.rotation, glm::vec3(0.f, 1.0f, 0.f));
		RV::_modelView = glm::translate(RV::_modelView,	-glm::vec3(carCharacter.pos.x, carCharacter.pos.y + 1.5f, carCharacter.pos.z));

		RV::_MVP = RV::_projection * RV::_modelView;
		drawFrameBufferObjectTexture();
	}

	carCharacter.pos.z += glm::cos(carCharacter.rotation);
	carCharacter.pos.x += glm::sin(carCharacter.rotation);

	carCharacter.rotationAxis = glm::vec3(0, 1, 0);

	if (carCharacter.pos.z >= 10.f)
	{
		carCharacter.turnLeft = true;
	}
	else if (carCharacter.rotation >= PI * 1.5f)
	{
		carCharacter.turnLeft = false;
	}
	if (carCharacter.pos.x <= -17.f)
	{
		carCharacter.turnRight = true;
	}
	else if (carCharacter.rotation <= 0.f)
	{
		carCharacter.turnRight = false;
	}

	if (carCharacter.turnLeft)
	{
		carCharacter.rotation += 0.05f;
	}
	else if (carCharacter.turnRight)
	{
		carCharacter.rotation -= 0.05f;
	}

	RearViewMirror.pos = glm::vec3(carCharacter.pos.x, carCharacter.pos.y, carCharacter.pos.z);
	RearViewMirror.rotationAxis = glm::vec3(0, 1, 0);
	RearViewMirror.rotation = carCharacter.rotation;
	RearViewMirror.pos += glm::vec3(0, 1.8f, 0.6f);

	//Drawing of the scene objects
	glEnable(GL_STENCIL_TEST);
	glClear(GL_STENCIL_BUFFER_BIT);
	glStencilFunc(GL_ALWAYS, 1, 0xFF); // Set any stencil to 1
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glStencilMask(0x00); // Write to stencil buffer
	skybox.drawObject(Object::Type::SKYBOX);
	ground.drawObject(Object::Type::CUBE);

	RearViewMirror.drawObject(Object::Type::REARVIEW_MIRROR);

	glDepthMask(GL_TRUE); // Write to depth buffer
	glStencilFunc(GL_ALWAYS, 1, 0xFF); // Make all fragments pass sten
	glStencilMask(0xFF);
	glDisable(GL_CULL_FACE);
	carCharacter.drawObject(Object::Type::CHARACTER);
	glEnable(GL_CULL_FACE);

	glDisable(GL_BLEND);
	Tree1.drawObject(Object::Type::QUAD, glm::vec3(10.f, 5.f, -10.f), 0, glm::vec3(1.f, 1.f, 1.f));
	Tree2.drawObject(Object::Type::QUAD, glm::vec3(-10.f, 5.f, -10.f), 0, glm::vec3(1.f, 1.f, 1.f));
	Tree3.drawObject(Object::Type::QUAD, glm::vec3(-10.f, 5.f, 10.f), 0, glm::vec3(1.f, 1.f, 1.f));
	Tree4.drawObject(Object::Type::QUAD, glm::vec3(10.f, 5.f, 10.f), 0, glm::vec3(1.f, 1.f, 1.f));
	glEnable(GL_BLEND);


	glDisable(GL_STENCIL_TEST);

	ImGui::Render();
}

void GUI() {
	bool show = true;
	ImGui::Begin("Physics Parameters", &show, 0);
	//
	{
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		if (ImGui::Button("Default Cam")) {
			camera = 0;
			RV::FOV = glm::radians(65.f);
		}	if (ImGui::Button("Car Cam")) {
			camera = 1;
			RV::FOV = glm::radians(30.f);
		}
		ImGui::SliderFloat("Windows transparency", &alpha, 0.f, 1.f);
	}

	ImGui::End();

}