#include "portal.h"

void connectPortals(Portal& p1, Portal& p2)
{
	p1._to = p2._from;
	p2._to = p1._from;
	p1._active = p2._active = true;
	p1._rot_change = glm::angleAxis(glm::pi<float>() * (1 + (p2._from.d - p1._from.d) / 3.0f), glm::vec3(0, 0, 1));
	p2._rot_change = glm::angleAxis(glm::pi<float>() * (1 + (p1._from.d - p2._from.d) / 3.0f), glm::vec3(0, 0, 1));
	p1._teleport_matrix = glm::translate(p2._map.cent(p2._from.i, p2._from.j)) * glm::rotate(-glm::pi<float>() * (p2._from.d / 3.0f + 1), glm::vec3(0, 0, 1)) * 
		glm::translate(glm::vec3(glm::sqrt(3.0f) - PORTAL_WIDTH, 0, 0)) * glm::rotate(glm::pi<float>() * p1._from.d / 3.0f, glm::vec3(0, 0, 1)) *
		glm::translate(-p1._map.cent(p1._from.i, p1._from.j));
	p2._teleport_matrix = glm::translate(p1._map.cent(p1._from.i, p1._from.j)) * glm::rotate(-glm::pi<float>() * (p1._from.d / 3.0f + 1), glm::vec3(0, 0, 1)) *
		glm::translate(glm::vec3(glm::sqrt(3.0f) - PORTAL_WIDTH, 0, 0)) * glm::rotate(glm::pi<float>() * p2._from.d / 3.0f, glm::vec3(0, 0, 1)) *
		glm::translate(-p2._map.cent(p2._from.i, p2._from.j));
	p1._map.portalQuad(p1._to.i, p1._to.j, p1._f1, p1._f2, p1._f3, p1._f4);
	p2._map.portalQuad(p2._to.i, p2._to.j, p2._f1, p2._f2, p2._f3, p2._f4);
}

Portal::Portal(GLFWwindow* window, Map& map, MeshCollection& meshes, Mesh& portal_core, PortalTypes type): _map(map), _meshes(meshes), _portal_core(portal_core), _type(type), _window(window)
{
	// Создалим кадровый буфер и изображение для вывода
	glGenFramebuffers(1, &_frame_buf_id);
	glBindFramebuffer(GL_FRAMEBUFFER, _frame_buf_id);

	GLuint tex_id;
	glGenTextures(1, &tex_id);
	glBindTexture(GL_TEXTURE_2D, tex_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, BUF_SIZE, BUF_SIZE, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	_texture = std::make_shared<Texture>(tex_id, GL_TEXTURE_2D);
	_texture->attachToFramebuffer(GL_COLOR_ATTACHMENT0);
	

	glGenRenderbuffers(1, &_depth_buf_id);
	glBindRenderbuffer(GL_RENDERBUFFER, _depth_buf_id);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, BUF_SIZE, BUF_SIZE);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depth_buf_id);

	GLenum dest = GL_COLOR_ATTACHMENT0;
	glDrawBuffers(1, &dest);

	assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	_portal_core.attachTexture(_texture);
}

Portal::~Portal()
{
	glDeleteFramebuffers(1, &_frame_buf_id);
	// текстура удалится в другой функции
	glDeleteRenderbuffers(1, &_depth_buf_id);

}

bool Portal::isSet()
{
	return _set;
}

bool Portal::set(size_t i, size_t j)
{
	if (_map._cells[i][j].portal_side != -1)
		return false;
	std::random_device rd;
	int d = rd() % 6;
	for (int k = 0; k < 6; ++k, d = (d == 5 ? 0 : d + 1)) {
		if (_map._cells[i][j].pass[d] || _map._cells[i][j].poster_side == d)
			continue;
		_from = PortalPosition({ i, j, (unsigned short)d });
		_map._cells[i][j].portal_side = d;
		_map._cells[i][j].portal_type = _type;
		_portal_core_triangles = _map.portalCore(i, j);
		_portal_core.addTriangles(std::move(_portal_core_triangles));
		_set = true;
		return true;
	}
	return false;
}

void Portal::close()
{
	const size_t index = MeshTypes::Portal1 + _type;
	_meshes.index(index).clearTriangles();
	_portal_core.clearTriangles();
	if (_set)
		_map._cells[_from.i][_from.j].portal_side = -1;
	_set = _active = false;
}

bool Portal::active()
{
	return _active;
}

bool Portal::teleport(glm::vec3 start, glm::vec3& finish, glm::quat& or )
{

	if (!_active)
		return false;
	for (Triangle& t : _portal_core_triangles) {
		if (TriangleSegmentIntersect(t.a, t.b, t.c, start, finish)) {
			finish = project(finish, _teleport_matrix);
			or *= _rot_change;
			return true;
		}
	}

	return false;
}

void Portal::__drawPortalQuad()
{
	_meshes.index(0).addTriangles(Triangles(1, Triangle({ _f1, _f3, _f2, glm::vec2(0, 0), glm::vec2(1, 1), glm::vec2(0, 1) })));
	_meshes.index(0).addTriangles(Triangles(1, Triangle({ _f4, _f3, _f1, glm::vec2(1, 0), glm::vec2(1, 1), glm::vec2(1, 0) })));

}

void Portal::drawToPortalTexture(std::shared_ptr<ShaderProgram> _shader, const glm::vec3& camera_pos, const CameraInfo& camera_info, bool fca)
{
	if (!_active)
		return;
	glBindFramebuffer(GL_FRAMEBUFFER, _frame_buf_id);

	glViewport(0, 0, BUF_SIZE, BUF_SIZE);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 transform = glm::scale(glm::vec3(-1, 1, 1)) * _projectMatrix(camera_pos);
	Light::setLightInfo(_shader, project(camera_pos, _teleport_matrix), fca);
	_shader->setMat4Uniform("transform", transform);
	
	_meshes.draw(_shader);
	_texture->generateMipmaps();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
}

glm::mat4 Portal::_projectMatrix(const glm::vec3& camera_pos)
{
	glm::vec3 projected_pos = project(camera_pos, _teleport_matrix);
	glm::vec4 a1 = glm::vec4(_f1, 1), a2 = glm::vec4(_f2, 1), a2_far = glm::vec4(projected_pos + 100.0f * ( _f2 - projected_pos), 1), a3 = glm::vec4(_f3, 1), a4 = glm::vec4(_f4, 1), a4_far = glm::vec4(projected_pos + 100.0f * (_f4 - projected_pos), 1);
	return projectmatrix5to5(glm::vec4(projected_pos, 1), a1, a2, a3, a4_far,
		glm::vec4(0, 0, 1, 0), glm::vec4(1, -1, -1, 1),  glm::vec4(1, 1, -1, 1), glm::vec4(-1, 1, -1, 1), glm::vec4(-1, -1, 1, 1));
}