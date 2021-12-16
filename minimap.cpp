#include "minimap.h"

Minimap::Minimap(GLFWwindow* window, Map* map) : _window(window), _map(map)
{
	_shader = std::make_shared<ShaderProgram>(DIR "mm_shader.vert", DIR "mm_shader.frag");
	glGenBuffers(1, &_vbo); // создаем буфер для хранения вершин
	glGenVertexArrays(1, &_vao); // создаем объект VertexArrayObject для хранения настроек полигональной модели

	glBindVertexArray(_vao);
	glBindBuffer(GL_ARRAY_BUFFER, _vbo);

	glEnableVertexAttribArray(0); // включаем 0-ой вершинный атрибут -- координаты
	glEnableVertexAttribArray(1); // включаем 1-ый вершинный атрибут -- цвет

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

Minimap::~Minimap()
{
	glDeleteBuffers(1, &_vbo);
	glDeleteVertexArrays(1, &_vao);
}

void Minimap::generate(std::pair<int, int> pos)
{
	_triangles.clear();

	i = pos.first, j = pos.second;
	for (int x = j - 2; x <= j + 2; ++x) {
		for (int y = i + std::max(-2, j - x - 2); y <= i + std::min(2, j - x + 2); ++y) {
			if (!_map->_inMap(y, x)) {
				continue;
			}
			glm::vec2 center = v_coef * (float(x - j) * v1 + float(y - i) * v2);
			for (int d = 0; d < 6; ++d) {
				glm::vec2 a = radius * rotate(-glm::pi<float>() * d / 3) * r1, b = radius * rotate(-glm::pi<float>() * d / 3) * r2,
					nd = rotate(-glm::pi<float>() * d / 3) * n, a1 = a * (1 + wall_width / radius), b1 = b * (1 + wall_width / radius);
				_addTriangle(center, center + a, center + b, glm::vec3(0.4, 0.4, 0), 0);
				if (_map->_cells[y][x].portal_side == d) {
					glm::vec2 a = center + rotate(-glm::pi<float>() * d / 3) * glm::vec2(-radius * glm::sqrt(3.0f) / 2, -radius * 0.35);
					glm::vec2 b = center + rotate(-glm::pi<float>() * d / 3) * glm::vec2(-radius * glm::sqrt(3.0f) / 2, radius * 0.35);
					glm::vec2 b1 = center + rotate(-glm::pi<float>() * d / 3) * glm::vec2(-radius * (glm::sqrt(3.0f) / 2 - 0.2), radius * 0.35);
					glm::vec2 a1 = center + rotate(-glm::pi<float>() * d / 3) * glm::vec2(-radius * (glm::sqrt(3.0f) / 2 - 0.2), -radius * 0.35);
					glm::vec3 color;
					if (_map->_cells[y][x].portal_type == PortalTypes::Blue)
						color = glm::vec3(0, 0, 1);
					else
						color = glm::vec3(0, 0.75, 1);
					_addTriangle(a, b1, b, color, -0.5);
					_addTriangle(a, a1, b1, color, -0.5);
				}
				if (_map->_cells[y][x].pass[d]) {
					glm::vec2 c = a * (1 + door_par) / 2.0f + b * (1 - door_par) / 2.0f, d = a * (1 - door_par) / 2.0f + b * (1 + door_par) / 2.0f;
					_addTriangle(center + a, center + a1, center + c, glm::vec3(0.8, 0.8, 0), 0);
					_addTriangle(center + a1, center + c + nd, center + c, glm::vec3(0.8, 0.8, 0), 0);
					_addTriangle(center + b, center + b1, center + d, glm::vec3(0.8, 0.8, 0), 0);
					_addTriangle(center + b1, center + d + nd, center + d, glm::vec3(0.8, 0.8, 0), 0);
					_addTriangle(center + c, center + c + nd, center + d, glm::vec3(0.4, 0.4, 0), 0);
					_addTriangle(center + d, center + c + nd, center + d + nd, glm::vec3(0.4, 0.4, 0), 0);
				} else {
					glm::vec3 color;
					if (_map->_inMap(y + _map->_dy[d], x + _map->_dx[d]))
						color = glm::vec3(0.8, 0.8, 0);
					else
						color = glm::vec3(0.2, 0.7, 0.2);
					_addTriangle(center + a, center + a1, center + b, color, 0);
					_addTriangle(center + a1, center + b, center + b1, color, 0);
				}
			}

		}
	}
}

void Minimap::draw(glm::vec3 pos, float alpha)
{
	_shader->use();
	_renewWinSize();
	glViewport(_width / 2, _height / 2, _width, _height);
	// неиндексированный метод! 
	_float_data.clear();

 	_addTrianglesToFloatData(_triangles);
	_addTrianglesToFloatData(_player(pos, alpha));

	glBindBuffer(GL_ARRAY_BUFFER, _vbo);
	glBufferData(GL_ARRAY_BUFFER, _float_data.size() * sizeof(float), _float_data.data(), GL_STATIC_DRAW); // копируем содержимое массива в буфер на видеокарте
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(_vao);
	glDrawArrays(GL_TRIANGLES, 0, _float_data.size() / 6);
}

void Minimap::_addTriangle(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec3 color, float h)
{
	_triangles.push_back(MMTriangle({ glm::vec3(a, h), glm::vec3(b, h), glm::vec3(c, h), color }));
}

void Minimap::_addTrianglesToFloatData(const std::vector<MMTriangle>& triangles)
{
	for (auto t : triangles) {
		join(_float_data, t.a);
		join(_float_data, t.color);
		join(_float_data, t.b);
		join(_float_data, t.color);
		join(_float_data, t.c);
		join(_float_data, t.color);
	}
}

void Minimap::_renewWinSize()
{
	glfwGetFramebufferSize(_window, &_width, &_height);
	_height /= 4;
	_width = _height;
}

std::vector<typename Minimap::MMTriangle> Minimap::_player(glm::vec3 pos, float alpha)
{
	std::vector<MMTriangle> ans;
	glm::vec3 color = glm::vec3(1, 0, 0);
	float v2_coord = (pos.y - i * _map->v2.y) / _map->v2.y, v1_coord = (pos.x - (i + v2_coord) * _map->v2.x - j * _map->v1.x) / _map->v1.x;
	glm::vec2 center = (v1 * v1_coord + v2 * v2_coord) * (radius + wall_width) * glm::sqrt(3.0f);
	glm::vec2 a = glm::vec2(0.1, 0), b = glm::vec2(-0.05, 0.05), c = glm::vec2(-0.05, -0.05);
	ans.push_back(MMTriangle({ glm::vec3(center, -1), glm::vec3(center + rotate(alpha) * a, -1), glm::vec3(center + rotate(alpha) * b, -1), color }));
	ans.push_back(MMTriangle({ glm::vec3(center, -1), glm::vec3(center + rotate(alpha) * a, -1), glm::vec3(center + rotate(alpha) * c, -1), color }));
	return ans;
}
