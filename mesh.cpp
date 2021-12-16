#include "mesh.h"

Mesh::Mesh(){
	glGenBuffers(1, &_vbo); // создаем буфер для хранения вершин
	glGenVertexArrays(1, &_vao); // создаем объект VertexArrayObject для хранения настроек полигональной модели
	glGenSamplers(1, &_sampler); // создаем сэмплер
	_initSampler();
	// Установим настройки атрибутов
	glBindVertexArray(_vao);
	glBindBuffer(GL_ARRAY_BUFFER, _vbo);
	
	glEnableVertexAttribArray(0); // включаем 0-ой вершинный атрибут -- координаты
	glEnableVertexAttribArray(1); // включаем 1-ый вершинный атрибут -- нормаль
	glEnableVertexAttribArray(2); // включаем 2-ой вершинный атрибут -- нормаль

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(6 * sizeof(float)));
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	_Ka = glm::vec3(0.4, 0.4, 0), _Kd = glm::vec3(0.7, 0.7, 0), _Ks = glm::vec3(0.4, 0.4, 0.4);
	_shininess = 100;
}

Mesh::~Mesh()
{
	glDeleteBuffers(1, &_vbo);
	glDeleteVertexArrays(1, &_vao);
	glDeleteSamplers(1, &_sampler);
}

std::pair<int, int> Mesh::texSize() const
{
	int w, h;
	_texture.get()->getSize(w, h);
	return std::pair<int, int>(w, h);
}

void attachTextureFromFile(const std::string& filename, float enlarge, Mesh& mesh)
{
	TexturePtr texture = loadTexture(filename);
	mesh.attachTexture(texture, enlarge);
}

void join(std::vector<float>& float_data, glm::vec2 v) {
	float_data.push_back(v.x);
	float_data.push_back(v.y);
}

void join(std::vector<float>& float_data, glm::vec3 v) {
	float_data.push_back(v.x);
	float_data.push_back(v.y);
	float_data.push_back(v.z);
}

void Mesh::draw(std::shared_ptr<ShaderProgram> _shader)
{

	_shader->setVec3Uniform("material_info.Ka", _Ka);
	_shader->setVec3Uniform("material_info.Kd", _Kd);
	_shader->setVec3Uniform("material_info.Ks", _Ks);
	_shader->setFloatUniform("material_info.shininess", _shininess);

	// неиндексированный метод! 

	
	glBindBuffer(GL_ARRAY_BUFFER, _vbo);
	glBufferData(GL_ARRAY_BUFFER, _float_data.size() * sizeof(float), _float_data.data(), GL_STATIC_DRAW); // копируем содержимое массива в буфер на видеокарте
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindSampler(0, _sampler);
	_shader->setIntUniform("with_texture", int(_with_texture));
	_shader->setIntUniform("with_normal_map", int(_with_nmap));
	if (_with_texture) {
		glActiveTexture(GL_TEXTURE0);  //текстурный юнит 0
		_texture->bind();
		glUniform1i(glGetUniformLocation(_shader->id(), "texture_sampler"), 0);
	}
	if (_with_nmap) {
		glActiveTexture(GL_TEXTURE1);  //текстурный юнит 1
		_nmap->bind();
		glUniform1i(glGetUniformLocation(_shader->id(), "nmap_sampler"), 1);
	}
	// наконец, рисуем треугольники
	glBindVertexArray(_vao);
	glDrawArrays(GL_TRIANGLES, 0, _float_data.size() / 8);
}

void Mesh::loadFromFile(std::string filename)
{
	Triangles triangles = ::loadFromFile(filename);
	_addToFloatData(std::move(triangles));
}

void Mesh::updateTriangles(Triangles&& t)
{
	_float_data.clear();
	_addToFloatData(std::move(t));

}

void Mesh::clearTriangles(){
	_float_data.clear();
}

void Mesh::addTriangles(Triangles&& t)
{
	_addToFloatData(std::move(t));
}

void Mesh::attachTexture(TexturePtr texture, float enlarge)
{
	_texture = texture;
	_texture->generateMipmaps();
	_enlarge = enlarge;
	_with_texture = true;
}

void Mesh::attachNMap(std::string filename)
{
	_nmap = loadTexture(filename);
	_nmap->generateMipmaps();
	_with_nmap = true;
}

void Mesh::setMaterialInfo(glm::vec3 Ka, glm::vec3 Kd, glm::vec3 Ks, float shininess)
{
	_Ka = Ka, _Kd = Kd, _Ks = Ks;
	_shininess = shininess;
}

void Mesh::_initSampler()
{
	glSamplerParameteri(_sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glSamplerParameteri(_sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glSamplerParameterf(_sampler, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8.0f);

	glSamplerParameteri(_sampler, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glSamplerParameteri(_sampler, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void Mesh::_addToFloatData(Triangles&& triangles)
{
	for (auto t : triangles) {
		auto norm = normal(t.a, t.b, t.c);
		join(_float_data, t.a);
		join(_float_data, norm);
		join(_float_data, t.ta * _enlarge);
		join(_float_data, t.b);
		join(_float_data, norm);
		join(_float_data, t.tb * _enlarge);
		join(_float_data, t.c);
		join(_float_data, norm);
		join(_float_data, t.tc * _enlarge);
	}
}

void MeshCollection::init(size_t size)
{
	_meshes.resize(size);
	for (size_t i = 0; i < size; ++i)
		_meshes[i] = std::make_shared<Mesh>();
}

void MeshCollection::clear()
{
	for (auto m : _meshes)
		m->clearTriangles();
}

Mesh& MeshCollection::index(size_t i)
{
	return *_meshes[i];
}

void MeshCollection::draw(std::shared_ptr<ShaderProgram> shader)
{
	for (auto m : _meshes) {
		m->draw(shader);
	}
}
