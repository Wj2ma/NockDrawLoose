#include <iostream>
#include <fstream>

#include <glm/ext.hpp>
#include <glm/gtx/io.hpp>

// #include "cs488-framework/ObjFileDecoder.hpp"
#include "Mesh.hpp"

using namespace std;
using namespace glm;

static const bool BOUNDING_VOLUME = true;

Mesh::Mesh( const std::string& fname )
	: m_vertices()
	, m_faces()
{
	std::string code;
	double vx, vy, vz;
	size_t s1, s2, s3;
	char dummySlash;
	int dummyV;

	std::ifstream ifs( ("Assets/" + fname + ".obj").c_str() );
	while( ifs >> code ) {
		if( code == "v" ) {
			ifs >> vx >> vy >> vz;
			m_vertices.push_back( glm::vec3( vx, vy, vz ) );
		} else if( code == "f" ) {
			ifs >> s1 >> dummySlash >> dummyV >> dummySlash >> dummyV
					>> s2 >> dummySlash >> dummyV >> dummySlash >> dummyV
					>> s3 >> dummySlash >> dummyV >> dummySlash >> dummyV;
			m_faces.push_back( Triangle( s1 - 1, s2 - 1, s3 - 1 ) );
		}
	}

	vec3 min = m_vertices[0];
	vec3 max = m_vertices[0];

	for (int i = 1; i < m_vertices.size(); ++i) {
		vec3 vertex = m_vertices[i];

		if (vertex[0] < min[0]) {
			min[0] = vertex[0];
		}
		if (vertex[1] < min[1]) {
			min[1] = vertex[1];
		}
		if (vertex[2] < min[2]) {
			min[2] = vertex[2];
		}
		if (vertex[0] > max[0]) {
			max[0] = vertex[0];
		}
		if (vertex[1] > max[1]) {
			max[1] = vertex[1];
		}
		if (vertex[2] > max[2]) {
			max[2] = vertex[2];
		}
	}

	pos = min;
	maxPos = max;
}

std::ostream& operator<<(std::ostream& out, const Mesh& mesh)
{
  out << "mesh {";
  /*

  for( size_t idx = 0; idx < mesh.m_verts.size(); ++idx ) {
  	const MeshVertex& v = mesh.m_verts[idx];
  	out << glm::to_string( v.m_position );
	if( mesh.m_have_norm ) {
  	  out << " / " << glm::to_string( v.m_normal );
	}
	if( mesh.m_have_uv ) {
  	  out << " / " << glm::to_string( v.m_uv );
	}
  }

*/
  out << "}";
  return out;
}

float Mesh::intersects(vec3 lookFrom, vec3 direction) {
	if (BOUNDING_VOLUME) {
		double t = boxIntersection(pos, maxPos, lookFrom, direction);
		if (t < 0.0f) {
			return -1.0f;
		}
	}

	double minT = INT_MAX;
	bool hit = false;
	for (int i = 0; i < m_faces.size(); ++i) {
		Triangle triangle = m_faces[i];

		vec3 p0 = m_vertices[triangle.v1];
		vec3 p1 = m_vertices[triangle.v2];
		vec3 p2 = m_vertices[triangle.v3];

		vec3 v1 = p1 - p0;
		vec3 v2 = p2 - p0;
		vec3 dir = lookFrom - p0;

		float d = determinant(mat3(
			v1,
			v2,
			-direction
		));

		float d1 = determinant(mat3(
			dir,
			v2,
			-direction
		));

		float d2 = determinant(mat3(
			v1,
			dir,
			-direction
		));

		float d3 = determinant(mat3(
			v1,
			v2,
			dir
		));

		float beta = d1 / d;
		float gamma = d2 / d;
		float t = d3 / d;
		if (beta >= 0.0f && gamma >= 0.0f && beta + gamma <= 1.0f && t > 0.0f && t < minT) {
			hit = true;
			minT = t;
		}
	}

	return hit ? minT : -1.0f;
}

vec3 Mesh::updateNormal(vec3 point) {
	// Not needed.
	return normalize(point - pos);
}