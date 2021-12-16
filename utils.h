#pragma once

// Общий заголовочный файл 
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "glm/gtx/intersect.hpp"
#include "glm/gtx/transform.hpp"
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw_gl3.h"
#include "ShaderProgram.hpp"
#include <memory>
#include <fstream>
#include <iostream>
#include <vector>
#include <exception>
#include <utility>

struct Triangle {
	glm::vec3 a, b, c;
	glm::vec2 ta, tb, tc;
};

enum MeshTypes {
	Floor,
	Walls,
	Ceiling,
	HonewDrop,
	Portal1,
	Portal2,
	Posters
};

enum PortalTypes {
	Blue,
	LightBlue
};

using Triangles = std::vector<Triangle>;

#define NEAR 0.03f
#define PORTAL_RADIUS 0.35f
#define PORTAL_WIDTH 0.08f
#define MESH_NUMBER 6 + POSTER_NUMBER
#define POSTER_NUMBER 5
#define DROP_SCALE 0.33

#define DIR "7910LyamkinData3/"

#define UnBindPoint 0  // привязываем к началу (у нас всего один буфер)

GLuint getUniformBuffer(GLuint program); // создает буфер для юниформ переменных

void initGLFW();
void initGLEW();

void closeAll();

glm::vec3 normal(glm::vec3 a, glm::vec3 b, glm::vec3 c); // правая нормаль к треугольнику
glm::mat2 rotate(float alpha);
bool TriangleSegmentIntersect(const glm::vec3& a1, const glm::vec3& a2, const glm::vec3& a3, const glm::vec3& b1, const glm::vec3& b2);

float distanceToTriangle(glm::vec3 o, Triangle t); // расстояние от точки до треугольника

glm::vec3 project(const glm::vec3& v, const glm::mat4x4& m);
glm::mat4 projectmatrix5to5(const glm::vec4& x1, const glm::vec4& x2, const glm::vec4& x3, const glm::vec4& x4, const glm::vec4& x5, const glm::vec4& y1, const glm::vec4& y2, const glm::vec4& y3, const glm::vec4& y4, const glm::vec4& y5);

Triangles loadFromFile(std::string filename);

void joinTriangles(Triangles& t1, Triangles&& t2);

float matNorm(const glm::mat4& mat);

void printMatrix(const glm::mat4& matrix);

void projectTriangles(Triangles& t, glm::mat4x4 transform);

namespace Light {
	const glm::vec3 La_free = glm::vec3(0.2, 0.2, 0.2), Ld_free = glm::vec3(0.8, 0.8, 0.8), Ls_free = glm::vec3(1, 1, 1);
	const glm::vec3 La_orbit = glm::vec3(0.5, 0.5, 0.5), Ld_orbit = glm::vec3(0.9, 0.9, 0.9), Ls_orbit = glm::vec3(1, 1, 1);
	const glm::vec3 light_dir = glm::vec3(0.4, 0.4, -1);
	void setLightInfo(std::shared_ptr<ShaderProgram> shader, const glm::vec3& camera_pos, bool free_camera);
}