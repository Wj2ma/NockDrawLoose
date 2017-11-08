#pragma once

#include "SceneNode.hpp"
#include "TextureImage.hpp"
#include "Primitive.hpp"

class GeometryNode : public SceneNode {
public:
	GeometryNode(
		const std::string & meshId,
		const std::string & name,
		Primitive *prim
	);

	GeometryNode(const GeometryNode & other);

	Material material;
	Primitive* m_primitive;

	TextureImage image;

	// Mesh Identifier. This must correspond to an object name of
	// a loaded .obj file.
	std::string meshId;
};
