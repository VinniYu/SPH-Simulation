#include "PARTICLE_3D.h"
#include "SPHERE.h"

Parallel::Parallel(int num)
{
	numParticles = num;
}

///////////////////////////////////////////////////////////////////////
// screen functions
///////////////////////////////////////////////////////////////////////

void Parallel::rotateCamLeft()
{
	float angle = -rot * M_PI / 180.0f; // converted to radians
	float x = camera.x;
	float z = camera.z;

	camera.x = x * cos(angle) + z * sin(angle);
	camera.z = -x * sin(angle) + z * cos(angle);

	// cout << "Camera pos: " << camera.x << ", "
	// 											 << camera.y << ", "
	// 											 << camera.z << endl;
}

void Parallel::rotateCamRight()
{
	float angle = rot * M_PI / 180.0f; // converted to radians
	float x = camera.x;
	float z = camera.z;

	camera.x = x * cos(angle) + z * sin(angle);
	camera.z = -x * sin(angle) + z * cos(angle);

	// cout << "Camera pos: " << camera.x << ", "
	// 											 << camera.y << ", "
	// 											 << camera.z << endl;
}

///////////////////////////////////////////////////////////////////////
// initialization functions
///////////////////////////////////////////////////////////////////////

void Parallel::initSimBounds()
{
	// define the vertices for a 2x2x2 cube centered at origin
	float vertices[] = {
		// front face
		-1.0f * xStretch,
		-1.0f,
		1.0f * zStretch,
		1.0f * xStretch,
		-1.0f,
		1.0f * zStretch,
		1.0f * xStretch,
		2.0f,
		1.0f * zStretch,
		-1.0f * xStretch,
		2.0f,
		1.0f * zStretch,
		// back face
		-1.0f * xStretch,
		-1.0f,
		-1.0f * zStretch,
		1.0f * xStretch,
		-1.0f,
		-1.0f * zStretch,
		1.0f * xStretch,
		2.0f,
		-1.0f * zStretch,
		-1.0f * xStretch,
		2.0f,
		-1.0f * zStretch,
	};

	boundsMin = vec3(-1.0 * xStretch, -1.0, -1.0 * zStretch);
	boundsMax = vec3(1.0 * xStretch, 2.0, 1.0 * zStretch);

	// indices for lines
	unsigned int indices[] = {
		// front
		0, 1, 1, 2, 2, 3, 3, 0,
		// back
		4, 5, 5, 6, 6, 7, 7, 4,
		// connecting lines
		0, 4, 1, 5, 2, 6, 3, 7};

	glGenVertexArrays(1, &boundVAO);
	glGenBuffers(1, &boundVBO);
	GLuint boundEBO;
	glGenBuffers(1, &boundEBO);

	glBindVertexArray(boundVAO);

	glBindBuffer(GL_ARRAY_BUFFER, boundVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, boundEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
	glEnableVertexAttribArray(0);

	// unbind
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Parallel::initParticleAndPrograms()
{
	std::vector<glm::vec4> positions(numParticles);
	std::vector<glm::vec4> velocities(numParticles);

	// generate a grid cube of initial positions
	int width = static_cast<int>(round(pow(numParticles, 1.0 / 3.0)));
	int height = width;
	int depth = (numParticles - 1) / (width * height) + 1;

	float spacing = 0.05;
	float gridWidth = width * spacing;
	float gridHeight = height * spacing;
	float gridDepth = depth * spacing;

	float offsetX = -gridWidth / 2.0f;
	float offsetY = -gridHeight / 2.0f;
	float offsetZ = -gridDepth / 2.0f;

	for (int i = 0; i < numParticles; i++)
	{
		int xi = i % width;
		int yi = (i / width) % height;
		int zi = i / (width * height);

		float x = (xi + 0.5f) * spacing + offsetX;
		float y = (yi + 0.5f) * spacing + offsetY;
		float z = (zi + 0.5f) * spacing + offsetZ;

		positions[i] = glm::vec4(x, y, z, 1.0f) + glm::vec4(0.5, 0.0, 0.0, 0.0);
		velocities[i] = glm::vec4(0.0f);
	}

	// initialize shader programs and ssbos
	initComputeShaders(positions, velocities);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, posSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, velSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, predPosSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, predVelSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, densitySSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, pressureSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, maxDensityError);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// set up sphere mesh and vao for instanced rendering
	std::vector<glm::vec3> sphereVerts;
	std::vector<unsigned int> sphereIndices;
	generateSphereMesh(sphereVerts, sphereIndices, 3);

	glGenVertexArrays(1, &particleVAO);
	glBindVertexArray(particleVAO);

	GLuint sphereVBO, sphereEBO;
	glGenBuffers(1, &sphereVBO);
	glGenBuffers(1, &sphereEBO);

	glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
	glBufferData(GL_ARRAY_BUFFER, sphereVerts.size() * sizeof(glm::vec3), sphereVerts.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);
	glEnableVertexAttribArray(0);

	// Index buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(unsigned int), sphereIndices.data(), GL_STATIC_DRAW);
	sphereIndexCount = sphereIndices.size();

	glBindVertexArray(0);
}

void Parallel::initComputeShaders(std::vector<glm::vec4> positions, std::vector<glm::vec4> velocities)
{
	glGenBuffers(1, &posSSBO);
	glGenBuffers(1, &velSSBO);
	glGenBuffers(1, &predPosSSBO);
	glGenBuffers(1, &predVelSSBO);
	glGenBuffers(1, &densitySSBO);
	glGenBuffers(1, &pressureSSBO);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, posSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) * numParticles, positions.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, velSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) * numParticles, velocities.data(), GL_DYNAMIC_DRAW);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, predPosSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) * numParticles, positions.data(), GL_DYNAMIC_DRAW);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, predVelSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) * numParticles, velocities.data(), GL_DYNAMIC_DRAW);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, densitySSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * numParticles, nullptr, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, pressureSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * numParticles, nullptr, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	progApplyExtForces = createComputeShader("compute3d/extForces.glsl");
	progComputeDensities = createComputeShader("compute3d/computeDensities.glsl");
	progApplyPressures = createComputeShader("compute3d/applyPressures.glsl");
	progApplyViscosity = createComputeShader("compute3d/viscosity.glsl");
	progResolveCollisions = createComputeShader("compute3d/resolveCollisions.glsl");
}

void Parallel::initRenderer(const char *boundVertex, const char *boundFragment,
							const char *fluidVertex, const char *fluidFragment)
{
	boundRenderer = createRenderProgram(boundVertex, boundFragment);
	fluidRenderer = createRenderProgram(fluidVertex, fluidFragment);
	objectRenderer = createRenderProgram("render3d/object_vert.glsl", "render3d/object_frag.glsl");
}

///////////////////////////////////////////////////////////////////////
// simulation functions
///////////////////////////////////////////////////////////////////////

void Parallel::render()
{
	// create transformation matrices
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::scale(model, glm::vec3(1.0f)); // scale to adjust cube size

	glm::mat4 view = glm::lookAt(
		glm::vec3(camera.x, camera.y, camera.z), // Camera position
		glm::vec3(origin.x, origin.y, origin.z), // Look at origin
		glm::vec3(up.x, up.y, up.z)				 // Up vector
	);

	glm::mat4 projection = glm::perspective(
		glm::radians(60.0f),				   // FOV
		(float)xScreenRes / (float)yScreenRes, // aspect ratio
		0.1f,								   // near plane
		10.0f								   // far plane
	);

	// RENDER BOUNDS
	glUseProgram(boundRenderer);

	// set uniforms
	GLuint modelLoc = glGetUniformLocation(boundRenderer, "model");
	GLuint viewLoc = glGetUniformLocation(boundRenderer, "view");
	GLuint projectionLoc = glGetUniformLocation(boundRenderer, "projection");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	// draw the cube in wireframe mode
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glBindVertexArray(boundVAO);
	glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);

	// RENDER FLUID
	glUseProgram(fluidRenderer);

	// set uniforms
	modelLoc = glGetUniformLocation(fluidRenderer, "model");
	viewLoc = glGetUniformLocation(fluidRenderer, "view");
	projectionLoc = glGetUniformLocation(fluidRenderer, "projection");
	GLint camLoc = glGetUniformLocation(fluidRenderer, "cameraPos");
	GLint lightLoc = glGetUniformLocation(fluidRenderer, "lightPos");

	vec3 lightPos;
	lightPos.x = camera.x * 0.7;
	lightPos.y = camera.y * -0.2;
	lightPos.z = camera.z;

	glUniform3f(camLoc, camera.x, camera.y, camera.z);
	glUniform3f(lightLoc, lightPos.x, lightPos.y, lightPos.z);
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	// draw particles
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, posSSBO);
	glBindVertexArray(particleVAO);
	// glPointSize(25.0f);
	// glDrawArrays(GL_POINTS, 0, numParticles);
	glDrawElementsInstanced(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0, numParticles);

	// RENDER OBSTACLE
	if (!doObstacle) { glUseProgram(0); return; }

	glUseProgram(objectRenderer);

	// uniforms
	modelLoc = glGetUniformLocation(objectRenderer, "model");
	viewLoc = glGetUniformLocation(objectRenderer, "view");
	projectionLoc = glGetUniformLocation(objectRenderer, "projection");

	glm::mat4 cubeModel = glm::mat4(1.0f); // centered at origin
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(cubeModel));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	glBindVertexArray(objectVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);

	glUseProgram(0);
}

void Parallel::compute()
{
	GLuint groups = (numParticles + 63) / 64;

	// 1. find neighbors, 3d will definitely need to do this

	// 2. apply external forces
	glUseProgram(progApplyExtForces);

	glUniform1f(glGetUniformLocation(progApplyExtForces, "dt"), dt);
	glUniform1f(glGetUniformLocation(progApplyExtForces, "gravity"), gravity);
	glUniform3f(glGetUniformLocation(progApplyExtForces, "boundsMin"), boundsMin.x,
				boundsMin.y,
				boundsMin.z);
	glUniform3f(glGetUniformLocation(progApplyExtForces, "boundsMax"), boundsMax.x,
				boundsMax.y,
				boundsMax.z);

	glDispatchCompute(groups, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	// 3. compute densities + pressures
	float beta = 2.0f * dt * dt / (restDensity * restDensity);
	float sumGradSquared = 315.0f / (64.0f * M_PI * pow(smoothingRadius, 9.0f)); // match your kernel
	float delta = 1.0f / (beta * sumGradSquared);

	int iter = 0;
	float maxDensityErrorFloat = 999.0f;

	// zero out pressure
	std::vector<float> zeros(numParticles, 0.0f);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, pressureSSBO);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(float) * numParticles, zeros.data());
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	while ((maxDensityErrorFloat > eta) && (iter < maxIterations))
	{
		float zero = 0.0f;
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, maxDensityError);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(float), &zero);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		// 3a: compute densities and pressures
		glUseProgram(progComputeDensities);

		glUniform1f(glGetUniformLocation(progComputeDensities, "dt"), dt);
		glUniform1f(glGetUniformLocation(progComputeDensities, "restDensity"), restDensity);
		glUniform1f(glGetUniformLocation(progComputeDensities, "smoothingRadius"), smoothingRadius);
		glUniform1f(glGetUniformLocation(progComputeDensities, "delta"), delta);
		glUniform1ui(glGetUniformLocation(progComputeDensities, "numParticles"), numParticles);

		glDispatchCompute(groups, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		// read back maxDensityError from GPU
		uint32_t maxDensityErrorUint = 0;
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, maxDensityError);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t), &maxDensityErrorUint);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		std::memcpy(&maxDensityErrorFloat, &maxDensityErrorUint, sizeof(float));

		// 3b: apply pressure corrections
		glUseProgram(progApplyPressures);

		glUniform1f(glGetUniformLocation(progApplyPressures, "dt"), dt);
		glUniform1f(glGetUniformLocation(progApplyPressures, "restDensity"), restDensity);
		glUniform1f(glGetUniformLocation(progApplyPressures, "smoothingRadius"), smoothingRadius);
		glUniform1f(glGetUniformLocation(progApplyPressures, "stiffness"), stiffness);
		glUniform1ui(glGetUniformLocation(progApplyPressures, "numParticles"), numParticles);
		glUniform3f(glGetUniformLocation(progApplyPressures, "boundsMin"), boundsMin.x,
					boundsMin.y,
					boundsMin.z);
		glUniform3f(glGetUniformLocation(progApplyPressures, "boundsMax"), boundsMax.x,
					boundsMax.y,
					boundsMax.z);

		glDispatchCompute(groups, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		// 3c: apply viscosity forces
		glUseProgram(progApplyViscosity);

		glUniform1f(glGetUniformLocation(progApplyViscosity, "viscosityStrength"), viscosityStrength);
		glUniform1f(glGetUniformLocation(progApplyViscosity, "smoothingRadius"), smoothingRadius);
		glUniform1ui(glGetUniformLocation(progApplyViscosity, "numParticles"), numParticles);

		glDispatchCompute(groups, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		iter++;
	}

	if (!doObstacle)
		return;

	glUseProgram(progResolveCollisions);

	glUniform3f(glGetUniformLocation(progResolveCollisions, "cubeMin"), objectCenter.x - size / 2, objectCenter.y - size / 2, objectCenter.z - size / 2);
	glUniform3f(glGetUniformLocation(progResolveCollisions, "cubeMax"), objectCenter.x + size / 2, objectCenter.y + size / 2, objectCenter.z + size / 2);
	glUniform1f(glGetUniformLocation(progResolveCollisions, "restitution"), 2.0f);
	glUniform1ui(glGetUniformLocation(progResolveCollisions, "numParticles"), numParticles);

	glDispatchCompute(groups, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Parallel::resetParticles()
{
	std::vector<glm::vec4> positions(numParticles);
	std::vector<glm::vec4> velocities(numParticles);

	// Regenerate the grid cube of initial positions
	int width = static_cast<int>(round(pow(numParticles, 1.0 / 3.0)));
	int height = width;
	int depth = (numParticles - 1) / (width * height) + 1;

	float spacing = 0.05f;
	float gridWidth = width * spacing;
	float gridHeight = height * spacing;
	float gridDepth = depth * spacing;

	float offsetX = -gridWidth / 2.0f;
	float offsetY = -gridHeight / 2.0f;
	float offsetZ = -gridDepth / 2.0f;

	for (int i = 0; i < numParticles; i++)
	{
		int xi = i % width;
		int yi = (i / width) % height;
		int zi = i / (width * height);

		float x = (xi + 0.5f) * spacing + offsetX;
		float y = (yi + 0.5f) * spacing + offsetY;
		float z = (zi + 0.5f) * spacing + offsetZ;

		positions[i] = glm::vec4(x, y, z, 1.0f) + glm::vec4(0.5, 0.4, 0.0, 0.0);
		velocities[i] = glm::vec4(0.0f);
	}

	// Upload to GPU
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, posSSBO);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(glm::vec4) * numParticles, positions.data());

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, velSSBO);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(glm::vec4) * numParticles, velocities.data());

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, predPosSSBO);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(glm::vec4) * numParticles, positions.data());

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, predVelSSBO);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(glm::vec4) * numParticles, velocities.data());

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

///

void Parallel::initObject()
{
	float half = size / 2.0f;

	std::vector<glm::vec3> cubeVertices = {
		// back face
		objectCenter + glm::vec3(-half, -half, -half),
		objectCenter + glm::vec3(half, -half, -half),
		objectCenter + glm::vec3(half, half, -half),
		objectCenter + glm::vec3(half, half, -half),
		objectCenter + glm::vec3(-half, half, -half),
		objectCenter + glm::vec3(-half, -half, -half),

		// front face
		objectCenter + glm::vec3(-half, -half, half),
		objectCenter + glm::vec3(half, -half, half),
		objectCenter + glm::vec3(half, half, half),
		objectCenter + glm::vec3(half, half, half),
		objectCenter + glm::vec3(-half, half, half),
		objectCenter + glm::vec3(-half, -half, half),

		// left face
		objectCenter + glm::vec3(-half, half, half),
		objectCenter + glm::vec3(-half, half, -half),
		objectCenter + glm::vec3(-half, -half, -half),
		objectCenter + glm::vec3(-half, -half, -half),
		objectCenter + glm::vec3(-half, -half, half),
		objectCenter + glm::vec3(-half, half, half),

		// right face
		objectCenter + glm::vec3(half, half, half),
		objectCenter + glm::vec3(half, half, -half),
		objectCenter + glm::vec3(half, -half, -half),
		objectCenter + glm::vec3(half, -half, -half),
		objectCenter + glm::vec3(half, -half, half),
		objectCenter + glm::vec3(half, half, half),

		// bottom face
		objectCenter + glm::vec3(-half, -half, -half),
		objectCenter + glm::vec3(half, -half, -half),
		objectCenter + glm::vec3(half, -half, half),
		objectCenter + glm::vec3(half, -half, half),
		objectCenter + glm::vec3(-half, -half, half),
		objectCenter + glm::vec3(-half, -half, -half),

		// top face
		objectCenter + glm::vec3(-half, half, -half),
		objectCenter + glm::vec3(half, half, -half),
		objectCenter + glm::vec3(half, half, half),
		objectCenter + glm::vec3(half, half, half),
		objectCenter + glm::vec3(-half, half, half),
		objectCenter + glm::vec3(-half, half, -half)};

	glGenVertexArrays(1, &objectVAO);
	glGenBuffers(1, &objectVBO);

	glBindVertexArray(objectVAO);
	glBindBuffer(GL_ARRAY_BUFFER, objectVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * cubeVertices.size(), cubeVertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
}

void Parallel::moveObjectX(float deltaX)
{
	objectCenter.x += deltaX;
	initObject();
}

void Parallel::moveObjectY(float deltaY)
{
	objectCenter.y += deltaY;
	initObject();
}

bool near(float a, float b) { return fabs(a - b) < 0.001f; }



void Parallel::loopObject()
{
	switch (currentDirection)
	{
	case LEFT:
		moveObjectX(-0.02);
		if (near(objectCenter.x, -1.0f))
		{
			objectCenter = glm::vec3(-1.0f, 1.0f, 0.0f);
			currentDirection = DOWN;
		}
		break;

	case DOWN:
		moveObjectY(-0.02);
		if (near(objectCenter.y, -0.5f))
		{
			objectCenter = glm::vec3(-1.0f, -0.5f, 0.0f);
			currentDirection = RIGHT;
		}

		break;

	case RIGHT:
		moveObjectX(0.02);
		if (near(objectCenter.x, 1.0f))
		{
			objectCenter = glm::vec3(1.0f, -0.5f, 0.0f);
			currentDirection = UP;
		}
		break;

	case UP:
		moveObjectY(0.02);
		if (near(objectCenter.y, 1.0f))
		{
			objectCenter = glm::vec3(1.0f, 1.0f, 0.0f);
			currentDirection = LEFT;
		}
		break;
	}
}
