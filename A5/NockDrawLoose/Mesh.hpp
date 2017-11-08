#pragma once

#include <vector>
#include <iosfwd>
#include <string>

#include <glm/glm.hpp>

#include "Primitive.hpp"

struct Triangle
{
	size_t v1;
	size_t v2;
	size_t v3;

	Triangle( size_t pv1, size_t pv2, size_t pv3 )
		: v1( pv1 )
		, v2( pv2 )
		, v3( pv3 )
	{}
};

// A polygonal mesh.
class Mesh : public Primitive {
public:
  Mesh( const std::string& fname );
  float intersects(glm::vec3 lookFrom, glm::vec3 direction) override;
private:
	glm::vec3 updateNormal(glm::vec3 point);
	std::vector<glm::vec3> m_vertices;
	std::vector<Triangle> m_faces;
	glm::vec3 pos;
	glm::vec3 maxPos;

    friend std::ostream& operator<<(std::ostream& out, const Mesh& mesh);
};
