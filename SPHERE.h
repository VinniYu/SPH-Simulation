#ifndef SPHERE_H
#define SPHERE_H

#include "SETTINGS.h"

void generateSphereMesh(std::vector<glm::vec3>& vertices, std::vector<unsigned int>& indices, int segments = 8, int rings = 8) {
	for (int y = 0; y <= rings; ++y) {
		for (int x = 0; x <= segments; ++x) {
			float xSegment = (float)x / segments;
			float ySegment = (float)y / rings;
			float xPos = std::cos(xSegment * 2.0f * M_PI) * std::sin(ySegment * M_PI);
			float yPos = std::cos(ySegment * M_PI);
			float zPos = std::sin(xSegment * 2.0f * M_PI) * std::sin(ySegment * M_PI);

			vertices.emplace_back(xPos, yPos, zPos);
		}
	}

	for (int y = 0; y < rings; ++y) {
		for (int x = 0; x < segments; ++x) {
			int i0 = y       * (segments + 1) + x;
			int i1 = (y + 1) * (segments + 1) + x;
			int i2 = (y + 1) * (segments + 1) + x + 1;
			int i3 = y       * (segments + 1) + x + 1;

			indices.push_back(i0);
			indices.push_back(i1);
			indices.push_back(i2);

			indices.push_back(i0);
			indices.push_back(i2);
			indices.push_back(i3);
		}
	}
}

GLuint createSphereVAO(const std::vector<glm::vec3>& vertices, const std::vector<unsigned int>& indices, GLuint& sphereVBO, GLuint& sphereEBO) {
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &sphereVBO);
	glGenBuffers(1, &sphereEBO);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

	glBindVertexArray(0);

	return vao;
}

GLuint createInstanceVBO(GLuint& instanceVBO, const std::vector<glm::vec3>& positions) {
	glGenBuffers(1, &instanceVBO);
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec3), positions.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	return instanceVBO;
}

void setupInstanceAttrib(GLuint vao, GLuint instanceVBO, GLuint attribLocation = 1) {
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glEnableVertexAttribArray(attribLocation);
	glVertexAttribPointer(attribLocation, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
	glVertexAttribDivisor(attribLocation, 1); // Update per instance
	glBindVertexArray(0);
}


#endif