#ifndef PARTICLE_2D_H
#define PARTICLE_2D_H

// Code to run a 2D PCI-SPH simulation on the GPU.

#include "SETTINGS.h"
#include "SHADER.h"

using namespace std;

// Particle structure
class Parallel
{
public:
	Parallel(int numParticles);
	~Parallel();

	// initialization
	void initParticlesAndProgram();
	void initComputeShaders(float *positions, float *velocities);
	void initRenderer(const char *vertexPath, const char *fragmentPath);

	// simulation
	void render();
	void compute();

	// interaction functions
	void injectForce(float x, float y, int pressed, int sign) { mouseX = x; 
																mouseY = y; 
																isDown = pressed;
																forceType = sign; };
	void resetParticles();

	// obstacle
	enum DIR { LEFT, DOWN, RIGHT, UP };
	bool showObstacle = false;
	bool loopObstacle = false;
	void initObject();
	void moveObjectX(float delta) { objectCenter += vec2(delta, 0.0); initObject(); };
	void moveObjectY(float delta) { objectCenter += vec2(0.0, delta); initObject(); };
	void setObject(vec2 pos) { objectCenter = pos; currentDir = LEFT; };
	void loopObject();

private:
	// openGL screen
	int xScreenRes = 1028;
	int yScreenRes = 768;

	// simulation bounds
	vec2 boundsMin;
	vec2 boundsMax;

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
	GLuint fluidRenderer, objectRenderer;
	GLuint VAO;
	GLuint objectVAO, objectVBO; 

	int numParticles;

	// 2d sim parameters
	int maxIterations = 8;
	float dt = 1.0f/60.0f;
	float gravity = 120.0f;
	float restDensity = 0.02f;
	float smoothingRadius = 40.0f;
	float stiffness = 0.003f;
	float eta = 0.01f;
	float viscosityStrength = 0.9;

	// obstacles
	vec2 objectStretch = vec2(0.5, 0.75);
	vec2 objectCenter = vec2(0.5, 0.5);
	DIR currentDir = LEFT;

	// mouse forces
	float mouseStrength = 3000.0;
	float mouseRadius = 200.0;
	
	int isDown = false;
	float mouseX = 0.0f;
	float mouseY = 0.0f;
	int forceType = 1;
};

#endif