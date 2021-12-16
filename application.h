#pragma once
#include <iostream>

#include "utils.h"
#include "MyCameraMover.h"
#include "minimap.h"
#include "map.h"
#include "mesh.h"

std::pair<int, int> monitorSize();

class Application
{
public:
	Application();
	~Application();

	void run();

	void handleKey(int key, int scancode, int action, int mods);
	void handleMouseMove(double xpos, double ypos);
	void handleScroll(double xoffset, double yoffset);
	bool freeCameraActive() const;
private:
	void _make_window();

	void _setPortal(std::shared_ptr<Portal> portal);
	void _changeCamera();
	
	void _initAll();
	void _initMeshes();
	void _setCallback(); // обработка событий: вызывать каждый раз при переключении камеры
	void _makeScene();
	void _updateCameraPos();
	void _updateMesh();
	void _drawPortals();


	glm::mat4 tranform;

	double _old_time; // время последней отрисовки
	GLFWwindow* _window; // текущий контекст
	std::pair<int, int> _cell; // координаты текущей ячейки

	std::shared_ptr<ShaderProgram> _shader;  // шейдеры
	Map _map; // карта лабиринта
	std::shared_ptr<Minimap> _minimap; // миникарта
	MeshCollection _meshes; // все треугольники в округе в виде меша
	MeshCollection _portal_cores; // текстуры портала
	MyOrbitCameraMover _orbit_camera; // две камеры
	MyFreeCameraMover _free_camera;
	CameraMover* _current_camera;
	std::shared_ptr<Portal> _portal1, _portal2;
};

