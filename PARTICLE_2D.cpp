#include "PARTICLE_2D.h"

Parallel::Parallel(int num)
{
	numParticles = num;
}

///////////////////////////////////////////////////////////////////////
// initialization functions
///////////////////////////////////////////////////////////////////////

void Parallel::initParticlesAndProgram()
{
	// generate grid of initial positions
	float *positions = new float[2 * numParticles];
	float *velocities = new float[2 * numParticles];

	int width = (int)sqrt(numParticles);
	int height = (numParticles + width - 1) / width; // ceil division
	float spacing = 5.0f;  // use reasonable pixel spacing

	for (int i = 0; i < numParticles; i++) {
		float gridWidth = width * spacing;
		float gridHeight = height * spacing;
		float offsetX = (xScreenRes - gridWidth) / 2.0f;
		float offsetY = (yScreenRes - gridHeight) / 2.0f;

		float x = (i % width) * spacing + offsetX;
		float y = (i / width) * spacing + offsetY;

		positions[2 * i + 0] = x;
		positions[2 * i + 1] = y;

		velocities[2 * i + 0] = 0.0f;
		velocities[2 * i + 1] = 0.0f;
	}

	// Create SSBOs and fill them
	initComputeShaders(positions, velocities);

	// Bind SSBOs to fixed bindings
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, posSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, velSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, predPosSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, predVelSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, densitySSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, pressureSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, maxDensityError);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// Dummy VAO needed by OpenGL Core profile
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glBindVertexArray(0);

	delete[] positions;
	delete[] velocities;
}

void Parallel::initComputeShaders(float *positions, float *velocities)
{
	glGenBuffers(1, &posSSBO);
	glGenBuffers(1, &velSSBO);
	glGenBuffers(1, &predPosSSBO);
	glGenBuffers(1, &predVelSSBO);
	glGenBuffers(1, &densitySSBO);
	glGenBuffers(1, &pressureSSBO);
	glGenBuffers(1, &maxDensityError);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, posSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * 2 * numParticles, positions, GL_STATIC_DRAW);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, velSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * 2 * numParticles, velocities, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, predPosSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * 2 * numParticles, positions, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, predVelSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * 2 * numParticles, velocities, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, densitySSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * numParticles, nullptr, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, pressureSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * numParticles, nullptr, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, maxDensityError);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float), nullptr, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	progApplyExtForces    = createComputeShader("compute2d/2D_extForces.glsl");
	progComputeDensities  = createComputeShader("compute2d/2D_computeDensities.glsl");
	progApplyPressures    = createComputeShader("compute2d/2D_applyPressures.glsl");
	progApplyViscosity    = createComputeShader("compute2d/2D_viscosity.glsl");
	progResolveCollisions = createComputeShader("compute2d/2D_resolveCollisions.glsl");
}

void Parallel::initRenderer(const char *vertexPath, const char *fragmentPath)
{
	fluidRenderer  = createRenderProgram(vertexPath, fragmentPath);
	objectRenderer = createRenderProgram("render2d/object_vert.glsl", "render2d/object_frag.glsl");
}

///////////////////////////////////////////////////////////////////////
// simulation functions
///////////////////////////////////////////////////////////////////////

void Parallel::render()
{
	// render particles
	glUseProgram(fluidRenderer);

	glBindVertexArray(VAO);

	glm::mat4 projection = glm::ortho(
		0.0f, static_cast<float>(xScreenRes),
		0.0f, static_cast<float>(yScreenRes),
		-1.0f, 1.0f
	);
	
	// bind position buffer to attribute 0
	GLuint projLoc = glGetUniformLocation(fluidRenderer, "uProjection");
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	// Bind SSBOs directly
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, posSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, velSSBO);

	// You don't even need VAO unless your shader requires it
	glPointSize(10.0f);
	glDrawArrays(GL_POINTS, 0, numParticles);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);

	glBindVertexArray(0);

	// render obstacle
	if (!showObstacle) { glUseProgram(0); return; }
	
	glUseProgram(objectRenderer);

	// Send it to the shader
	projLoc = glGetUniformLocation(objectRenderer, "uProjection");
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	glBindVertexArray(objectVAO);
    glDrawArrays(GL_LINE_STRIP, 0, 5);  // draw box outline
    glBindVertexArray(0);

	glUseProgram(0);
}

void Parallel::compute()
{
	GLuint groups = (numParticles + 63) / 64;

	// 1. apply external forces
	glUseProgram(progApplyExtForces);

	glUniform1f(glGetUniformLocation(progApplyExtForces, "dt"), dt);
	glUniform1f(glGetUniformLocation(progApplyExtForces, "gravity"), gravity);

	glUniform1f(glGetUniformLocation(progApplyExtForces, "mouseX"), mouseX);
	glUniform1f(glGetUniformLocation(progApplyExtForces, "mouseY"), mouseY);
	glUniform1f(glGetUniformLocation(progApplyExtForces, "mouseStrength"), mouseStrength);
	glUniform1f(glGetUniformLocation(progApplyExtForces, "mouseRadius"), mouseRadius);
	glUniform1i(glGetUniformLocation(progApplyExtForces, "isDown"), isDown ? 1 : 0);
	glUniform1i(glGetUniformLocation(progApplyExtForces, "forceType"), forceType);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, posSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, velSSBO);

	glDispatchCompute(groups, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	

	// delta
	float sumGradSquared = 24.0f / (M_PI * pow(smoothingRadius, 4.0f));
	float beta = 2.0f * dt * dt / (restDensity * restDensity);
	float delta = 1.0f / (beta * sumGradSquared);

	// 2. Compute Densities + Pressures
	int iter = 0;
	float maxDensityErrorFloat = 999.0f;
	int maxIterations = 8;

	std::vector<float> zeros(numParticles, 0.0f);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, pressureSSBO);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(float) * numParticles, zeros.data());
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	while ((maxDensityErrorFloat > eta) && (iter < maxIterations))
	{
		// reset maxDensityError to zero on GPU
		float zero = 0.0f;
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, maxDensityError);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(float), &zero);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		// 2a: compute densities and pressures
		glUseProgram(progComputeDensities);

		glUniform1f(glGetUniformLocation(progComputeDensities, "dt"), dt);
		glUniform1f(glGetUniformLocation(progComputeDensities, "restDensity"), restDensity);
		glUniform1f(glGetUniformLocation(progComputeDensities, "smoothingRadius"), smoothingRadius);
		glUniform1f(glGetUniformLocation(progComputeDensities, "delta"), delta);
		glUniform1ui(glGetUniformLocation(progComputeDensities, "numParticles"), numParticles);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, posSSBO);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, velSSBO);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, predPosSSBO);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, predVelSSBO);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, pressureSSBO);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, densitySSBO);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, maxDensityError);

		glDispatchCompute(groups, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		// read back maxDensityError from GPU
		uint32_t maxDensityErrorUint = 0;
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, maxDensityError);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t), &maxDensityErrorUint);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		std::memcpy(&maxDensityErrorFloat, &maxDensityErrorUint, sizeof(float));

		// 2b: apply pressure corrections
		glUseProgram(progApplyPressures);

		glUniform1f(glGetUniformLocation(progApplyPressures, "dt"), dt);
		glUniform1f(glGetUniformLocation(progApplyPressures, "restDensity"), restDensity);
		glUniform1f(glGetUniformLocation(progApplyPressures, "stiffness"), stiffness);
		glUniform1f(glGetUniformLocation(progApplyPressures, "smoothingRadius"), smoothingRadius);
		glUniform1ui(glGetUniformLocation(progApplyPressures, "numParticles"), numParticles);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, posSSBO);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, velSSBO);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, pressureSSBO);

		glDispatchCompute(groups, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		// 2c: apply viscosity forces
		glUseProgram(progApplyViscosity);
		glUniform1f(glGetUniformLocation(progApplyViscosity, "viscosityStrength"), viscosityStrength);
		glUniform1f(glGetUniformLocation(progApplyViscosity, "smoothingRadius"), smoothingRadius);
		glUniform1ui(glGetUniformLocation(progApplyViscosity, "numParticles"), numParticles);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, posSSBO);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, velSSBO);

		glDispatchCompute(groups, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		iter++;
	}

	if (!showObstacle) return;

	// 3: resolve collisions
	glUseProgram(progResolveCollisions); 

	glUniform2f(glGetUniformLocation(progResolveCollisions, "boxMin"), objectCenter.x - 0.5f * objectStretch.x, objectCenter.y - 0.5f * objectStretch.y);
	glUniform2f(glGetUniformLocation(progResolveCollisions, "boxMax"), objectCenter.x + 0.5f * objectStretch.x, objectCenter.y + 0.5f * objectStretch.y);
	glUniform1f(glGetUniformLocation(progResolveCollisions, "restitution"), 0.2f);  // adjust bounce
	glUniform1ui(glGetUniformLocation(progResolveCollisions, "numParticles"), numParticles);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, posSSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, velSSBO);

	glDispatchCompute(groups, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Parallel::resetParticles() {
	float *positions = new float[2 * numParticles];
	float *velocities = new float[2 * numParticles];

	int width = (int)sqrt(numParticles);
	int height = (numParticles + width - 1) / width; // ceil division
	float spacing = 5.0f;  // use reasonable pixel spacing

	for (int i = 0; i < numParticles; i++) {
		float gridWidth = width * spacing;
		float gridHeight = height * spacing;
		float offsetX = (xScreenRes - gridWidth) / 2.0f;
		float offsetY = (yScreenRes - gridHeight) / 2.0f;

		float x = (i % width) * spacing + offsetX;
		float y = (i / width) * spacing + offsetY;

		positions[2 * i + 0] = x;
		positions[2 * i + 1] = y;

		velocities[2 * i + 0] = 0.0f;
		velocities[2 * i + 1] = 0.0f;
	}

	// Overwrite GPU buffers with fresh data
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, posSSBO);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(float) * 2 * numParticles, positions);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, velSSBO);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(float) * 2 * numParticles, velocities);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, predPosSSBO);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(float) * 2 * numParticles, positions);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, predVelSSBO);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(float) * 2 * numParticles, velocities);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, densitySSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * numParticles, nullptr, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, pressureSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * numParticles, nullptr, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	delete[] positions;
	delete[] velocities;
}

void Parallel::initObject() {

	objectCenter = vec2(1028.0f / 2.0f + 200.0f, 768.0f / 2.0f - 100.0f);
	objectStretch = vec2(150.0f, 400.0f);

	float object[] = {
		objectCenter.x - 0.5f * objectStretch.x, objectCenter.y - 0.5f * objectStretch.y,
		objectCenter.x + 0.5f * objectStretch.x, objectCenter.y - 0.5f * objectStretch.y,
		objectCenter.x + 0.5f * objectStretch.x, objectCenter.y + 0.5f * objectStretch.y,
		objectCenter.x - 0.5f * objectStretch.x, objectCenter.y + 0.5f * objectStretch.y,
		objectCenter.x - 0.5f * objectStretch.x, objectCenter.y - 0.5f * objectStretch.y
	};
	
	glGenVertexArrays(1, &objectVAO);
    glBindVertexArray(objectVAO);

    glGenBuffers(1, &objectVBO);
    glBindBuffer(GL_ARRAY_BUFFER, objectVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(object), object, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glBindVertexArray(0);
}