#pragma once

#include "cs488-framework/CS488Window.hpp"
#include "cs488-framework/OpenGLImport.hpp"
#include "cs488-framework/ShaderProgram.hpp"
#include "MeshCombiner.hpp"

#include "Arrow.hpp"
#include "Bow.hpp"
#include "Target.hpp"
#include "SceneNode.hpp"
#include "GeometryNode.hpp"

#include <glm/glm.hpp>
#include <memory>
#include <map>
#include <chrono>
#include <AL/al.h>
#include <AL/alc.h>

struct LightSource {
	glm::vec3 position;
	glm::vec3 rgbIntensity;
};

struct Image {
	unsigned int width;
	unsigned int height;
	std::vector<unsigned char> data;
};

struct TargetData {
	glm::vec3 position;
	glm::vec3 rotation;
	float cycleTime;
	glm::vec3 deltaPosition;

	TargetData(glm::vec3 pos = glm::vec3(), glm::vec3 r = glm::vec3(), float cT = 0.0f,
		glm::vec3 dPos = glm::vec3())
		: position(pos), rotation(r), cycleTime(cT), deltaPosition(dPos) { }
};

struct Sound {
	ALuint source;
	ALuint buffer;
};

class NockDrawLoose : public CS488Window {
public:
	NockDrawLoose(const std::string & luaSceneFile);
	virtual ~NockDrawLoose();

protected:
	virtual void init() override;
	virtual void appLogic() override;
	virtual void guiLogic() override;
	virtual void draw() override;
	virtual void cleanup() override;

	//-- Virtual callback methods
	virtual bool cursorEnterWindowEvent(int entered) override;
	virtual bool mouseMoveEvent(double xPos, double yPos) override;
	virtual bool mouseButtonInputEvent(int button, int actions, int mods) override;
	virtual bool mouseScrollEvent(double xOffSet, double yOffSet) override;
	virtual bool windowResizeEvent(int width, int height) override;
	virtual bool keyInputEvent(int key, int action, int mods) override;

	bool mainMenuKeyInput(int key);
	bool menuKeyInput(int key);
	bool gameOverKeyInput(int key);
	bool gameKeyInput(int key);

	//-- One time initialization methods:
	void processLuaSceneFile(const std::string & filename);
	void processNodes(SceneNode &node);
	void processSecret();
	void createShaderProgram();
	void initSkybox();
	void initViewMatrix();
	void initLightSources();
	void initShadowBuffer();
	void initRound(int round);
	void initPerspectiveMatrix();
	void initSound();
	void uploadVertexDataToVbos(const MeshCombiner & meshCombiner);

	bool loadSound(const char* filename, Sound& sound);

	void reset();
	void enableVertexShaderInputSlots();
	void enableShadowInputSlots();
	void mapVboDataToVertexShaderInputLocations();
	void mapVboDataToShadowInputLocations();

	void uploadCommonSceneUniforms();
	void updateShootingArrows(float elapsedTime);
	void updateTargets(float elapsedTime);
	void renderShadowGraph(const SceneNode &root);
	void renderSceneGraph(const SceneNode &root);
	void traverseShadow(const SceneNode &root);
	void traverse(const SceneNode &root);
	void drawLine();
	void drawSkybox();
	void drawPuddle();

	bool maybeRotateScene(float fNewX, float fNewY, float fOldX, float fOldY);

	double prev_x_pos;
	double prev_y_pos;
	bool l_mouse_pressed;
	bool r_mouse_pressed;

	std::chrono::high_resolution_clock::time_point prevTime;
	float totalTime;
	float roundStart;
	std::vector<float> roundTimes;
	int arrowsShot;
	int bullseyes;

	glm::mat4 m_perspective;
	glm::mat4 m_view;
	glm::mat4 m_shadow_perspective;
	glm::mat4 m_shadow_view;

	LightSource m_light;

	//-- GL resources for mesh geometry data:
	GLuint m_vao_meshData;
	GLuint m_vbo_vertexPositions;
	GLuint m_vbo_vertexNormals;
	GLuint m_vbo_vertexTextures;
	GLuint m_depthTexture;
	GLint m_positionAttribLocation;
	GLint m_normalAttribLocation;
	GLint m_textureAttribLocation;
	GLint m_shadowPositionLocation;

	ShaderProgram m_shader;
	ShaderProgram m_shadow;
	ShaderProgram m_quad;
	ShaderProgram m_line;
	ShaderProgram m_skybox;
	ShaderProgram m_reflection;

	GLuint shadowBuffer;

	GLuint m_skyboxTexture;
	GLuint m_skybox_vao;
	GLuint m_skybox_vbo;
	GLuint m_skybox_pos;

	// BatchInfoMap is an associative container that maps a unique MeshId to a BatchInfo
	// object. Each BatchInfo object contains an index offset and the number of indices
	// required to render the mesh with identifier MeshId.
	BatchInfoMap m_batchInfoMap;

	std::string m_luaSceneFile;

	std::shared_ptr<SceneNode> m_rootNode;
	Bow* bow;
	std::list<Arrow*> activeArrows;
	std::list<Target*> activeTargets;
	GeometryNode* targetNode;
	SceneNode* obstacles;
	std::shared_ptr<GeometryNode> puddle;

	PhysicsData* physics;

	GLuint m_line_vao;
	GLuint m_line_vbo;
	GLuint m_line_pos;

	bool perspectiveChange;
	bool viewChange;
	bool devMode;
	bool rightKeyPress;
	bool leftKeyPress;
	bool upKeyPress;
	bool downKeyPress;
	bool moveLeft;
	bool moveRight;
	bool shadows;
	bool textures;
	bool transparency;
	bool hideCursorOnly;
	bool showLine;
	bool instantArrow;

	float sensitivityX;
	float sensitivityY;

	std::map<std::string, Image> images;

	int currRound;

	ALCdevice *device;
	ALCcontext *context;

	Sound drawSound;
	Sound shootSound;
	Sound hitTargetSound;
	Sound hitGroundSound;
	Sound hitCrateSound;
	Sound bullseyeSound;
	Sound outdoorsSound;
};
