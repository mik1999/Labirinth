#pragma once

#include <iostream>
#include <vector>
#include "utils.h"
#include "Camera.hpp"
#include "Texture.hpp"

class Mesh
{
public:
	Mesh();
	Mesh(const Mesh&) = delete;
	~Mesh();
	std::pair<int, int> texSize() const; 
	void draw(std::shared_ptr<ShaderProgram> _shader);
	void loadFromFile(std::string filename);
	void updateTriangles(Triangles&& t);
	void clearTriangles();
	void addTriangles(Triangles&& t);
	void attachTexture(TexturePtr texture, float enlarge = 1);
	void attachNMap(std::string filename);
	void setMaterialInfo(glm::vec3 Ka, glm::vec3 Kd, glm::vec3 Ks, float shininess);
private:
	void _initSampler();
	void _addToFloatData(Triangles&& t);

	glm::vec3 _Ka, _Kd, _Ks;
	float _shininess;
	GLuint _sampler = 0;
	int _vcount = 0;
	GLuint _vao = 0;
	GLuint _vbo;
	std::vector<float> _float_data;
	TexturePtr _texture, _nmap;
	bool _with_texture = false, _with_nmap = false;
	float _enlarge = 1;
};

void attachTextureFromFile(const std::string& filename, float enlarge, Mesh& mesh);

void join(std::vector<float> &float_data, glm::vec2 v);
void join(std::vector<float>& float_data, glm::vec3 v);

class MeshCollection {
public:
	MeshCollection() = default;
	void clear();
	Mesh& index(size_t i);
	void init(size_t size);
	void draw(std::shared_ptr<ShaderProgram> shader);
private:
	std::vector<std::shared_ptr<Mesh> > _meshes;
};