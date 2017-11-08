#include "SceneNode.hpp"
#include "GeometryNode.hpp"

#include "cs488-framework/MathUtils.hpp"

#include <iostream>
#include <sstream>
using namespace std;

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

using namespace glm;


// Static class variable
unsigned int SceneNode::nodeInstanceCount = 0;


//---------------------------------------------------------------------------------------
SceneNode::SceneNode(const std::string &name)
  : m_name(name),
	m_nodeType(NodeType::SceneNode),
	trans(mat4()),
	initTrans(mat4()),
	m_nodeId(nodeInstanceCount++)
{

}

//---------------------------------------------------------------------------------------
// Deep copy
SceneNode::SceneNode(const SceneNode &other) {
	m_nodeType = other.m_nodeType;
  m_name = other.m_name;
  trans = other.trans;
  invtrans = other.invtrans;
  m_nodeId = nodeInstanceCount++;
  initTrans = other.initTrans;
	for(SceneNode * child : other.children) {
		if (child->m_nodeType == NodeType::SceneNode) {
			this->children.push_front(new SceneNode(*child));
		} else if (child->m_nodeType == NodeType::GeometryNode) {
			this->children.push_front(new GeometryNode(*(static_cast<const GeometryNode *>(child))));
		}
	}
}

//---------------------------------------------------------------------------------------
SceneNode::~SceneNode() {
	for(SceneNode * child : children) {
		delete child;
	}
}

//---------------------------------------------------------------------------------------
void SceneNode::set_transform(const glm::mat4& m) {
	trans = m;
	invtrans = m;
}

//---------------------------------------------------------------------------------------
const glm::mat4& SceneNode::get_transform() const {
	return trans;
}

//---------------------------------------------------------------------------------------
const glm::mat4& SceneNode::get_inverse() const {
	return invtrans;
}

void SceneNode::setInit() {
	initTrans = trans;
}

void SceneNode::resetPosition() {
	trans[3][0] = initTrans[3][0];
	trans[3][1] = initTrans[3][1];
	trans[3][2] = initTrans[3][2];
}

void SceneNode::resetOrientation() {
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			trans[i][j] = initTrans[i][j];
		}
	}
}

const glm::mat4& SceneNode::get_initTrans() const {
	return initTrans;
}

//---------------------------------------------------------------------------------------
void SceneNode::add_child(SceneNode* child) {
	children.push_back(child);
}

//---------------------------------------------------------------------------------------
void SceneNode::remove_child(SceneNode* child) {
	children.remove(child);
}

//---------------------------------------------------------------------------------------
void SceneNode::rotate(char axis, float angle, bool radians) {
	if (!radians) {
		angle = degreesToRadians(angle);
	}

	vec3 rot_axis;

	switch (axis) {
		case 'x':
			rot_axis = vec3(1,0,0);
			break;
		case 'y':
			rot_axis = vec3(0,1,0);
	        break;
		case 'z':
			rot_axis = vec3(0,0,1);
	        break;
		default:
			break;
	}

	mat4 rot_matrix = glm::rotate(angle, rot_axis);
	trans = trans * rot_matrix;
}

// To rotate about a point, translate to that point, and then rotate, and then translate back to that point
void SceneNode::rotate(glm::mat4 rotMatrix) {
	trans = trans * rotMatrix;
//	* mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1.0, 1) * rotMatrix * mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, -1.0, 1);
}

//---------------------------------------------------------------------------------------
void SceneNode::scale(const glm::vec3 &amount) {
	trans = glm::scale(amount) * trans;
}

//---------------------------------------------------------------------------------------
void SceneNode::translate(const glm::vec3 &amount) {
	trans = glm::translate(amount) * trans;
}

//---------------------------------------------------------------------------------------
int SceneNode::totalSceneNodes() const {
	return nodeInstanceCount;
}

//---------------------------------------------------------------------------------------
std::ostream & operator << (std::ostream &os, const SceneNode &node) {

	//os << "SceneNode:[NodeType: ___, name: ____, id: ____, isSelected: ____, transform: ____"
	switch (node.m_nodeType) {
		case NodeType::SceneNode:
			os << "SceneNode";
			break;
		case NodeType::GeometryNode:
			os << "GeometryNode";
			break;
	}
	os << ":[";

	os << "name:" << node.m_name << ", ";
	os << "id:" << node.m_nodeId;
	os << "]";

	return os;
}
