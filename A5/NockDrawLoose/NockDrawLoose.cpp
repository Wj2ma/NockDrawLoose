#include "NockDrawLoose.hpp"
#include "scene_lua.hpp"
using namespace std;

#include "cs488-framework/GlErrorCheck.hpp"
#include "cs488-framework/MathUtils.hpp"
#include "lodepng/lodepng.h"
#include "GeometryNode.hpp"
#include "trackball.hpp"
#include "globals.hpp"
#include "TextureImage.hpp"

#include <imgui/imgui.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <sstream>

#include <stack>

using namespace glm;

static bool show_menu = false;
static bool game_over = false;
static bool main_menu = true;
static bool instructions = false;

static stack<mat4> transformations;

static GLuint texBufferId;

const static int SECRET_KEYS[] = { GLFW_KEY_I,
	GLFW_KEY_H, GLFW_KEY_A, GLFW_KEY_T, GLFW_KEY_E };
const static int SECRET_SIZE = 5;
static bool secretMode = false;
static int secret = 0;
static TextureImage secretTextures[SECRET_SIZE];

//----------------------------------------------------------------------------------------
// Constructor
NockDrawLoose::NockDrawLoose(const std::string & luaSceneFile)
	: m_luaSceneFile(luaSceneFile),
	  m_positionAttribLocation(0),
	  m_normalAttribLocation(0),
	  m_vao_meshData(0),
	  m_vbo_vertexPositions(0),
	  m_vbo_vertexNormals(0),
	  m_vbo_vertexTextures(0),
	  m_depthTexture(0),
	  m_textureAttribLocation(0),
	  m_shadowPositionLocation(0),
	  shadowBuffer(0),
	  m_skyboxTexture(0),
	  m_skybox_vao(0),
	  m_skybox_vbo(0),
	  m_skybox_pos(0),
	  prev_x_pos(0),
	  prev_y_pos(0),
	  l_mouse_pressed(false),
	  r_mouse_pressed(false),
	  prevTime(chrono::high_resolution_clock::now()),
	  totalTime(0.0f),
	  roundStart(0.0f),
	  arrowsShot(0),
		bullseyes(0),
	  m_line_vao(0),
	  m_line_vbo(0),
	  perspectiveChange(false),
	  viewChange(false),
	  devMode(false),
	  rightKeyPress(false),
	  leftKeyPress(false),
	  upKeyPress(false),
	  downKeyPress(false),
	  moveLeft(false),
	  moveRight(false),
	  shadows(true),
	  textures(true),
	  transparency(true),
	  hideCursorOnly(false),
	  instantArrow(false),
	  showLine(false),
	  sensitivityY(DEFAULT_SENSITIVITY_Y),
	  sensitivityX(DEFAULT_SENSITIVITY_X),
	  currRound(1)
{
	physics = new PhysicsData();
	physics->bowMass = DEFAULT_BOW_MASS;
	physics->arrowMass = DEFAULT_ARROW_MASS;
	physics->k = DEFAULT_K;
	physics->gravity = DEFAULT_GRAVITY;
}

//----------------------------------------------------------------------------------------
// Destructor
NockDrawLoose::~NockDrawLoose()
{
	delete bow;

	for (Arrow* arrow : activeArrows) {
		delete arrow;
	}

	for (Target* target : activeTargets) {
		m_rootNode->children.remove(target->node);
		delete target;
	}

	delete physics;

	// Apologize for the bad code.
	alDeleteSources(1, &drawSound.source);
	alDeleteBuffers(1, &drawSound.buffer);
	alDeleteSources(1, &shootSound.source);
	alDeleteBuffers(1, &shootSound.buffer);
	alDeleteSources(1, &hitTargetSound.source);
	alDeleteBuffers(1, &hitTargetSound.buffer);
	alDeleteSources(1, &hitGroundSound.source);
	alDeleteBuffers(1, &hitGroundSound.buffer);
	alDeleteSources(1, &hitCrateSound.source);
	alDeleteBuffers(1, &hitCrateSound.buffer);
	alDeleteSources(1, &bullseyeSound.source);
	alDeleteBuffers(1, &bullseyeSound.buffer);
	alDeleteSources(1, &outdoorsSound.source);
	alDeleteBuffers(1, &outdoorsSound.buffer);

	alcMakeContextCurrent(NULL);
	alcDestroyContext(context);
	alcCloseDevice(device);
}

//----------------------------------------------------------------------------------------
/*
 * Called once, at program start.
 */
void NockDrawLoose::init()
{
	// Set the background colour.
	glClearColor(0.35, 0.35, 0.35, 1.0);

	createShaderProgram();

	glGenVertexArrays(1, &m_vao_meshData);

	processLuaSceneFile(m_luaSceneFile);

	processNodes(*m_rootNode);
	processSecret();
	// Erase the bow to render last.
	m_rootNode->children.remove(bow->node);
	// Erase the target to copy.
	m_rootNode->children.remove(targetNode);

	// Load and decode all .obj files at once here.  You may add additional .obj files to
	// this list in order to support rendering additional mesh types.  All vertex
	// positions, and normals will be extracted and stored within the MeshCombiner
	// class.
	unique_ptr<MeshCombiner> meshCombiner (new MeshCombiner{
			getAssetFilePath("cube.obj"),
			getAssetFilePath("target.obj"),
			getAssetFilePath("bow.obj"),
			getAssetFilePath("grass.obj"),
			getAssetFilePath("arrow.obj"),
			getAssetFilePath("crate.obj"),
			getAssetFilePath("puddle.obj")
	});

	// Acquire the BatchInfoMap from the MeshCombiner.
	meshCombiner->getBatchInfoMap(m_batchInfoMap);

	// Take all vertex data within the MeshCombiner and upload it to VBOs on the GPU.
	uploadVertexDataToVbos(*meshCombiner);

	initSkybox();

	initPerspectiveMatrix();

	initLightSources();

	initViewMatrix();

	initShadowBuffer();

	initSound();

	glGenVertexArrays(1, &m_line_vao);
	glGenBuffers(1, &m_line_vbo);
	glBindVertexArray(m_line_vao);
	m_line_pos = m_line.getAttribLocation("position");
	glEnableVertexAttribArray(m_line_pos);
	glBindVertexArray(0);
	CHECK_GL_ERRORS;

	m_line.enable();
	GLint pv = m_line.getUniformLocation("pv");
	glUniformMatrix4fv(pv, 1, GL_FALSE, value_ptr(m_perspective * m_view));
	CHECK_GL_ERRORS;
	m_line.disable();

	// Exiting the current scope calls delete automatically on meshCombiner freeing
	// all vertex data resources.  This is fine since we already copied this data to
	// VBOs on the GPU.  We have no use for storing vertex data on the CPU side beyond
	// this point.
}

void NockDrawLoose::initSound() {
	device = alcOpenDevice(NULL);
	context = alcCreateContext(device, NULL);
	alcMakeContextCurrent(context);

	ALfloat listenerPos[] = { 0.0f, 0.0f, 0.0f };
	ALfloat listenerVel[] = { 0.0f, 0.0f, 0.0f };
	ALfloat listenerOri[] = { 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f };
	alListenerfv(AL_POSITION, listenerPos);
	alListenerfv(AL_VELOCITY, listenerVel);
	alListenerfv(AL_ORIENTATION, listenerOri);

	if (!loadSound("Assets/draw.wav", drawSound)) {
		cout << "Unable to load draw sound." << endl;
	}
	if (!loadSound("Assets/shoot.wav", shootSound)) {
		cout << "Unable to load shoot sound." << endl;
	}
	if (!loadSound("Assets/hitTarget.wav", hitTargetSound)) {
		cout << "Unable to load hitting target sound." << endl;
	}
	if (!loadSound("Assets/hitGround.wav", hitGroundSound)) {
		cout << "Unable to load hitting ground sound." << endl;
	}
	if (!loadSound("Assets/hitCrate.wav", hitCrateSound)) {
		cout << "Unable to load hitting crate sound." << endl;
	}
	if (!loadSound("Assets/bullseye.wav", bullseyeSound)) {
		cout << "Unable to load bullseye sound." << endl;
	}
	if (!loadSound("Assets/outdoors.wav", outdoorsSound)) {
		cout << "Unable to load outdoors sound." << endl;
	}

	alSourcei(outdoorsSound.source, AL_LOOPING, AL_TRUE);
	alSourcef(outdoorsSound.source, AL_GAIN, 0.05f);
}

// Copied from https://www.youtube.com/watch?v=V83Ja4FmrqE to load wav file.
bool NockDrawLoose::loadSound(const char* filename, Sound& sound) {
	FILE *fp = fopen(filename, "rb");
	char type[4];
	unsigned int size, chunkSize;
	short formatType, channels;
	unsigned int sampleRate, avgBytesPerSec;
	short bytesPerSample, bitsPerSample;
	unsigned int dataSize;

	fread(type, sizeof(char), 4, fp);
	if (type[0] != 'R' || type[1] != 'I' || type[2] != 'F' || type[3] != 'F') {
		return false;
	}

	fread(&size, sizeof(unsigned int), 1, fp);
	fread(type, sizeof(char), 4, fp);
	if (type[0] != 'W' || type[1] != 'A' || type[2] != 'V' || type[3] != 'E') {
		return false;
	}

	fread(type, sizeof(char), 4, fp);
	if (type[0] != 'f' || type[1] != 'm' || type[2] != 't' || type[3] != ' ') {
		return false;
	}

	fread(&chunkSize, sizeof(unsigned int), 1, fp);
	fread(&formatType, sizeof(short), 1, fp);
	fread(&channels, sizeof(short), 1, fp);
	fread(&sampleRate, sizeof(unsigned int), 1, fp);
	fread(&avgBytesPerSec, sizeof(unsigned int), 1, fp);
	fread(&bytesPerSample, sizeof(short), 1, fp);
	fread(&bitsPerSample, sizeof(short), 1, fp);

	fread(type, sizeof(char), 4, fp);
	if (type[0] != 'd' || type[1] != 'a' || type[2] != 't' || type[3] != 'a') {
		return false;
	}

	fread(&dataSize, sizeof(unsigned int), 1, fp);
	unsigned char* soundData = new unsigned char[dataSize];
	fread(soundData, sizeof(unsigned char), dataSize, fp);

	ALenum format;
	if (bitsPerSample == 8) {
		if (channels == 1) {
			format = AL_FORMAT_MONO8;
		} else if (channels == 2) {
			format = AL_FORMAT_STEREO8;
		}
	} else if (bitsPerSample == 16) {
		if (channels == 1) {
			format = AL_FORMAT_MONO16;
		} else if (channels == 2) {
			format = AL_FORMAT_STEREO16;
		}
	}

	alGenSources((ALuint) 1, &sound.source);

	alGenBuffers((ALuint) 1, &sound.buffer);
	alBufferData(sound.buffer, format, soundData, dataSize, sampleRate);

	ALfloat sourcePos[] = { 0.0f, 0.0f, 0.0f };
	ALfloat sourceVel[] = { 0.0f, 0.0f, 0.0f };

	alSourcei(sound.source, AL_BUFFER, sound.buffer);
	alSourcef(sound.source, AL_PITCH, 1.0f);
	alSourcef(sound.source, AL_GAIN, 1.0f);
	alSourcefv(sound.source, AL_POSITION, sourcePos);
	alSourcefv(sound.source, AL_VELOCITY, sourceVel);
	alSourcei(sound.source, AL_LOOPING, AL_FALSE);

	delete [] soundData;
	fclose(fp);

	return true;
}

void NockDrawLoose::reset() {
	// Delete all targets.
	auto targetIt = activeTargets.begin();
	while (targetIt != activeTargets.end()) {
		m_rootNode->children.remove((*targetIt)->node);
		delete *targetIt;
		targetIt = activeTargets.erase(targetIt);
	}

	// Delete all active arrows.
	auto arrowIt = activeArrows.begin();
	while (arrowIt != activeArrows.end()) {
		delete *arrowIt;
		arrowIt = activeArrows.erase(arrowIt);
	}

	// Clear the environment.
	auto items = m_rootNode->children;
	for (auto it = m_rootNode->children.begin(); it != m_rootNode->children.end();) {
		SceneNode* node = *it;
		if (node->m_nodeType == NodeType::GeometryNode) {
			it = m_rootNode->children.erase(it);
			delete node;
		} else {
			++it;
		}
	}

	currRound = 1;
	totalTime = 0.0f;
	roundStart = 0.0f;
	roundTimes.clear();
	arrowsShot = 0;
	bullseyes = 0;
	initRound(currRound);
	alSourcePlay(outdoorsSound.source);
}

//----------------------------------------------------------------------------------------
void NockDrawLoose::processLuaSceneFile(const std::string & filename) {
	// This version of the code treats the Lua file as an Asset,
	// so that you'd launch the program with just the filename
	// of a puppet in the Assets/ directory.
	// std::string assetFilePath = getAssetFilePath(filename.c_str());
	// m_rootNode = std::shared_ptr<SceneNode>(import_lua(assetFilePath));

	// This version of the code treats the main program argument
	// as a straightforward pathname.
	m_rootNode = std::shared_ptr<SceneNode>(import_lua(filename));
	if (!m_rootNode) {
		std::cerr << "Could not open " << filename << std::endl;
	}

	puddle = std::shared_ptr<GeometryNode>(static_cast<GeometryNode*>(import_lua("Assets/puddle.lua")));
	if (!puddle) {
		std::cerr << "Could not open puddle.lua" << std::endl;
	}
}

void NockDrawLoose::processNodes(SceneNode &node) {
	node.setInit();

	if (node.m_name == "obstacles") {
		obstacles = &node;
	}

	if (node.m_nodeType == NodeType::GeometryNode) {
		GeometryNode &geometryNode = static_cast<GeometryNode&>(node);

		if (geometryNode.m_name == "bow") {
			bow = new Bow(&geometryNode, vec3(0.0f, 0.0f, -1.0f));
		} else if (geometryNode.m_name == "target") {
			targetNode = &geometryNode;
		}

		auto imageIt = images.find(geometryNode.meshId);
		Image image;
		if (imageIt == images.end()) {
		  unsigned error = lodepng::decode(image.data, image.width, image.height,
		  	"Assets/" + geometryNode.meshId + ".png");

		  if (error != 0)
		  {
		    std::cout << "error " << error << ": " << lodepng_error_text(error) << std::endl;
		  }

		  images[geometryNode.meshId] = image;
		} else {
			image = imageIt->second;
		}

		glGenTextures(1, &geometryNode.image.texBufferId);
		glBindTexture(GL_TEXTURE_2D, geometryNode.image.texBufferId);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0, GL_RGBA,
			GL_UNSIGNED_BYTE, &(image.data.front()));
		CHECK_GL_ERRORS;
	}

	for (SceneNode *child : node.children) {
		processNodes(*child);
	}
}

void NockDrawLoose::processSecret() {
	for (int i = 0; i < SECRET_SIZE; ++i) {
		TextureImage image;
		stringstream name;
		name << "secret" << i;
	  unsigned error = lodepng::decode(image.data, image.width, image.height,
	  	"Assets/" + name.str() + ".png");

	  if (error != 0)
	  {
	    std::cout << "error " << error << ": " << lodepng_error_text(error) << std::endl;
	  }

	  glGenTextures(1, &image.texBufferId);
	  glBindTexture(GL_TEXTURE_2D, image.texBufferId);

	  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0, GL_RGBA,
			GL_UNSIGNED_BYTE, &(image.data.front()));

		secretTextures[i] = image;

		CHECK_GL_ERRORS;
	}
}

//----------------------------------------------------------------------------------------
void NockDrawLoose::createShaderProgram()
{
	m_shader.generateProgramObject();
	m_shader.attachVertexShader( getAssetFilePath("VertexShader.vs").c_str() );
	m_shader.attachFragmentShader( getAssetFilePath("FragmentShader.fs").c_str() );
	m_shader.link();

	m_shadow.generateProgramObject();
	m_shadow.attachVertexShader( getAssetFilePath("ShadowVertexShader.vs").c_str() );
	m_shadow.attachFragmentShader( getAssetFilePath("ShadowFragmentShader.fs").c_str() );
	m_shadow.link();

	m_quad.generateProgramObject();
	m_quad.attachVertexShader( getAssetFilePath("quadVertexShader.vs").c_str() );
	m_quad.attachFragmentShader( getAssetFilePath("quadFragmentShader.fs").c_str() );
	m_quad.link();

	m_line.generateProgramObject();
	m_line.attachVertexShader( getAssetFilePath("LineVertexShader.vs").c_str() );
	m_line.attachFragmentShader( getAssetFilePath("LineFragmentShader.fs").c_str() );
	m_line.link();

	m_skybox.generateProgramObject();
	m_skybox.attachVertexShader( getAssetFilePath("SkyboxVertexShader.vs").c_str() );
	m_skybox.attachFragmentShader( getAssetFilePath("SkyboxFragmentShader.fs").c_str() );
	m_skybox.link();

	m_reflection.generateProgramObject();
	m_reflection.attachVertexShader( getAssetFilePath("ReflectionVertexShader.vs").c_str() );
	m_reflection.attachFragmentShader( getAssetFilePath("ReflectionFragmentShader.fs").c_str() );
	m_reflection.link();
}

//----------------------------------------------------------------------------------------
void NockDrawLoose::enableVertexShaderInputSlots()
{
	//-- Enable input slots for m_vao_meshData:
	{
		glBindVertexArray(m_vao_meshData);

		// Enable the vertex shader attribute location for "position" when rendering.
		m_positionAttribLocation = m_shader.getAttribLocation("position");
		glEnableVertexAttribArray(m_positionAttribLocation);

		// // Enable the vertex shader attribute location for "normal" when rendering.
		m_normalAttribLocation = m_shader.getAttribLocation("normal");
		glEnableVertexAttribArray(m_normalAttribLocation);

		m_textureAttribLocation = m_shader.getAttribLocation("uv");
		glEnableVertexAttribArray(m_textureAttribLocation);

		CHECK_GL_ERRORS;
	}

	// Restore defaults
	glBindVertexArray(0);
}

void NockDrawLoose::enableShadowInputSlots() {
	{
		glBindVertexArray(m_vao_meshData);
		m_shadowPositionLocation = m_shadow.getAttribLocation("position");
		glEnableVertexAttribArray(m_shadowPositionLocation);

		CHECK_GL_ERRORS;
	}

	glBindVertexArray(0);
}

//----------------------------------------------------------------------------------------
void NockDrawLoose::uploadVertexDataToVbos (
		const MeshCombiner & meshCombiner
) {
	// Generate VBO to store all vertex position data
	{
		glGenBuffers(1, &m_vbo_vertexPositions);

		glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertexPositions);

		glBufferData(GL_ARRAY_BUFFER, meshCombiner.getNumVertexPositionBytes(),
				meshCombiner.getVertexPositionDataPtr(), GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		CHECK_GL_ERRORS;
	}

	// Generate VBO to store all vertex normal data
	{
		glGenBuffers(1, &m_vbo_vertexNormals);

		glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertexNormals);

		glBufferData(GL_ARRAY_BUFFER, meshCombiner.getNumVertexNormalBytes(),
				meshCombiner.getVertexNormalDataPtr(), GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		CHECK_GL_ERRORS;
	}

	// Generate VBO to store all vertex texture data
	{
		glGenBuffers(1, &m_vbo_vertexTextures);

		glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertexTextures);

		glBufferData(GL_ARRAY_BUFFER, meshCombiner.getNumVertexTextureBytes(),
			meshCombiner.getVertexTextureDataPtr(), GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		CHECK_GL_ERRORS;
	}
}

void NockDrawLoose::mapVboDataToShadowInputLocations() {
	glBindVertexArray(m_vao_meshData);

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertexPositions);
	glVertexAttribPointer(m_shadowPositionLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	CHECK_GL_ERRORS;
}

//----------------------------------------------------------------------------------------
void NockDrawLoose::mapVboDataToVertexShaderInputLocations()
{
	// Bind VAO in order to record the data mapping.
	glBindVertexArray(m_vao_meshData);

	// Tell GL how to map data from the vertex buffer "m_vbo_vertexPositions" into the
	// "position" vertex attribute location for any bound vertex shader program.
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertexPositions);
	glVertexAttribPointer(m_positionAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Tell GL how to map data from the vertex buffer "m_vbo_vertexNormals" into the
	// "normal" vertex attribute location for any bound vertex shader program.
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertexNormals);
	glVertexAttribPointer(m_normalAttribLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Tell GL how to map data from the vertex buffer "m_vbo_vertexTextures" into the
	// "texture" vertex attribute location for any bound vertex shader program.
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertexTextures);
	glVertexAttribPointer(m_textureAttribLocation, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	//-- Unbind target, and restore default values:
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	CHECK_GL_ERRORS;
}

void NockDrawLoose::initSkybox() {
	glGenTextures(1, &m_skyboxTexture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_skyboxTexture);

	string skyboxTextures[] = {
		"Assets/TropicalSunnyDayRight.png",
		"Assets/TropicalSunnyDayLeft.png",
		"Assets/TropicalSunnyDayUp.png",
		"Assets/TropicalSunnyDayDown.png",
		"Assets/TropicalSunnyDayBack.png",
		"Assets/TropicalSunnyDayFront.png"
	};

	for (int i = 0; i < 6; ++i) {
		Image image;
		unsigned int error = lodepng::decode(image.data, image.width, image.height,
			  	skyboxTextures[i]);

	  if (error != 0) {
	    std::cout << "error " << error << ": " << lodepng_error_text(error) << std::endl;
	  }
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, image.width,
			image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &(image.data.front()));
		CHECK_GL_ERRORS;
	}

	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	float skyboxVertices[] = {
    // positions
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
  };

  glGenVertexArrays(1, &m_skybox_vao);
  glBindVertexArray(m_skybox_vao);
  glGenBuffers(1, &m_skybox_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, m_skybox_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
  m_skybox_pos = m_skybox.getAttribLocation("position");
  glEnableVertexAttribArray(m_skybox_pos);
  glVertexAttribPointer(m_skybox_pos, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

//----------------------------------------------------------------------------------------
void NockDrawLoose::initPerspectiveMatrix()
{
	float aspect = ((float)m_windowWidth) / m_windowHeight;
	m_perspective = glm::perspective(degreesToRadians(60.0f), aspect, 0.1f, 100.0f);
	m_shadow_perspective = glm::perspective(degreesToRadians(60.0f), aspect, 0.1f, 100.0f);
	//m_shadow_perspective = ortho<float>(-15.0f, 15.0f, -15.0f, 15.0f, 1.0f, 30.0f);
//	m_perspective = m_shadow_perspective;
}


//----------------------------------------------------------------------------------------
void NockDrawLoose::initViewMatrix() {
	m_view = glm::lookAt(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f),
			vec3(0.0f, 1.0f, 0.0f));
	m_shadow_view = lookAt(m_light.position, vec3(0.0f, 0.0f, -5.0f), vec3(0.0f, 1.0f, 0.0f));
//	m_view = m_shadow_view;
}

//----------------------------------------------------------------------------------------
void NockDrawLoose::initLightSources() {
	// World-space position
	m_light.position = vec3(5.0f, 12.0f, 5.0f);
	m_light.rgbIntensity = vec3(1.0f); // White light
}

void NockDrawLoose::initShadowBuffer() {
	// Taken from http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping/
  glGenFramebuffers(1, &shadowBuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, shadowBuffer);

  glGenTextures(1, &m_depthTexture);
  glBindTexture(GL_TEXTURE_2D, m_depthTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 1024, 768, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

 	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_depthTexture, 0);

  glDrawBuffer(GL_NONE); // No color buffer is drawn to.
  glReadBuffer(GL_NONE);

  // Always check that our framebuffer is okay.
  if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      cout << "Frame buffer not okay" << endl;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, 0);
  CHECK_GL_ERRORS;
}

void NockDrawLoose::initRound(int currRound) {
	TargetData* targets;
	int length = 0;
	switch (currRound) {
		case 1:
			length = 5;
			targets = new TargetData[length] {
				TargetData(vec3(-3.0f, 0.0f, -8.0f)),
				TargetData(vec3(-2.5f, 2.0f, -13.5f)),
				TargetData(vec3(5.0f, 2.0f, -17.5f)),
				TargetData(vec3(1.5f, 0.0f, -7.0f)),
				TargetData(vec3(5.0f, 1.0f, -10.5f))
			};
			break;
		case 2:
			length = 5;
			targets = new TargetData[length] {
				TargetData(vec3(-5.5f, 1.0f, -13.0f), vec3(0.0f, 0.0f, 1.0f)),
				TargetData(vec3(-4.5f, 2.0f, -13.5f)),
				TargetData(vec3(0.0f, 0.0f, -20.0f)),
				TargetData(vec3(4.0f, 0.0f, -10.0f), vec3(), 8000.0f, vec3(-4.0f, 0.0f, 0.0f)),
				TargetData(vec3(10.0f, 0.0f, -15.0f))
			};
			break;
		case 3:
			length = 5;
			targets = new TargetData[length] {
				TargetData(vec3(2.0, 5.0f, -17.5f)),
				TargetData(vec3(3.0f, 4.0f, -17.5f)),
				TargetData(vec3(4.0f, 3.0f, -17.5f)),
				TargetData(vec3(5.0f, 2.0f, -17.5f)),
				TargetData(vec3(6.0f, 1.0f, -17.5f))
			};
			break;
		case 4:
			length = 5;
			targets = new TargetData[length] {
				TargetData(vec3(-10.0f, 0.0f, -18.0f)),
				TargetData(vec3(-5.0f, 2.0f, -13.5f), vec3(), 3000.0f, vec3(3.0f, 0.0f, 0.0f)),
				TargetData(vec3(1.5f, 4.5f, -17.5f), vec3(0.0f, 0.0f, 1.0f), 4000.0f, vec3(0.0f, -4.0f, 0.0f)),
				TargetData(vec3(4.0f, 0.0f, -11.0f), vec3(), 2000.0f, vec3(2.0f, 0.0f, 0.0f)),
				TargetData(vec3(10.0f, 0.0f, -18.0f))
			};
			break;
		case 5:
			length = 5;
			targets = new TargetData[length] {
				TargetData(vec3(-15.0f, 0.0f, -25.0f)),
				TargetData(vec3(-5.0f, 2.0f, -13.5f), vec3(), 1500.0f, vec3(3.0f, 0.0f, 0.0f)),
				TargetData(vec3(0.0f, 0.0f, -17.5f), vec3(), 2000.0f, vec3(4.0f, 0.0f, 0.0f)),
				TargetData(vec3(4.0f, 3.0f, -17.5f), vec3(), 3000.0f, vec3(0.0f, -3.0f, 0.0f)),
				TargetData(vec3(15.0f, 0.0f, -25.0f))
			};
			break;
	}

	for (int i = 0; i < length; ++i) {
		TargetData data = targets[i];
		GeometryNode* newTarget = new GeometryNode(*targetNode);
		Target* target = new Target(newTarget, data.position, hitTargetSound.source,
			hitGroundSound.source, bullseyeSound.source, data.rotation, data.cycleTime,
			data.deltaPosition);
		if (secretMode) {
			newTarget->image = secretTextures[i];
		}
		m_rootNode->children.push_back(newTarget);
		activeTargets.push_back(target);
	}

	delete [] targets;
}

//----------------------------------------------------------------------------------------
void NockDrawLoose::uploadCommonSceneUniforms() {
	m_shader.enable();
	{
		//-- Set perspective matrix uniform for the scene:
		GLint location = m_shader.getUniformLocation("Perspective");
		glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(m_perspective));
		CHECK_GL_ERRORS;


		//-- Set LightSource uniform for the scene:
		{
			location = m_shader.getUniformLocation("light.position");
			glUniform3fv(location, 1, value_ptr(m_light.position));
			location = m_shader.getUniformLocation("light.rgbIntensity");
			glUniform3fv(location, 1, value_ptr(m_light.rgbIntensity));
			CHECK_GL_ERRORS;
		}

		//-- Set background light ambient intensity
		{
			location = m_shader.getUniformLocation("ambientIntensity");
			vec3 ambientIntensity(0.05f);
			glUniform3fv(location, 1, value_ptr(ambientIntensity));
			CHECK_GL_ERRORS;
		}
	}
	m_shader.disable();
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, before guiLogic().
 */
void NockDrawLoose::appLogic()
{
	// Place per frame, application logic here ...
	uploadCommonSceneUniforms();

	chrono::high_resolution_clock::time_point now = chrono::high_resolution_clock::now();
	float elapsedTime = ((chrono::duration<double, milli>)(now - prevTime)).count();
	prevTime = now;

	if (rightKeyPress) {
		m_view = rotate(
			degreesToRadians(elapsedTime * 360.0f / 3000.0f), vec3(0.0f, 1.0f, 0.0f)) * m_view;
	}
	if (leftKeyPress) {
		m_view = rotate(
			degreesToRadians(elapsedTime * 360.0f / 3000.0f), vec3(0.0f, -1.0f, 0.0f)) * m_view;
	}
	if (upKeyPress) {
		m_view = rotate(
			degreesToRadians(elapsedTime * 360.0f / 3000.0f), vec3(-1.0f, 0.0f, 0.0f)) * m_view;
	}
	if (downKeyPress) {
		m_view = rotate(
			degreesToRadians(elapsedTime * 360.0f / 3000.0f), vec3(1.0f, 0.0f, 0.0f)) * m_view;
	}

	if (moveLeft) {
		m_view = translate(vec3(elapsedTime * 100.0f / 3000.0f, 0.0f, 0.0f)) * m_view;
	}
	if (moveRight) {
		m_view = translate(vec3(-elapsedTime * 100.0f / 3000.0f, 0.0f, 0.0f)) * m_view;
	}

	if (!show_menu && !game_over && !main_menu) {
		totalTime += elapsedTime / 1000.0f;

		if (l_mouse_pressed) {
			bow->pull(elapsedTime);
		}

		updateShootingArrows(elapsedTime);
		updateTargets(elapsedTime);
	}
}

void NockDrawLoose::updateShootingArrows(float elapsedTime) {
	if (instantArrow) {
		elapsedTime += 100000.0f;
	}

	// Update shooting arrow locations and erase them once they pass a certain depth.
	for (auto it = activeArrows.begin(); it != activeArrows.end();) {
		Arrow* shootingArrow = *it;
		GeometryNode* arrowNode = shootingArrow->node;

		// If elapsedTime is greater than max time interval, then update at each max interval.
		float totalTime = elapsedTime;
		do {
			float time = std::min(totalTime, MAX_TIME_INTERVAL);
			totalTime -= time;
			vec3 direction = shootingArrow->nextFrame(time);
			vec3 normalizedDir = normalize(direction);
			vec3 arrowHead = vec3(arrowNode->get_transform()[3])
				+ MIDDLE_ARROWHEAD_DISTANCE * normalizedDir;

			float t;
			float maxT;

			// Intersection with obstacles.
			for (SceneNode* obstacle : obstacles->children) {
				GeometryNode* node = static_cast<GeometryNode*>(obstacle);
				mat4 invTransform = inverse(node->get_transform());
				vec3 localArrowHead = vec3(invTransform * vec4(arrowHead, 1.0f));
				vec3 localDir = vec3(invTransform * vec4(direction, 0.0f));
				// Reverse the direction since the arrow position is at the end of the line.
				vec3 normalizedLocalDir = normalize(localDir);
				t = node->m_primitive->intersects(localArrowHead, normalizedLocalDir);
				maxT = length(localDir);
				// Intersection iff the it's in between the line it travelled this frame.
				if (t >= 0.0f && t <= maxT) {
					// Translate back to where it should be intersected.
					arrowNode->translate(
						vec3(node->get_transform() * vec4(t * normalizedLocalDir, 0.0f)));
					vec3 position = vec3(arrowNode->get_transform()[3]);
					// Position for sound. Scale down by 2 because it is too quiet.
					float pos[] = { position[0], position[1], position[2] };
					alSourcefv(hitCrateSound.source, AL_POSITION, pos);
					alSourcePlay(hitCrateSound.source);

					it = activeArrows.erase(it);
					delete shootingArrow;
					goto next;
				}
			}

			// Intersection with targets.
			for (Target* target : activeTargets) {
				if (target->intersects(arrowNode, arrowHead, direction, bullseyes)) {
					// Stick arrow as child of target node.
					GeometryNode* targetNode = target->node;
					m_rootNode->children.remove(arrowNode);
					targetNode->children.push_back(arrowNode);
					// Multiply by inverse target transform.
					arrowNode->set_transform(
						inverse(targetNode->get_transform()) * arrowNode->get_transform());

					it = activeArrows.erase(it);
					delete shootingArrow;
					goto next;
				}
			}

			// Intersection with the ground.
			t = normalizedDir.y != 0.0f ?
				(GROUND_LEVEL - arrowHead.y) / normalizedDir.y : -1.0f;
			maxT = length(direction);
			if (t >= 0.0f && t <= maxT || arrowHead.y <= GROUND_LEVEL) {
				arrowNode->translate(t * normalizedDir);
				vec3 position = vec3(arrowNode->get_transform()[3]);
				// Position for sound. Scale down by 2 because it is too quiet.
				float pos[] = { position[0], position[1], position[2] };
				alSourcefv(hitGroundSound.source, AL_POSITION, pos);
				alSourcePlay(hitGroundSound.source);
				it = activeArrows.erase(it);
				delete shootingArrow;
				goto next;
			}

			arrowNode->translate(direction);

			// Arrow went too far.
			if (arrowHead.z <= -30.0f) {
				m_rootNode->children.remove(arrowNode);
				it = activeArrows.erase(it);
				delete arrowNode;
				delete shootingArrow;
				goto next;
			}
		} while (totalTime > 0.0f);

		++it;
	next:;
	}
}

void NockDrawLoose::updateTargets(float elapsedTime) {
	// Update active targets and erase them once they are hit and finished translated.
	for (auto it = activeTargets.begin(); it != activeTargets.end();) {
		Target* target = *it;
		target->nextFrame(elapsedTime);

		if (target->shouldDelete && !target->inTransition) {
			it = activeTargets.erase(it);
			m_rootNode->children.remove(target->node);
			delete target;
		} else {
			++it;
		}
	}

	if (activeTargets.size() == 0) {
		roundTimes.push_back(totalTime - roundStart);
		roundStart = totalTime;

		if (currRound < 5) {
			initRound(++currRound);
		} else {
			alSourceStop(outdoorsSound.source);
			alSourceStop(drawSound.source);
			game_over = true;
			glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	}
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after appLogic(), but before the draw() method.
 */
void NockDrawLoose::guiLogic()
{
	static bool showTime(true);

	if (main_menu) {
		ImGui::SetNextWindowSize(ImVec2(400, 510), ImGuiSetCond_FirstUseEver);
		ImGui::OpenPopup("Main Menu");
		if (ImGui::BeginPopupModal("Main Menu",
			NULL, ImGuiWindowFlags_NoResize)) {

			ImVec2 size(200, 40);

			if (instructions) {
				ImGui::TextWrapped((string("The goal of the game is to try to hit all of the ")
					+ string("targets as fast as possible. Use the mouse to aim the bow, hold ")
					+ string("click to draw back the bow, and release to fire an arrow.")).c_str());
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
				ImGui::Text("Developer Record: 46.38 s");
				ImGui::Separator();
				ImGui::Text("Round 1: 5.42 s");
				ImGui::Text("Round 2: 9.95 s");
				ImGui::Text("Round 3: 6.40 s");
				ImGui::Text("Round 4: 8.64 s");
				ImGui::Text("Round 5: 15.96 s");
				ImGui::Text("Least Arrows Fired: 35");
				ImGui::Text("Most Bullseyes: 4");

				ImGui::SetCursorPosX(100);
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
				if(ImGui::Button("Return (R)", size)) {
					instructions = false;
				}
			} else {
				ImGui::SetCursorPosX(100);
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
				if(ImGui::Button("Start Game (S)", size)) {
					main_menu = false;
					reset();
					glfwSetInputMode(m_window, GLFW_CURSOR,
						hideCursorOnly ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_DISABLED);
					ImGui::CloseCurrentPopup();
				}

				ImGui::SetCursorPosX(100);
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
				if(ImGui::Button("Instructions (I)", size)) {
					instructions = true;
				}

				ImGui::SetCursorPosX(100);
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
				if (ImGui::Button("Exit (Q)", size)) {
					glfwSetWindowShouldClose(m_window, GL_TRUE);
				}
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);

				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
				ImGui::Text("Game Settings:");
				ImGui::Separator();
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
				ImGui::SetCursorPosX(40);
				ImGui::DragFloat("Bow Mass", &physics->bowMass, 0.005f, 0.5f, 100.0f);
				ImGui::SetCursorPosX(40);
				ImGui::DragFloat("Arrow Mass", &physics->arrowMass, 0.005f, 0.01f, 100.0f);
				ImGui::SetCursorPosX(40);
				ImGui::DragFloat("K Constant", &physics->k, 10.0f, 1000.0f, 10000.0f);
				ImGui::SetCursorPosX(40);
				ImGui::DragFloat("Gravity", &physics->gravity, 1.0f, 1.0f, 500.0f);

				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
				ImGui::Text("Sensitivity:");
				ImGui::SetCursorPosX(40);
				ImGui::DragFloat("X", &sensitivityX, 0.005f, 0.1f, 5.0f);
				ImGui::SetCursorPosX(40);
				ImGui::DragFloat("Y", &sensitivityY, 0.005f, 0.1f, 5.0f);

				ImGui::SetCursorPosX(100);
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
				if (ImGui::Button("Reset Defaults (R)", ImVec2(200, 20))) {
					physics->bowMass = DEFAULT_BOW_MASS;
					physics->arrowMass = DEFAULT_ARROW_MASS;
					physics->k = DEFAULT_K;
					physics->gravity = DEFAULT_GRAVITY;
					sensitivityX = DEFAULT_SENSITIVITY_X;
					sensitivityY = DEFAULT_SENSITIVITY_Y;
				}
			}

      ImGui::EndPopup();
		}
	} else {
		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar
		| ImGuiWindowFlags_NoCollapse;
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::Begin("Elapsed Time", &showTime, ImVec2(120, 20), 0.5f, windowFlags);
		ImGui::Text("Time: %.2f s", totalTime);
		ImGui::End();

		if (devMode) {
			ImGui::SetNextWindowPos(ImVec2(909, 0));
			ImGui::Begin("Dev Mode", &devMode, ImVec2(115, 20), 0.5f, windowFlags);
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "DEVELOPER MODE");
			ImGui::End();
		}
	}

	if (show_menu) {
		ImGui::SetNextWindowSize(ImVec2(300, 420), ImGuiSetCond_FirstUseEver);
		ImGui::OpenPopup("Menu");
		if (ImGui::BeginPopupModal("Menu",
			NULL, ImGuiWindowFlags_NoResize)) {
			ImVec2 size(200, 40);

			ImGui::SetCursorPosX(50);
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
			if (ImGui::Button("Restart (R)", size)) {
				show_menu = false;
				reset();
				glfwSetInputMode(m_window, GLFW_CURSOR,
					hideCursorOnly ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_DISABLED);
				ImGui::CloseCurrentPopup();
			}

			ImGui::SetCursorPosX(50);
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
			if (ImGui::Button("Main Menu (M)", size)) {
				alSourceStop(outdoorsSound.source);
				main_menu = true;
				show_menu = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::SetCursorPosX(50);
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
			if(ImGui::Button("Exit Game (Q)", size)) {
				glfwSetWindowShouldClose(m_window, GL_TRUE);
			}

			ImGui::SetCursorPosX(50);
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
			if (ImGui::Button("Return (ESC)", size)) {
				alSourcePlay(outdoorsSound.source);
				show_menu = false;
				glfwSetInputMode(m_window, GLFW_CURSOR,
					hideCursorOnly ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_DISABLED);
				ImGui::CloseCurrentPopup();
			}

			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 40);
			ImGui::Text("Sensitivity:");
			ImGui::Separator();
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
			ImGui::SetCursorPosX(50);
			ImGui::DragFloat("X", &sensitivityX, 0.005f, 0.1f, 5.0f);
			ImGui::SetCursorPosX(50);
			ImGui::DragFloat("Y", &sensitivityY, 0.005f, 0.1f, 5.0f);
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
      ImGui::EndPopup();
    }
	}

	if (game_over) {
		ImGui::SetNextWindowSize(ImVec2(400, 330), ImGuiSetCond_FirstUseEver);
		ImGui::OpenPopup("Game Over");
		if (ImGui::BeginPopupModal("Game Over",
			NULL, ImGuiWindowFlags_NoResize)) {
      ImGui::Text("Congratulations, you finished in %.2f seconds!!", totalTime);
      ImGui::Separator();

      ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
      for (int i = 1; i <= roundTimes.size(); ++i) {
      	ImGui::SetCursorPosX(145);
      	ImGui::Text("Round %d: %.2f s", i, roundTimes[i - 1]);
      }

      ImGui::SetCursorPosX(145);
      ImGui::Text("Arrows fired: %d", arrowsShot);

      ImGui::SetCursorPosX(145);
      ImGui::Text("Bullseyes: %d", bullseyes);

      ImGui::SetCursorPosX(125);
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
			if (ImGui::Button("Restart (R)", ImVec2(150, 30))) {
				game_over = false;
				reset();
				glfwSetInputMode(m_window, GLFW_CURSOR,
					hideCursorOnly ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_DISABLED);
				ImGui::CloseCurrentPopup();
			}

			ImGui::SetCursorPosX(125);
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
			if (ImGui::Button("Main Menu (M)", ImVec2(150, 30))) {
				game_over = false;
				main_menu = true;
				ImGui::CloseCurrentPopup();
			}

			ImGui::SetCursorPosX(125);
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
			if(ImGui::Button("Exit Game (Q)", ImVec2(150, 30))) {
				glfwSetWindowShouldClose(m_window, GL_TRUE);
			}
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
      ImGui::EndPopup();
    }
	}
}

//----------------------------------------------------------------------------------------
// Update mesh specific shader uniforms:
static void updateShaderUniforms(
		const ShaderProgram &shader,
		const GeometryNode &node,
		const mat4 &viewMatrix,
		const mat4 &depthPVMatrix,
		const GLuint depthTextureId
) {
	shader.enable();
	{
		// Shadow variables.
		mat4 depthMVP = depthPVMatrix * transformations.top() * node.trans;
		GLint depthMVPId = shader.getUniformLocation("depthMVP");
		glUniformMatrix4fv(depthMVPId, 1, GL_FALSE, value_ptr(depthMVP));

		// //-- Set ModelView matrix:
		GLint location = shader.getUniformLocation("ModelView");
		mat4 modelView = viewMatrix * transformations.top() * node.trans;
		glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(modelView));
		CHECK_GL_ERRORS;

		// //-- Set NormMatrix:
		location = shader.getUniformLocation("NormalMatrix");
		mat3 normalMatrix = glm::transpose(glm::inverse(mat3(modelView)));
		glUniformMatrix3fv(location, 1, GL_FALSE, value_ptr(normalMatrix));
		CHECK_GL_ERRORS;

		//-- Set Material values:
		location = shader.getUniformLocation("material.kd");
		vec3 kd = node.material.kd;
		glUniform3fv(location, 1, value_ptr(kd));
		location = shader.getUniformLocation("material.ks");
		vec3 ks = node.material.ks;
		glUniform3fv(location, 1, value_ptr(ks));
		location = shader.getUniformLocation("material.shininess");
		glUniform1f(location, node.material.shininess);
		CHECK_GL_ERRORS;

		// Set Texture values
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, node.image.texBufferId);
		location = shader.getUniformLocation("textureSampler");
		glUniform1i(location, 0);
		CHECK_GL_ERRORS;

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depthTextureId);
		location = shader.getUniformLocation("shadowMap");
		glUniform1i(location, 1);
		CHECK_GL_ERRORS;
	}
	shader.disable();

}

static void updateShadowUniforms(
	const ShaderProgram &shader,
	const GeometryNode &node,
	const mat4 &depthPVMatrix) {
	// Taken from https://github.com/opengl-tutorials/ogl/blob/master/tutorial16_shadowmaps/tutorial16_SimpleVersion.cpp
	shader.enable();
	{
		// Compute the ModelView matrix from the light's point of view.
		mat4 depthMvp = depthPVMatrix * transformations.top() * node.trans;

		// Send our transformation to the currently bound shader in the "ModelView" uniform.
		GLint location = shader.getUniformLocation("mvp");
		glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(depthMvp));

		CHECK_GL_ERRORS;
	}
	shader.disable();
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after guiLogic().
 */
void NockDrawLoose::draw() {
	GLint disableShadows = m_shader.getUniformLocation("disableShadows");
	GLint reflectiveDisableShadows = m_reflection.getUniformLocation("disableShadows");
	if (shadows) {
		m_shader.enable();
		glUniform1i(disableShadows, 0);
		CHECK_GL_ERRORS;
		m_shader.disable();

		m_reflection.enable();
		glUniform1i(reflectiveDisableShadows, 0);
		CHECK_GL_ERRORS;
		m_reflection.disable();

		renderShadowGraph(*m_rootNode);
	} else {
		m_shader.enable();
		glUniform1i(disableShadows, 1);
		CHECK_GL_ERRORS;
		m_shader.disable();

		m_reflection.enable();
		glUniform1i(reflectiveDisableShadows, 1);
		CHECK_GL_ERRORS;
		m_reflection.disable();
	}

	GLint disableTextures = m_shader.getUniformLocation("disableTextures");
	GLint disableTransparency = m_shader.getUniformLocation("disableTransparency");
	m_shader.enable();
	if (textures) {
		glUniform1i(disableTextures, 0);
	} else {
		glUniform1i(disableTextures, 1);
	}
	if (transparency) {
		glUniform1i(disableTransparency, 0);
	} else {
		glUniform1i(disableTransparency, 1);
	}
	CHECK_GL_ERRORS;
	m_shader.disable();

	renderSceneGraph(*m_rootNode);

	if (showLine) {
		drawLine();
	}

	drawSkybox();
	drawPuddle();
}

void NockDrawLoose::renderShadowGraph(const SceneNode &root) {
	glBindFramebuffer(GL_FRAMEBUFFER, shadowBuffer);
	glViewport(0, 0, 1024, 768);	// Render on the whole framebuffer, complete from the lower left corner to the upper right.

	enableShadowInputSlots();
	mapVboDataToShadowInputLocations();

	// Bind the VAO once here, and reuse for all GeometryNode rendering below.
	glBindVertexArray(m_vao_meshData);

	// We don't use bias in the shader, but instead we draw back faces, which are already
	// separated from the front faces by a small distance (if your geometry is made this way)
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT); // Cull back-facing triangles -> draw only front-facing triangles

	// Clear the screen.
	glClear(GL_DEPTH_BUFFER_BIT);

	// Render the objects.
	transformations.push(root.get_transform());
	traverseShadow(root);
	transformations.pop();

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	glBindVertexArray(0);

	// Go back to normal scene.
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	CHECK_GL_ERRORS;
}

void NockDrawLoose::traverseShadow(const SceneNode &root) {
	for (const SceneNode *node : root.children) {
		if (node->m_nodeType == NodeType::GeometryNode) {
			const GeometryNode *geometryNode = static_cast<const GeometryNode*>(node);
			updateShadowUniforms(m_shadow, *geometryNode, m_shadow_perspective * m_shadow_view);

			// Get the BatchInfo corresponding to the GeometryNode's unique MeshId.
			BatchInfo batchInfo = m_batchInfoMap[geometryNode->meshId];

			// Now render the mesh.
			m_shadow.enable();
			glDrawArrays(GL_TRIANGLES, batchInfo.startIndex, batchInfo.numIndices);
			m_shadow.disable();
		}

		transformations.push(transformations.top() * node->get_transform());
		traverseShadow(*node);
		transformations.pop();
	}
}

//----------------------------------------------------------------------------------------
void NockDrawLoose::renderSceneGraph(const SceneNode &root) {
	glViewport(0, 0, m_framebufferWidth, m_framebufferHeight);

	enableVertexShaderInputSlots();
	mapVboDataToVertexShaderInputLocations();

	// Bind the VAO once here, and reuse for all GeometryNode rendering below.
	glBindVertexArray(m_vao_meshData);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	transformations.push(root.get_transform());
	traverse(root);
	// Render the bow last for transparency to work.
	transformations.push(transformations.top() * bow->node->get_transform());
	traverse(*bow->node);
	transformations.pop();
	updateShaderUniforms(m_shader, *bow->node, m_view,
		m_shadow_perspective * m_shadow_view, m_depthTexture);
	BatchInfo batchInfo = m_batchInfoMap[bow->node->meshId];
	m_shader.enable();
	glDrawArrays(GL_TRIANGLES, batchInfo.startIndex, batchInfo.numIndices);
	m_shader.disable();
	transformations.pop();

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	glBindVertexArray(0);
	CHECK_GL_ERRORS;
}

void NockDrawLoose::traverse(const SceneNode &root) {
	for (const SceneNode *node : root.children) {
		transformations.push(transformations.top() * node->get_transform());
		traverse(*node);
		transformations.pop();

		if (node->m_nodeType == NodeType::GeometryNode) {
			const GeometryNode *geometryNode = static_cast<const GeometryNode*>(node);

			updateShaderUniforms(m_shader, *geometryNode, m_view,
				m_shadow_perspective * m_shadow_view, m_depthTexture);

			// Get the BatchInfo corresponding to the GeometryNode's unique MeshId.
			BatchInfo batchInfo = m_batchInfoMap[geometryNode->meshId];

			//-- Now render the mesh:
			m_shader.enable();
			glDrawArrays(GL_TRIANGLES, batchInfo.startIndex, batchInfo.numIndices);
			m_shader.disable();
		}
	}
}

void NockDrawLoose::drawLine() {
	vec3 bowPos = vec3(bow->node->trans[3]);
	vec3 line[2] = { bowPos, 5.0f * bow->direction + bowPos };

	m_line.enable();
	glBindVertexArray(m_line_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_line_vbo);
	glBufferData(GL_ARRAY_BUFFER, 2 * sizeof(vec3), line, GL_STATIC_DRAW);
	CHECK_GL_ERRORS;
	glVertexAttribPointer(m_line_pos, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
	CHECK_GL_ERRORS;
	glDrawArrays(GL_LINES, 0, 2);
	CHECK_GL_ERRORS;
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	CHECK_GL_ERRORS;
	m_line.disable();
}

void NockDrawLoose::drawSkybox() {
	m_skybox.enable();
	glBindVertexArray(m_skybox_vao);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	mat4 view = mat4(mat3(m_view));
	view[3].y = -0.11f;
	GLint location = m_skybox.getUniformLocation("pv");
	glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(m_perspective * view));

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_skyboxTexture);
	location = m_skybox.getUniformLocation("skybox");
	glUniform1i(location, 3);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	CHECK_GL_ERRORS;
	m_skybox.disable();
	glDepthFunc(GL_LESS);
	glDisable(GL_DEPTH_TEST);
}

void NockDrawLoose::drawPuddle() {
	glBindVertexArray(m_vao_meshData);
  GLuint pos = m_reflection.getAttribLocation("originalPos");
  glEnableVertexAttribArray(pos);
  GLuint norm = m_reflection.getAttribLocation("originalNorm");
  glEnableVertexAttribArray(norm);
  glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertexPositions);
  glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
  glBindBuffer(GL_ARRAY_BUFFER, m_vbo_vertexNormals);
  glVertexAttribPointer(norm, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
  CHECK_GL_ERRORS;

  m_reflection.enable();
  glEnable(GL_DEPTH_TEST);
  GLint location = m_reflection.getUniformLocation("depthMVP");
  glUniformMatrix4fv(location, 1, GL_FALSE,
  	value_ptr(m_shadow_perspective * m_shadow_view * puddle->get_transform()));
	location = m_reflection.getUniformLocation("model");
	glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(puddle->get_transform()));
	location = m_reflection.getUniformLocation("pv");
	glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(m_perspective * m_view));
	location = m_reflection.getUniformLocation("cameraPos");
	// Complicated way to get camera position in world space. https://gamedev.stackexchange.com/questions/22283/how-to-get-translation-from-view-matrix
	glUniform3fv(location, 1, value_ptr(-transpose(mat3(m_view)) * vec3(m_view[3])));
	location = m_reflection.getUniformLocation("diffuse");
	glUniform3fv(location, 1, value_ptr(vec3(0.6f, 0.86098f, 0.91765f)));
	CHECK_GL_ERRORS;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_skyboxTexture);
	location = m_reflection.getUniformLocation("skybox");
	glUniform1i(location, 3);
	CHECK_GL_ERRORS;

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_depthTexture);
	location = m_reflection.getUniformLocation("shadowMap");
	glUniform1i(location, 1);
	CHECK_GL_ERRORS;

	BatchInfo batchInfo = m_batchInfoMap[puddle->meshId];
	glDrawArrays(GL_TRIANGLES, batchInfo.startIndex, batchInfo.numIndices);
	glDisable(GL_DEPTH_TEST);
	glBindVertexArray(0);
	CHECK_GL_ERRORS;
	m_reflection.disable();
}

bool NockDrawLoose::maybeRotateScene(float fNewX, float fNewY, float fOldX, float fOldY) {
	float h = m_framebufferHeight / 2.0;
	float w = m_framebufferWidth / 2.0;
	vec3 rotateAxis = vCalcRotVec(
		fNewX - w,
		-(fNewY - h),
		fOldX - w,
		-(fOldY - h),
		std::min(w, h)
	);
	m_rootNode->rotate(vAxisRotMatrix(rotateAxis[0], rotateAxis[1], rotateAxis[2]));
	return true;
}

//----------------------------------------------------------------------------------------
/*
 * Called once, after program is signaled to terminate.
 */
void NockDrawLoose::cleanup()
{

}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles cursor entering the window area events.
 */
bool NockDrawLoose::cursorEnterWindowEvent (
		int entered
) {
	bool eventHandled(false);

	// Fill in with event handling code...

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse cursor movement events.
 */
bool NockDrawLoose::mouseMoveEvent (
		double xPos,
		double yPos
) {
	bool eventHandled(false);

	if (!ImGui::IsMouseHoveringAnyWindow()) {
		float deltaX = xPos - prev_x_pos;
		float deltaY = yPos - prev_y_pos;

		if (r_mouse_pressed) {
			eventHandled |= maybeRotateScene(xPos, yPos, prev_x_pos, prev_y_pos);
		}

		if (!show_menu && !game_over && !main_menu) {
			if (hideCursorOnly) {
				bow->updateDirection(vec3(
					xPos * (MAX_X - MIN_X) / m_framebufferWidth + MIN_X,
					(m_framebufferHeight - yPos) * (MAX_Y - MIN_Y) / m_framebufferHeight + MIN_Y,
					-1.0f
				));
			} else {
				bow->updateDirection(
					deltaX * sensitivityX / m_framebufferWidth,
					-deltaY * sensitivityY / m_framebufferHeight
				);
			}
		}

		prev_x_pos = xPos;
		prev_y_pos = yPos;
	}

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse button events.
 */
bool NockDrawLoose::mouseButtonInputEvent (
		int button,
		int actions,
		int mods
) {
	bool eventHandled(false);

	if (!ImGui::IsMouseHoveringAnyWindow()) {
		if (actions == GLFW_PRESS) {
			glfwGetCursorPos(m_window, &prev_x_pos, &prev_y_pos);
			if (button == GLFW_MOUSE_BUTTON_LEFT && !show_menu && !game_over && !main_menu) {
				// Drawing back bow.
				alSourcePlay(drawSound.source);
				l_mouse_pressed = true;
				eventHandled = true;
			}
			// if (button == GLFW_MOUSE_BUTTON_RIGHT) {
			// 	r_mouse_pressed = true;
			// 	eventHandled = true;
			// }
		} else if (actions == GLFW_RELEASE) {
			if (button == GLFW_MOUSE_BUTTON_LEFT && !show_menu && !game_over && !main_menu) {
				// Shooting arrow.
				alSourceStop(drawSound.source);
				alSourcePlay(shootSound.source);
				++arrowsShot;
				Arrow* shootingArrow = bow->shoot(physics);
				m_rootNode->children.push_back(shootingArrow->node);
				activeArrows.push_back(shootingArrow);

				l_mouse_pressed = false;
				eventHandled = true;
			}
			// if (button == GLFW_MOUSE_BUTTON_RIGHT) {
			// 	r_mouse_pressed = false;
			// 	eventHandled = true;
			// }
		}
	}

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse scroll wheel events.
 */
bool NockDrawLoose::mouseScrollEvent (
		double xOffSet,
		double yOffSet
) {
	bool eventHandled(false);

	// Fill in with event handling code...

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles window resize events.
 */
bool NockDrawLoose::windowResizeEvent (
		int width,
		int height
) {
	bool eventHandled(false);
	initPerspectiveMatrix();
	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles key input events.
 */
bool NockDrawLoose::keyInputEvent (
		int key,
		int action,
		int mods
) {
	bool eventHandled(false);

	if (action == GLFW_PRESS) {
		 eventHandled = mainMenuKeyInput(key)
			 || menuKeyInput(key)
			 || gameOverKeyInput(key)
			 || gameKeyInput(key);
	} else if (action == GLFW_RELEASE) {
		if (key == GLFW_KEY_LEFT) {
			leftKeyPress = false;
			eventHandled = true;
		} else if (key == GLFW_KEY_RIGHT) {
			rightKeyPress = false;
			eventHandled = true;
		} else if (key == GLFW_KEY_UP) {
			upKeyPress = false;
			eventHandled = true;
		} else if (key == GLFW_KEY_DOWN) {
			downKeyPress = false;
			eventHandled = true;
		} else if (key == GLFW_KEY_Z) {
			moveLeft = false;
			eventHandled = true;
		} else if (key == GLFW_KEY_C) {
			moveRight = false;
			eventHandled = true;
		}
	}

	return eventHandled;
}

bool NockDrawLoose::mainMenuKeyInput(int key) {
	if (main_menu) {
		if (!instructions) {
			if (key == GLFW_KEY_Q) {
				glfwSetWindowShouldClose(m_window, GL_TRUE);
				return true;
			} else if (key == GLFW_KEY_S) {
				main_menu = false;
				reset();
				glfwSetInputMode(m_window, GLFW_CURSOR,
					hideCursorOnly ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_DISABLED);
				return true;
			} else if (key == GLFW_KEY_R) {
				physics->bowMass = DEFAULT_BOW_MASS;
				physics->arrowMass = DEFAULT_ARROW_MASS;
				physics->k = DEFAULT_K;
				physics->gravity = DEFAULT_GRAVITY;
				sensitivityX = DEFAULT_SENSITIVITY_X;
				sensitivityY = DEFAULT_SENSITIVITY_Y;
				return true;
			} else if (key == GLFW_KEY_I) {
				instructions = true;
				return true;
			}
		} else if (key == GLFW_KEY_R) {
			instructions = false;
		}
	}
	return false;
}

bool NockDrawLoose::menuKeyInput(int key) {
	if (show_menu) {
		if (key == GLFW_KEY_Q) {
			glfwSetWindowShouldClose(m_window, GL_TRUE);
			return true;
		} else if (key == GLFW_KEY_R) {
			show_menu = false;
			reset();
			glfwSetInputMode(m_window, GLFW_CURSOR,
				hideCursorOnly ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_DISABLED);
			return true;
		} else if (key == GLFW_KEY_M) {
			alSourceStop(outdoorsSound.source);
			show_menu = false;
			main_menu = true;
		} else if (key == GLFW_KEY_ESCAPE) {
			alSourcePlay(outdoorsSound.source);
			show_menu = false;
			glfwSetInputMode(m_window, GLFW_CURSOR,
				hideCursorOnly ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_DISABLED);
			return true;
		}
	}
	return false;
}

bool NockDrawLoose::gameOverKeyInput(int key) {
	if (game_over) {
		if (key == GLFW_KEY_Q) {
			game_over = false;
			glfwSetWindowShouldClose(m_window, GL_TRUE);
			return true;
		} else if (key == GLFW_KEY_R) {
			game_over = false;
			reset();
			glfwSetInputMode(m_window, GLFW_CURSOR,
				hideCursorOnly ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_DISABLED);
			return true;
		} else if (key == GLFW_KEY_M) {
			game_over = false;
			main_menu = true;
		}
	}
	return false;
}

bool NockDrawLoose::gameKeyInput(int key) {
	if (!main_menu && !show_menu && !game_over) {
		if (key == GLFW_KEY_ESCAPE) {
			alSourcePause(outdoorsSound.source);
			show_menu = true;
			glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			return true;
		} else if (key == GLFW_KEY_D) {
			devMode = !devMode;
			m_view = glm::lookAt(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f),
						vec3(0.0f, 1.0f, 0.0f));
			float aspect = ((float)m_windowWidth) / m_windowHeight;
					m_perspective = glm::perspective(degreesToRadians(60.0f), aspect, 0.1f, 100.0f);
					secret = 0;
			return true;
		}

		if (devMode) {
			if (key == SECRET_KEYS[secret]) {
				++secret;
				if (secret == SECRET_SIZE) {
					secretMode = !secretMode;
					reset();
					secret = 0;
				}
				return true;
			} else {
				secret = 0;
			}

			if (key == GLFW_KEY_P) {
				if (perspectiveChange) {
					float aspect = ((float)m_windowWidth) / m_windowHeight;
					m_perspective = glm::perspective(degreesToRadians(60.0f), aspect, 0.1f, 100.0f);
					m_view = glm::lookAt(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f),
						vec3(0.0f, 1.0f, 0.0f));
				} else {
					m_perspective = m_shadow_perspective;
					m_view = m_shadow_view;
				}
				perspectiveChange = !perspectiveChange;
				return true;
			} else if (key == GLFW_KEY_V) {
				if (viewChange) {
					m_view = lookAt(vec3(5.0f, 2.0f, 5.0f), vec3(5.0f, 2.0f, 0.0f),
					 vec3(0.0f, 1.0f, 0.0f));
				} else {
					m_view = glm::lookAt(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f),
						vec3(0.0f, 1.0f, 0.0f));
				}
				viewChange = !viewChange;
			} else if (key == GLFW_KEY_S) {
				shadows = !shadows;
				cout << "Shadows set to " << shadows << endl;
				return true;
			} else if (key == GLFW_KEY_T) {
				textures = !textures;
				cout << "Textures set to " << textures << endl;
				return true;
			} else if (key == GLFW_KEY_R) {
				transparency = !transparency;
				cout << "Transparency set to " << transparency << endl;
			}else if (key == GLFW_KEY_LEFT) {
				leftKeyPress = true;
				rightKeyPress = false;
				return true;
			} else if (key == GLFW_KEY_RIGHT) {
				rightKeyPress = true;
				leftKeyPress = false;
				return true;
			} else if (key == GLFW_KEY_UP) {
				upKeyPress = true;
				downKeyPress = false;
				return true;
			} else if (key == GLFW_KEY_DOWN) {
				downKeyPress = true;
				upKeyPress = false;
				return true;
			} else if (key == GLFW_KEY_Z) {
				moveLeft = true;
				moveRight = false;
				return true;
			} else if (key == GLFW_KEY_C) {
				moveLeft = false;
				moveRight = true;
				return true;
			} else if (key == GLFW_KEY_L) {
				showLine = !showLine;
				cout << "Show Line set to " << showLine << endl;
				return true;
			} else if (key == GLFW_KEY_B) {
				hideCursorOnly = !hideCursorOnly;
				cout << "Hide Cursor Only set to " << hideCursorOnly << endl;
				glfwSetInputMode(m_window, GLFW_CURSOR,
					hideCursorOnly ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_DISABLED);
				return true;
			} else if (key == GLFW_KEY_A) {
				instantArrow = !instantArrow;
				cout << "Instant arrow set to " << instantArrow << endl;
				return true;
			}
		}
	}
	return false;
}
