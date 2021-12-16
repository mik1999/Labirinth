#pragma once

#include "utils.h"
#include "mesh.h"

#include <iostream>
#include <vector>
#include <fstream>
#include <set>
#include <tuple>
#include <algorithm>
#include <random>

struct Cell {
	int poster_side; // -1 -- нет плаката, иначе число кодирует сторону с плакатом
	size_t poster_type; // номер плаката
	bool pass[6]; // какие стороны проходимы
	bool drop = false; // наличие капли меда
	int portal_side = -1; // -1 -- нет портала
	int portal_type;
};

class Minimap;
class Portal;

class Map
{
public:
	Map();
	bool load(const std::string& filename);
	void save(const std::string& filename) const;
	void setSize(size_t h, size_t w);
	void randomize();
	bool initialized() const;
	bool& ceil();
	std::pair<int, int> getCell(const glm::vec3& pos);
	bool canGo(const glm::vec3& start, const glm::vec3& finish);
	bool closeToCenter(const glm::vec3& pos, int i, int j);

	void triangles(MeshCollection& meshes) const;
	void triangles(int i, int j, int radius, MeshCollection& meshes) const;

	Triangles portalCore(int i, int j) const;
	void portalQuad(int i, int j, glm::vec3& f1, glm::vec3& f2, glm::vec3& f3, glm::vec3& f4);

	glm::vec3 cent(int i, int j) const;
private:
	friend class Minimap;
	friend class Portal;
	static const int _dx[6], _dy[6];
	static const float _wall_thick;
	static const glm::vec3 v1, v2, v3;
	
	void _randomWalk(int i, int j);
	bool _inMap(int i, int j) const;

	Triangles _obstacles(int i, int j) const;
	Triangles _portalTriangles(int i, int j) const;
	void _cellTriangles(int i, int j, MeshCollection& meshes) const;
	void _cellPoster(int i, int j, MeshCollection& meshes) const;
	void _cellDrop(int i, int j, MeshCollection& meshes) const;
	Triangles _floor, _ceiling, _door, _wall, _drop, _portal_frame, _portal_core;
	size_t _h = 0, _w = 0;
	bool _init = false;
	bool _ceil = true;
	std::vector<std::vector<Cell>> _cells;
};

