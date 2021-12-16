#include "application.h"

Application::Application()
{
	_initAll();
}

Application::~Application()
{
	closeAll();
}

void Application::run()
{
    // главный цикл приложени€
	while (!glfwWindowShouldClose(_window)) { 
        glfwPollEvents();
        
        _shader->use();
        
        int width, height;
        glfwGetFramebufferSize(_window, &width, &height);
        _free_camera.updateScreen(width, height);
        auto camera_info = _current_camera->cameraInfo();
        
        _updateCameraPos();
        if (freeCameraActive() && _cell != _map.getCell(_free_camera.position()))
            _updateMesh();
        //ImGui_ImplGlfwGL3_NewFrame();
        _shader->setVec3Uniform("camera_pos", _current_camera->position());
        
        _drawPortals();
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // очистка после рендера в frame buffer
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glm::mat4 transform = camera_info.projMatrix * camera_info.viewMatrix;
        _shader->setMat4Uniform("transform", transform);
        Light::setLightInfo(_shader, _current_camera->position(), freeCameraActive());
        _meshes.draw(_shader);
        if (_portal1->active())
            _portal_cores.draw(_shader);
        if (freeCameraActive())
            _minimap->draw(_current_camera->position(), _free_camera.OZ_angle());
        //ImGui::Render();
        glfwSwapBuffers(_window);
	}
}

// обработчики событий. ¬ызываем обработчики приложени€
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    Application* app = (Application*)glfwGetWindowUserPointer(window);
    app->handleKey(key, scancode, action, mods);
}

void windowSizeChangedCallback(GLFWwindow* window, int width, int height)
{
    // do nothing
}

void mouseButtonPressedCallback(GLFWwindow* window, int button, int action, int mods)
{
    // do nothing
}

void mouseCursosPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    Application* app = (Application*)glfwGetWindowUserPointer(window);
    app->handleMouseMove(xpos, ypos);
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    Application* app = (Application*)glfwGetWindowUserPointer(window);
    app->handleScroll(xoffset, yoffset);
}

void Application::_make_window()
{
    auto monitor_size = monitorSize();
    _window = glfwCreateWindow(monitor_size.first, monitor_size.second, "Beehive", nullptr, nullptr); // создаем окно, не нужно полного экрана
    glfwMakeContextCurrent(_window); //ƒелаем это окно текущим

    if (!_window)
    {
        std::cerr << "ERROR: could not open window with GLFW3\n";
        glfwTerminate();
        exit(1);
    }
    ImGui_ImplGlfwGL3_Init(_window, false);
    _setCallback();
}

void Application::_setPortal(std::shared_ptr<Portal> portal)
{
    if (!_map.closeToCenter(_free_camera.position(), _cell.first, _cell.second))
        return;
    portal->close();
    portal->set(_cell.first, _cell.second);
    if (_portal1->isSet() && _portal2->isSet())
        connectPortals(*_portal1, *_portal2);
    _updateMesh();
}

void Application::_changeCamera()
{   // смена способа обзора при нажатии 'c'
    if (freeCameraActive()) {
        _map.ceil() = false;
        _current_camera = &_orbit_camera;
        _map.triangles(_meshes); // whole map
    }
    else {
        _map.ceil() = true;
        _current_camera = &_free_camera;
        _updateMesh();
    }
}



void Application::_initAll()
{  //начальна€ инициализаци€
    initGLFW();

    _make_window();

    initGLEW();

    _minimap = std::make_shared<Minimap>(_window, &_map);

    _makeScene();

    _portal1 = std::make_shared <Portal>(_window, _map, _meshes, _portal_cores.index(PortalTypes::Blue), PortalTypes::Blue);
    _portal2 = std::make_shared <Portal>(_window, _map, _meshes, _portal_cores.index(PortalTypes::LightBlue), PortalTypes::LightBlue);

    _shader = std::make_shared<ShaderProgram>(DIR "shader.vert", DIR "shader.frag");

}

void Application::_initMeshes()
{
    _meshes.init(MESH_NUMBER);
    _portal_cores.init(2);
    attachTextureFromFile(DIR "yellow_brick.png", 2, _meshes.index(MeshTypes::Floor));
    _meshes.index(MeshTypes::Floor).attachNMap(DIR "yellow_brick_normal.png");
    attachTextureFromFile(DIR "honeycomb.png", 1, _meshes.index(MeshTypes::Walls));
    _meshes.index(MeshTypes::Walls).attachNMap(DIR "honeycomb_normal.png");
    _meshes.index(MeshTypes::HonewDrop).setMaterialInfo(glm::vec3(0.2, 0.2, 0), glm::vec3(0.7, 0.5, 0.1), glm::vec3(0.4, 0.4, 0.4), 50);
    _meshes.index(MeshTypes::Portal1).setMaterialInfo(glm::vec3(0, 0, 0.2), glm::vec3(0, 0, 0.8), glm::vec3(0.3, 0.3, 0.3), 100);
    _meshes.index(MeshTypes::Portal2).setMaterialInfo(glm::vec3(0, 0.15, 0.2), glm::vec3(0, 0.6, 0.8), glm::vec3(0.3, 0.3, 0.3), 100);
    for (size_t i = 0; i < POSTER_NUMBER; ++i) {
        attachTextureFromFile(std::string(DIR "poster") + std::to_string(i) + std::string(".png"), 1, _meshes.index(MeshTypes::Posters + i));
        _meshes.index(MeshTypes::Posters + i).setMaterialInfo(glm::vec3(0.2, 0.2, 0.1), glm::vec3(0.7, 0.7, 0.3), glm::vec3(0.1, 0.1, 0.1), 100);
    }
    _portal_cores.index(0).setMaterialInfo(glm::vec3(0, 0, 0), glm::vec3(1, 1, 1), glm::vec3(0, 0, 0), 50);
    _portal_cores.index(1).setMaterialInfo(glm::vec3(0, 0, 0), glm::vec3(1, 1, 1), glm::vec3(0, 0, 0), 50);
}

void Application::_setCallback()
{   // обработчики событий
    glfwSetWindowUserPointer(_window, this); // указатель на текущее приложение
    glfwSetKeyCallback(_window, keyCallback);
    glfwSetWindowSizeCallback(_window, windowSizeChangedCallback);
    glfwSetMouseButtonCallback(_window, mouseButtonPressedCallback);
    glfwSetCursorPosCallback(_window, mouseCursosPosCallback);
    glfwSetScrollCallback(_window, scrollCallback);
}

void Application::_makeScene()
{   // создание лабиринта и загрузка меша
    _initMeshes();
    _current_camera = &_free_camera;
    _free_camera.setMap(&_map);
    _map.setSize(10, 10);
    _map.randomize();
    _updateMesh();
}


void Application::_updateCameraPos()
{
    // обновление камеры и меша
    double cur_time = glfwGetTime(), dt = cur_time - _old_time;
    if (freeCameraActive()) {
        _free_camera.teleport(_window, dt, *_portal1);
        _free_camera.teleport(_window, dt, *_portal2);
    }
    _current_camera->update(_window, dt);
    _old_time = cur_time;
}

void Application::_updateMesh()
{   // загружает другой меш
    _cell = _map.getCell(_free_camera.position());
    //_map.triangles(_cell.first, _cell.second, 5, _meshes);
    _map.triangles(_meshes);
    _minimap->generate(_cell);
}

void Application::_drawPortals()
{
    _portal1->drawToPortalTexture(_shader, _current_camera->position(), _current_camera->cameraInfo(), freeCameraActive());
    _portal2->drawToPortalTexture(_shader, _current_camera->position(), _current_camera->cameraInfo(), freeCameraActive());
}

void Application::handleKey(int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE)
    {
        glfwSetWindowShouldClose(_window, GL_TRUE);
        return;
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_C) {
        _changeCamera();
        return;
    }
    if (action == GLFW_PRESS && key == GLFW_KEY_P) {
        if (!_portal1->isSet())
            _setPortal(_portal1);
        else if (!_portal2->isSet())
            _setPortal(_portal2);
        return;
    }
    if (action == GLFW_PRESS && (key == GLFW_KEY_L || key == GLFW_KEY_1)) {
        _setPortal(_portal1);
        return;
    }
    if (action == GLFW_PRESS && (key == GLFW_KEY_R || key == GLFW_KEY_2)) {
        _setPortal(_portal2);
        return;
    }
    if (action == GLFW_PRESS && (key == GLFW_KEY_R || key == GLFW_KEY_J)) {
        _portal1->__drawPortalQuad();
        _portal2->__drawPortalQuad();
        return;
    }
    

    _current_camera->handleKey(_window, key, scancode, action, mods);
}

void Application::handleMouseMove(double xpos, double ypos)
{
    _current_camera->handleMouseMove(_window, xpos, ypos);
}

void Application::handleScroll(double xoffset, double yoffset)
{
    _current_camera->handleScroll(_window, xoffset, yoffset);
}

bool Application::freeCameraActive() const
{
    return _current_camera == &_free_camera;
}

std::pair<int, int> monitorSize()
{
    int monitorsCount;
    GLFWmonitor** monitors = glfwGetMonitors(&monitorsCount);
    const GLFWvidmode* videoMode = glfwGetVideoMode(monitors[monitorsCount - 1]);
    return std::pair<int, int>(videoMode->height, videoMode->width);
}
