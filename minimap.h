#include "utils.h"
#include "map.h"

class Minimap {
public:
	Minimap() = delete;
	Minimap(GLFWwindow* _window, Map* map);
	~Minimap();
	void generate(std::pair<int, int> pos);
	void draw(glm::vec3 pos, float alpha);
private:
	struct MMTriangle {
		glm::vec3 a, b, c;
		glm::vec3 color;
	};

	const float radius = 0.2, wall_width = 0.02, door_par = 0.6, v_coef = (radius + wall_width) * glm::sqrt(3);
	const glm::vec2 v1 = glm::vec2(1, 0), v2 = glm::vec2(0.5, glm::sqrt(3) / 2),
		r1 = glm::vec2(-glm::sqrt(3) / 2, -0.5), r2 = glm::vec2(-glm::sqrt(3) / 2, 0.5), n = glm::vec2(-wall_width * glm::sqrt(3) / 2, 0);

	int i, j; // текуща€ €чейка

	void _addTriangle(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec3 color, float h);
	void _addTrianglesToFloatData(const std::vector<MMTriangle>& t);
	void _renewWinSize();
	std::vector<MMTriangle> _player(glm::vec3 pos, float alpha);

	std::vector<MMTriangle> _triangles;
	std::vector<float> _float_data;
	int _width, _height;
	GLuint _vao = 0, _vbo = 0;
	Map* _map;
	GLFWwindow* _window;
	ShaderProgramPtr _shader;
};