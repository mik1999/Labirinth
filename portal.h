#include "mesh.h"
#include "map.h"

class Portal;
void connectPortals(Portal& p1, Portal& p2);

class Portal {
public:
	Portal() = delete;
	Portal(GLFWwindow* window, Map& map, MeshCollection& meshes, Mesh& portal_core, PortalTypes type);
	~Portal();

	bool isSet();
	bool set(size_t i, size_t j);
	void close();
	bool active();
	bool teleport(glm::vec3 start, glm::vec3& finish, glm::quat& or);
	void __drawPortalQuad();
	void drawToPortalTexture(std::shared_ptr<ShaderProgram> _shader, const glm::vec3& camera_pos, const CameraInfo& camera_info, bool fca);
private:

	static const size_t BUF_SIZE = 1000;

	friend void connectPortals(Portal& p1, Portal& p2);
	struct PortalPosition {
		size_t i, j;
		short unsigned int d;
	};

	glm::mat4 _projectMatrix(const glm::vec3& camera_pos);

	bool _set = false, _active = false;

	GLuint _frame_buf_id, _depth_buf_id;
	TexturePtr _texture;
	GLFWwindow* _window;
	PortalTypes _type;
	PortalPosition _from, _to;
	Triangles _portal_core_triangles;
	Mesh& _portal_core;
	MeshCollection& _meshes;
	glm::vec3 _f1, _f2, _f3, _f4; // вершины квадрата, на который проецируем и который потом клеим на внутреннюю часть пор
	glm::quat _rot_change;
	glm::mat4 _teleport_matrix;
	Map& _map;
};