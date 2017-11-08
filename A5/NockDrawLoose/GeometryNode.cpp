#include "GeometryNode.hpp"

//---------------------------------------------------------------------------------------
GeometryNode::GeometryNode(
		const std::string & meshId,
		const std::string & name,
		Primitive *prim
)
	: SceneNode(name),
	  meshId(meshId),
	  m_primitive(prim)
{
	m_nodeType = NodeType::GeometryNode;
}

GeometryNode::GeometryNode(const GeometryNode & other) : SceneNode(other) {
	meshId = other.meshId;
	material.kd = other.material.kd;
	material.ks = other.material.ks;
	material.shininess = other.material.shininess;
	image = other.image;
	m_primitive = other.m_primitive;
}