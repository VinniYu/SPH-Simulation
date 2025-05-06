#ifndef PARTICLE_3D_H
#define PARTICLE_3D_H

// Code to run a 3D PCI-SPH simulation on the GPU.

#include "SETTINGS.h"
#include "SHADER.h"

using namespace std;

class Parallel {
public:

	Parallel(int numParticles);
	~Parallel();

	// initialization
	void initSimBounds();
	void initParticleAndPrograms();
	void initComputeShaders(vector<glm::vec4> positions, vector<glm::vec4> velocities);
	void initRenderer(const char* boundVertex, const char* boundFragment,
										const char* fluidVertex, const char* fluidFragment);
	
	// simulation										
	void render();
	void compute();
	void resetParticles();

	// move camera
	void rotateCamLeft();
	void rotateCamRight();

	

	// interaction
	enum DIR { LEFT, DOWN, RIGHT, UP };
	bool doObstacle = false;
	void initObject();
	void setObject(glm::vec3 pos) { objectCenter = pos; currentDirection = DOWN; };
	void moveObjectX(float deltaX);
	void moveObjectY(float deltaY);
	void loopObject();

private:
	// openGL screen
	int xScreenRes = 1440; 
	int yScreenRes = 900;

	vec3 camera = {0.0, 0.75, 4.95};
	vec3 origin = {0.0, 0.5, 0.0};
	vec3 up     = {0.0, 1.0, 0.0};
	float rot = 3.0f;

	// stretches for sim bounds, no stretch in y;
	float xStretch = 2.0;
	float zStretch = 1.0;
	vec3 boundsMin;
	vec3 boundsMax;

	// SSBOs
	GLuint posSSBO, velSSBO;
	GLuint predPosSSBO, predVelSSBO;
	GLuint densitySSBO, pressureSSBO;
	GLuint maxDensityError;

	// compute shader programs (in order)
	GLuint progApplyExtForces; 
	GLuint progComputeDensities;
	GLuint progApplyPressures;
	GLuint progApplyViscosity;
	GLuint progResolveCollisions;

	// renderer
	GLuint boundRenderer, fluidRenderer, objectRenderer;
	GLuint boundVAO, boundVBO;
	GLuint particleVAO, particleVBO;
	GLuint sphereIndexCount = 0;
	GLuint objectVAO, objectVBO; 

	// obstacle
	float size = 0.75f;
	glm::vec3 objectCenter = glm::vec3(-1.0f, 1.0f, 0.0f);
	DIR currentDirection = DOWN;

	// 3d sim parameters
	int numParticles;
	int maxIterations = 8;
	float dt = 0.002;
	float gravity = 60.0;
	float restDensity = 9900;
	float smoothingRadius = 0.12;
	float stiffness = 0.0055;
	float eta = 0.01;
	float viscosityStrength = 0.00009;
};


#endif