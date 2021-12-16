#include "map.h"

const int Map::_dx[6]{ -1, -1, 0, 1, 1, 0 }, Map::_dy[6]{ 0, 1, 1, 0, -1, -1 };

const float Map::_wall_thick = 0.02;

const glm::vec3 Map::v1 = (glm::sqrt(3.0f) + _wall_thick) * glm::vec3(1, 0, 0);
const glm::vec3 Map::v2 = (glm::sqrt(3.0f) + _wall_thick) * glm::normalize(glm::vec3(glm::sqrt(3) / 4, 0.75, 0));
const glm::vec3 Map::v3(0, 0, 1);

Map::Map()
{
    _floor = loadFromFile(DIR "floor.txt");
    _ceiling = loadFromFile(DIR "ceiling.txt");
    _door = loadFromFile(DIR "door.txt");
    _wall = loadFromFile(DIR "wall.txt");
    _drop = loadFromFile(DIR "drop_reverse.txt");
    _portal_frame = loadFromFile(DIR "portal_frame.txt");
    _portal_core = loadFromFile(DIR "portal_core.txt");
}

bool Map::load(const std::string& filename)
{
    std::ifstream in(filename);
    if (!in)
        return false;
    in >> _h >> _w;
    char c;
    for (size_t y = 0; y < _h; ++y)
        for (size_t x = 0; x < _w; ++x)
            for (size_t k = 0; k < 6; ++k) {
                in >> c;
                _cells[y][x].pass[k] = (c == '1');
            }
    _init = true;
    return true;
}

void Map::save(const std::string& filename) const
{
    // информаци€ о плакатах не сохран€етс€!
    std::ofstream out(filename);
    out << _h << ' ' << _w << '\n';
    for (size_t y = 0; y < _h; ++y) {
        for (size_t x = 0; x < _w; ++x) {
            for (size_t k = 0; k < 6; ++k)
                out << (int)_cells[y][x].pass[k];
            //out << ' ';
        }
        out << '\n';
    }
}

void Map::setSize(size_t h, size_t w)
{
    _h = h;
    _w = w;
    _cells.resize(_h);
    for (size_t y = 0; y < _h; ++y) {
        _cells[y].resize(_w);
        for (size_t x = 0; x < _w; ++x)
            for (size_t k = 0; k < 6; ++k)
                _cells[y][x].pass[k] = false;
    }
}

void Map::randomize()
{
    std::random_device rd;
    _randomWalk(0, 0);
    for (int i = 0; i < glm::sqrt(_h * _w); ++i) {
        _randomWalk(rd() % _h, rd() % _w);
    }
    for (int i = 0; i < _h; ++i)
        for (int j = 0; j < _w; ++j) {
            if (rd() % 1 == 0) {
                int side = rd() % 6;
                if (_cells[i][j].pass[side])
                    _cells[i][j].poster_side = -1;
                else {
                    _cells[i][j].poster_side = side;
                    _cells[i][j].poster_type = rd() % POSTER_NUMBER;
                }
            }
            else
                _cells[i][j].poster_side = -1;
            //if (rd() % 5 == 0)
            //    _cells[i][j].drop = true;
        }
    _init = true;
}

bool Map::initialized() const
{
    return _init;
}

bool& Map::ceil()
{
    return _ceil;
}

std::pair<int, int> Map::getCell(const glm::vec3& pos) // возвращает пару y, x
{
    float x = pos.x, y = pos.y;
    float i = y / v2.y;
    float j = (x - i * v2.x) / v1.x; 
    
    int k = int(i + 0.5), l = int(j + 0.5); // здесь мы могли ошибитьс€, попасть в соседнюю €чейку
    std::pair<int, int> ans = std::make_pair(k, l);
    float min_dist = glm::length(glm::vec3(x, y, 0) - v1 * float(l) - v2 * float(k));
    for (int u = 0; u < 6; ++u) {
        int k1 = k + _dy[u], l1 = l + _dx[u];
        float dist = glm::length(glm::vec3(x, y, 0) - v1 * float(l1) - v2 * float(k1));
        if (dist < min_dist) {
            min_dist = dist;
            ans = std::make_pair(k1, l1);
        }
    }
    return ans;
}

bool Map::canGo(const glm::vec3& start, const glm::vec3& finish)
{
    // провер€ет, можно ли пройти из точки start в finish
    // критерий: сфера радиуса 1.1 * NEAR вокруг finish не пересекает ни один треугольник €чейки исходной позиции
    if (glm::length(finish - start) > NEAR) {
        return false; // длинные шаги нельз€ делать
    }
    auto cell = getCell(start);
    auto triangles = _obstacles(cell.first, cell.second);
    for (auto t : triangles) {
        if (distanceToTriangle(finish, t) < NEAR * 1.1f)
            return false;
    }
    return true;
}

bool Map::closeToCenter(const glm::vec3& pos, int i, int j)
{
    return glm::length(pos - float(j) * v1 - float(i) * v2 - 0.5f * v3) < 0.9;
}

void Map::triangles(MeshCollection& meshes) const
{
    meshes.clear();
    for (size_t i = 0; i < _h; ++i)
        for (size_t j = 0; j < _w; ++j) {
            _cellTriangles(i, j, meshes);
        }
}

void Map::triangles(int i, int j, int radius, MeshCollection& meshes) const
{
    meshes.clear();
    for (int k = i - radius; k <= i + radius; ++k)
        for (int l = j - radius; l <= j + radius; ++l) {
            if (!_inMap(k, l))
                continue;
            _cellTriangles(k, l, meshes);
        }
}

Triangles Map::portalCore(int i, int j) const
{
    assert(_cells[i][j].portal_side != -1);
    Triangles t = _portal_core;
    glm::vec3 center = cent(i, j) + 0.5f * v3;
    projectTriangles(t, glm::translate(center) *
        glm::rotate(glm::pi<float>() * (-0.5f - 2 * _cells[i][j].portal_side / 6.0f), glm::vec3(0, 0, 1)) *
        glm::translate(glm::vec3(0, -glm::sqrt(3.0f) / 2, 0)));
    return t;
}

void Map::portalQuad(int i, int j, glm::vec3& f1, glm::vec3& f2, glm::vec3& f3, glm::vec3& f4)
{
    f1 = glm::vec3(PORTAL_WIDTH / 2 - glm::sqrt(3.0f) / 2, -PORTAL_RADIUS, 0.5f - PORTAL_RADIUS);
    f2 = glm::vec3(PORTAL_WIDTH / 2 - glm::sqrt(3.0f) / 2, -PORTAL_RADIUS, 0.5f + PORTAL_RADIUS);
    f3 = glm::vec3(PORTAL_WIDTH / 2 - glm::sqrt(3.0f) / 2, PORTAL_RADIUS, 0.5f + PORTAL_RADIUS);
    f4 = glm::vec3(PORTAL_WIDTH / 2 - glm::sqrt(3.0f) / 2, PORTAL_RADIUS, 0.5f - PORTAL_RADIUS);
    glm::vec3 center = cent(i, j);
    glm::mat4 project_matrix = glm::translate(center) * glm::rotate(glm::pi<float>() * (-_cells[i][j].portal_side / 3.0f), glm::vec3(0, 0, 1));
    f1 = project(f1, project_matrix), f2 = project(f2, project_matrix), f3 = project(f3, project_matrix), f4 = project(f4, project_matrix);
}

glm::vec3 Map::cent(int i, int j) const
{
    return float(i) * v2 + float(j) * v1;
}

void Map::_randomWalk(int i, int j)
{
    std::random_device rd;
    for (int l = 0; l < glm::sqrt(_h * _w); ++l) {
        int i1, j1, k;
        do {
            k = rd() % 6;
            i1 = i + _dy[k], j1 = j + _dx[k];

        } while (!_inMap(i1, j1));
        _cells[i][j].pass[k] = true;
        _cells[i1][j1].pass[(k + 3) % 6] = true;
        i = i1, j = j1;
    }
}

bool Map::_inMap(int i, int j) const
{
    return i >= 0 && j >= 0 && i < _h && j < _w;
}

Triangles Map::_obstacles(int i, int j) const
{
    Triangles ans, temp;
    glm::vec3 center = cent(i, j);
    for (size_t d = 0; d < 6; ++d) {
        if (_cells[i][j].pass[d])
            temp = _door;
        else
            temp = _wall;
        projectTriangles(temp, glm::translate(center) *
            glm::rotate(glm::pi<float>() * (-0.5f - 2 * d / 6.0f), glm::vec3(0, 0, 1)) *
            glm::translate(glm::vec3(-0.5f, -glm::sqrt(3.0f) / 2, .0f)));
        joinTriangles(ans, std::move(temp));
        if (_cells[i][j].portal_side != -1)
            joinTriangles(ans, _portalTriangles(i, j));
    }
    return ans;
}

Triangles Map::_portalTriangles(int i, int j) const
{
    Triangles t = _portal_frame;
    glm::vec3 center = float(j) * v1 + float(i) * v2 + 0.5f * v3;
    projectTriangles(t, glm::translate(center) *
        glm::rotate(glm::pi<float>() * (-0.5f - 2 * _cells[i][j].portal_side / 6.0f), glm::vec3(0, 0, 1)) *
        glm::translate(glm::vec3(0, -glm::sqrt(3.0f) / 2, 0)));
    return t;
}

void Map::_cellTriangles(int i, int j, MeshCollection& meshes) const
{
    Triangles temp;
    glm::vec3 center = cent(i, j);
    temp = _floor;
    projectTriangles(temp, glm::translate(center));
    meshes.index(MeshTypes::Floor).addTriangles(std::move(temp)); // add стирает temp
    if (_ceil) {
        temp = _ceiling;
        projectTriangles(temp, glm::translate(center));
        meshes.index(MeshTypes::Ceiling).addTriangles(std::move(temp));

    }
    for (size_t d = 0; d < 6; ++d) {
        if (_cells[i][j].pass[d])
            temp = _door;
        else
            temp = _wall;
        projectTriangles(temp, glm::translate(center) * 
            glm::rotate(glm::pi<float>() * (-0.5f - d / 3.0f), glm::vec3(0, 0, 1)) * 
            glm::translate(glm::vec3(-0.5f, -glm::sqrt(3.0f) / 2, .0f)));
        meshes.index(MeshTypes::Walls).addTriangles(std::move(temp));

    }
    _cellPoster(i, j, meshes);
    _cellDrop(i, j, meshes);

    if (_cells[i][j].portal_side != -1) {
        const size_t index = MeshTypes::Portal1 + _cells[i][j].portal_type;
        meshes.index(index).addTriangles(_portalTriangles(i, j));
    }
}

void Map::_cellPoster(int i, int j, MeshCollection& meshes) const
{
    if (_cells[i][j].poster_side == -1)
        return;
    glm::vec3 center = cent(i, j);
    auto t_size = meshes.index(MeshTypes::Posters + _cells[i][j].poster_type).texSize();
    int w = t_size.first, h = t_size.second;
    glm::vec3 a1{ -float(w) / 2000, 0, 0 }, b1{ float(w) / 2000, 0, 0 }, c1 = { float(w) / 2000, 0.01, 0 }, d1 = { -float(w) / 2000, 0.01, 0 }, n = v3 * float(h) / 1000.0f;
    glm::vec3 a2 = a1 + n, b2 = b1 + n, c2 = c1 + n, d2 = d1 + n;
    Triangles t;
    t.push_back(Triangle({ d2, d1, c1, glm::vec2(0, 1), glm::vec2(0, 0), glm::vec2(1, 0) }));
    t.push_back(Triangle({ c2, d2, c1, glm::vec2(1, 1), glm::vec2(0, 1), glm::vec2(1, 0) }));
    glm::vec2 edge{ 0, 0 };
    t.push_back(Triangle({ a2, a1, d1, edge, edge, edge }));
    t.push_back(Triangle({ a2, d1, d2, edge, edge, edge }));
    t.push_back(Triangle({ b2, a2, d2, edge, edge, edge }));
    t.push_back(Triangle({ b2, d2, c2, edge, edge, edge }));
    t.push_back(Triangle({ b1, b2, c2, edge, edge, edge }));
    t.push_back(Triangle({ b1, c2, c1, edge, edge, edge }));
    t.push_back(Triangle({ c1, b1, a1, edge, edge, edge }));
    t.push_back(Triangle({ c1, a1, d1, edge, edge, edge }));
    projectTriangles(t, glm::translate(center) *
        glm::rotate(glm::pi<float>() * (-0.5f - 2 * _cells[i][j].poster_side / 6.0f), glm::vec3(0, 0, 1)) *
        glm::translate(glm::vec3(0, -glm::sqrt(3.0f) / 2, 0.5f - float(h) / 2000)));
    meshes.index(MeshTypes::Posters + _cells[i][j].poster_type).addTriangles(std::move(t));
}

void Map::_cellDrop(int i, int j, MeshCollection& meshes) const
{
    if (!_cells[i][j].drop)
        return;
    glm::vec3 center = cent(i, j) + float(1 - DROP_SCALE) * v3;
    Triangles temp = _drop;
    projectTriangles(temp, glm::translate(center) * glm::scale(glm::vec3(DROP_SCALE, DROP_SCALE, DROP_SCALE)));
    meshes.index(MeshTypes::HonewDrop).addTriangles(std::move(temp));
}
