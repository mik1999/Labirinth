#include "utils.h"

GLuint getUniformBuffer(GLuint program) {
	GLuint buffer;

	// мы не используем прямой доступ к состоянию (DSA)
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_UNIFORM_BUFFER, buffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0); // поюзал -- отбайндил

	glBindBufferBase(GL_UNIFORM_BUFFER, UnBindPoint, buffer); // привязываем буфер
	return buffer;
}

void initGLFW()
{
	if (!glfwInit()) //Инициализируем библиотеку GLFW
	{
		std::cerr << "ERROR: could not start GLFW3\n";
		exit(1);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // GLFW 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // совместимость с новыми версиями
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // еще больше совместимости не надо
	// glfwSwapInterval(0); //Отключаем вертикальную синхронизацию
}


void initGLEW() {

	glewExperimental = GL_TRUE; // Для версий GLEW 1.13 и ранее
	if (glewInit() != GLEW_OK) {
		std::cerr << "ERROR: could not start GLEW\n";
		exit(1);
	}; //Инициализируем библиотеку GLEW

	glGetError(); // Сбросим флаг ошибки.

	/*
	DebugOutput _debug_output; // отладочный вывод

	if (DebugOutput::isSupported())
		_debug_output.attach(); // подключаем дебаг-вывод
	*/

	glEnable(GL_DEPTH_TEST); // включаем тест глубины
	glDepthFunc(GL_LESS); 

}


void closeAll()
{
	glfwTerminate();
	ImGui_ImplGlfwGL3_Shutdown();
}

glm::vec3 normal(glm::vec3 a, glm::vec3 b, glm::vec3 c)
{
	glm::vec3 n = glm::cross(b - a, a - c);
	return glm::normalize(n);
}

glm::mat2 rotate(float alpha)
{
	return glm::mat2(glm::cos(alpha), glm::sin(alpha), -glm::sin(alpha), glm::cos(alpha));
}

bool TriangleSegmentIntersect(const glm::vec3& a1, const glm::vec3& a2, const glm::vec3& a3, const glm::vec3& b1, const glm::vec3& b2)
{
	glm::vec2 p;
	float d;
	glm::intersectRayTriangle(b1, b2 - b1, a1, a2, a3, p, d);
	if (d < 0)
		return false;
	glm::intersectRayTriangle(b2, b1 - b2, a1, a2, a3, p, d);
	return d > 0;
}

float distanceToTriangle(glm::vec3 o, Triangle t)
{
	glm::vec3 a = t.a, b = t.b, c = t.c;
	glm::vec2 coef = glm::inverse(glm::mat2x2(glm::dot(b - a, b - a), glm::dot(c - a, b - a), glm::dot(b - a, c - a), 
		glm::dot(c - a, c - a))) * glm::vec2(glm::dot(o - a, b - a), glm::dot(o - a, c - a));
	float al = coef.x, be = coef.y;
	if (0 <= al && 0 <= be && al + be <= 1) { // проекция падает внутрь треугольника
		return glm::distance(o, a + al * (b - a) + be * (c - a));
	}
	return std::min(glm::distance(o, a), std::min(glm::distance(o, b), glm::distance(o, c)));
}

glm::vec3 project(const glm::vec3& v, const glm::mat4x4& m) {
	auto mul = m * glm::vec4(v, 1);
	return glm::vec3(mul.x, mul.y, mul.z) / mul[3];
}

// создает матрицу проективого преобразования, переводящего 5 точек в 5 точек
glm::mat4 projectmatrix5to5(const glm::vec4& x1, const glm::vec4& x2, const glm::vec4& x3, const glm::vec4& x4, const glm::vec4& x5, const glm::vec4& y1, const glm::vec4& y2, const glm::vec4& y3, const glm::vec4& y4, const glm::vec4& y5)
{
	glm::mat4 X, Y;
	X[0] = x1, X[1] = x2, X[2] = x3, X[3] = x4, Y[0] = y1, Y[1] = y2, Y[2] = y3, Y[3] = y4;
	glm::mat4 X_inv = glm::inverse(X), Y_inv = glm::inverse(Y);
	glm::vec4 v = (Y_inv * y5), u = (X_inv * x5), alpha = v / u;
	for (int i = 0; i < 4; ++i)
		Y[i] *= alpha[i];
	return Y * X_inv;
}

Triangles loadFromFile(std::string filename)
{
	Triangles triangles;
	std::ifstream in = std::ifstream(filename);
	glm::vec3 a, b, c;
	glm::vec2 ta, tb, tc;
	Triangle t;
	while (in >> a.x) {
		in >> a.y >> a.z >> ta.x >> ta.y >> b.x >> b.y >> b.z >> tb.x >> tb.y >> c.x >> c.y >> c.z >> tc.x >> tc.y;
		t.a = a, t.b = b, t.c = c, t.ta = ta, t.tb = tb, t.tc = tc;
		triangles.push_back(t);
	}
	return triangles;
}

void joinTriangles(Triangles& t1, Triangles&& t2)
{
	size_t os = t1.size();
	t1.resize(os + t2.size());
	std::copy(t2.begin(), t2.end(), t1.begin() + os);
	t2.clear();
}

float matNorm(const glm::mat4& mat)
{
	float ans = 0;
	for (int i = 0; i < 4; ++i)
		for (int j = 0; j < 4; ++j)
			ans += mat[i][j] * mat[i][j];
	return ans;
}

void printMatrix(const glm::mat4& matrix)
{
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j)
			std::cout << matrix[i][j] << ' ';
		std::cout << '\n';
	}
}

void projectTriangles(Triangles& t, glm::mat4x4 transform)
{
	Triangles old = t;
	for (size_t i = 0; i < t.size(); ++i) {
		t[i].a = ::project(old[i].a, transform);
		t[i].b = ::project(old[i].b, transform);
		t[i].c = ::project(old[i].c, transform);
	}
}

void Light::setLightInfo(std::shared_ptr<ShaderProgram> shader, const glm::vec3& camera_pos, bool free_camera)
{
	shader->setIntUniform("light_info.point_flag", int(free_camera));
	if (free_camera) {
		shader->setVec3Uniform("light_info.pos", camera_pos);
		shader->setVec3Uniform("light_info.La", La_free);
		shader->setVec3Uniform("light_info.Ld", Ld_free);
		shader->setVec3Uniform("light_info.Ls", Ls_free);
	}
	else {
		shader->setVec3Uniform("light_info.pos", light_dir);
		shader->setVec3Uniform("light_info.La", La_orbit);
		shader->setVec3Uniform("light_info.Ld", Ld_orbit);
		shader->setVec3Uniform("light_info.Ls", Ls_orbit);
	}
}
