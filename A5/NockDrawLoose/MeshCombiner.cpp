#include "MeshCombiner.hpp"
using namespace glm;
using namespace std;

#include "cs488-framework/Exception.hpp"
#include "cs488-framework/ObjFileDecoder.hpp"

#include <iostream>


//----------------------------------------------------------------------------------------
// Default constructor
MeshCombiner::MeshCombiner()
{

}

//----------------------------------------------------------------------------------------
// Destructor
MeshCombiner::~MeshCombiner()
{

}

//----------------------------------------------------------------------------------------
template <typename T>
static void appendVector (
		std::vector<T> & dest,
		const std::vector<T> & source
) {
	// Increase capacity to hold source.size() more elements
	dest.reserve(dest.size() + source.size());

	dest.insert(dest.end(), source.begin(), source.end());
}


//----------------------------------------------------------------------------------------
MeshCombiner::MeshCombiner(
		std::initializer_list<ObjFilePath> objFileList
) {

	MeshId meshId;
	vector<vec3> positions;
	vector<vec3> normals;
	vector<vec2> textures;
	BatchInfo batchInfo;
	unsigned long indexOffset(0);

    for(const ObjFilePath & objFile : objFileList) {
	    ObjFileDecoder::decode(objFile.c_str(), meshId, positions, normals, textures);

	    uint numIndices = positions.size();
	    if (numIndices != normals.size() || numIndices != textures.size()) {
		    throw Exception("Error within MeshCombiner: "
					"positions.size() != normals.size()\n");
	    }

	    batchInfo.startIndex = indexOffset;
	    batchInfo.numIndices = numIndices;

	    m_batchInfoMap[meshId] = batchInfo;

	    appendVector(m_vertexPositionData, positions);
	    appendVector(m_vertexNormalData, normals);
	    appendVector(m_vertexTextureData, textures);

	    indexOffset += numIndices;
    }

}

//----------------------------------------------------------------------------------------
void MeshCombiner::getBatchInfoMap (
		BatchInfoMap & batchInfoMap
) const {
	batchInfoMap = m_batchInfoMap;
}

//----------------------------------------------------------------------------------------
// Returns the starting memory location for vertex position data.
const float * MeshCombiner::getVertexPositionDataPtr() const {
	return &(m_vertexPositionData[0].x);
}

//----------------------------------------------------------------------------------------
// Returns the starting memory location for vertex normal data.
const float * MeshCombiner::getVertexNormalDataPtr() const {
    return &(m_vertexNormalData[0].x);
}

//----------------------------------------------------------------------------------------
// Returns the starting memory location for vertex texture data.
const float * MeshCombiner::getVertexTextureDataPtr() const {
    return &(m_vertexTextureData[0].x);
}

//----------------------------------------------------------------------------------------
// Returns the total number of bytes of all vertex position data.
size_t MeshCombiner::getNumVertexPositionBytes() const {
	return m_vertexPositionData.size() * sizeof(vec3);
}

//----------------------------------------------------------------------------------------
// Returns the total number of bytes of all vertex normal data.
size_t MeshCombiner::getNumVertexNormalBytes() const {
	return m_vertexNormalData.size() * sizeof(vec3);
}

//----------------------------------------------------------------------------------------
// Returns the total number of bytes of all vertex texture data.
size_t MeshCombiner::getNumVertexTextureBytes() const {
	return m_vertexTextureData.size() * sizeof(vec2);
}