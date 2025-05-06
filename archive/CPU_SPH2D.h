#ifndef PARTICLE_2D_H
#define PARTICLE_2D_H

#include <iostream>
#include <cmath>
#include <unordered_map>
#include <vector>
#include <functional>

#include "SETTINGS.h"

using namespace std;

float lowerBound = 0.0175;
float upperBound = 1.0f - lowerBound;

// spatial hash grid for lookup optimizations
struct gridCell {
	std::vector<int> particleIndices;
};

typedef std::pair<int, int> gridCoord;

struct gridHash {
	std::size_t operator()(const gridCoord &key) const {
		return std::hash<int>()(key.first) ^ (std::hash<int>()(key.second) << 1);
	}
};

// Particle structure
class Particles {
private:
	// arrays of 2 vectors
	vec2 *_positions = nullptr;
	vec2 *_velocities = nullptr;
	vec2 *_pressureForces = nullptr;
	vec2 *_predictedPositions = nullptr;
	vec2 *_predictedVelocities = nullptr;
	
	
	REAL *_pressures = nullptr;
	REAL *_densities = nullptr;
	
	
	REAL _mass;   // all particles have equal mass
	REAL _radius; // all particles have equal radius
	

	int _numParticles;
	bool _doGravity;
	
	std::unordered_map<gridCoord, gridCell, gridHash> _spatialGrid;
	REAL _gridCellSize;

public:
	
	REAL _delta;
	REAL _gravity;
	REAL _forceScale;
	REAL _smoothingRadius; 

	// constructor
	Particles(int numParticles, REAL radius = 0.005, bool doGravity = true) {
		_numParticles = numParticles;
		_positions           = new vec2[numParticles];
		_velocities          = new vec2[numParticles];
		_pressureForces      = new vec2[numParticles];
		_predictedPositions  = new vec2[numParticles];
		_predictedVelocities = new vec2[numParticles];
		_densities  = new REAL[numParticles];
		_pressures  = new REAL[numParticles];
		

		_radius = radius;
		// _smoothingRadius = _radius * 35.0;
		_smoothingRadius = _radius * 15.0;
		_doGravity = doGravity;
		_mass = 1.0;
		_delta = 0.5;
		_forceScale = 1.0;

		// space the particles out in a grid
		int width  = (int)sqrt(numParticles);
		int height = (numParticles-1) / width + 1;
		float spacing = _radius*3.75;

		float gridWidth = width * spacing;
		float gridHeight = height * spacing;
		float offsetX = (1.0f - gridWidth) / 2.0f;
		float offsetY = (1.0f - gridHeight) / 2.0f;

		for (int i = 0; i < numParticles; i++) {
			float x = (i % width + 0.5f) * spacing;
			float y = (i / width + 0.5f) * spacing;
			
			// offset to center in world space
			x += offsetX - spacing;
			y += offsetY - spacing;

			// initialize values
			_positions[i]  = vec2(x,y);
			_velocities[i] = vec2(0.0f, 0.0f);
			_densities[i]  = 0.0;
		}
	}

	// destructor
	~Particles() {
		delete[] _positions;
		delete[] _velocities;
		delete[] _densities;
	}

	// accessors
	int getNumParticles() const { return _numParticles; }
	REAL getSmoothingRadius() const { return _smoothingRadius; }
	vec2* getPositions()        { return _positions; }
	vec2* getVelocities()       { return _velocities; }
	REAL* getDensities()        { return _densities; }
	void updatePos(int i, vec2 value) { _positions[i] = value; }
	void updateVel(int i, vec2 value) { _velocities[i] = value; }

	// accessors for individual particles
	vec2& position(int i) { return _positions[i]; }
	vec2& velocity(int i) { return _velocities[i]; }
	REAL& density(int i)  { return _densities[i]; }


	void update(REAL dt) {
		for (int i = 0; i < _numParticles; i++) {
			if (_doGravity) _velocities[i].y -= _gravity;
			
			_positions[i] = _positions[i] + _velocities[i] * 1./60;

			checkBoundaries(i);
		}

		calculateDensities();
	}

	void checkBoundaries(int i) {
		float damping = 0.9;

		if (_positions[i].x < lowerBound) {
			_positions[i].x = lowerBound;
			_velocities[i].x = -_velocities[i].x * damping;
		}
		else if (_positions[i].x > upperBound) {
			_positions[i].x = upperBound;
			_velocities[i].x = -_velocities[i].x * damping;
		}

		if (_positions[i].y < lowerBound) {
			_positions[i].y = lowerBound;
			_velocities[i].y = -_velocities[i].y * damping;
		}
		else if (_positions[i].y > upperBound) {
			_positions[i].y = upperBound;
			_velocities[i].y = -_velocities[i].y * damping;
		}
	}

	void reset() {
		int width = (int)sqrt(_numParticles);
		int height = (_numParticles-1) / width + 1;
		float spacing = _radius*3.75;

		float gridWidth = width * spacing;
		float gridHeight = height * spacing;
		float offsetX = (1.0f - gridWidth) / 2.0f;
		float offsetY = (1.0f - gridHeight) / 2.0f;

		for (int i = 0; i < _numParticles; i++) {
			float x = (i % width + 0.5f) * spacing;
			float y = (i / width + 0.5f) * spacing;
			
			// Offset to center in world space
			x += offsetX - spacing;
			y += offsetY - spacing;

			_positions[i] = vec2(x, y);
			_velocities[i] = vec2(0.0f, 0.0f);
		}
	}

	void randomize() {
		float range = upperBound - lowerBound;

		for (int i = 0; i < _numParticles; i++) {
			// random positions
			float randX = lowerBound + (float)rand() / RAND_MAX * range;
			float randY = lowerBound + (float)rand() / RAND_MAX * range;
			_positions[i] = vec2(randX, randY);
		}
	}

	REAL smoothingKernel(REAL dst, REAL radius) {
		if (dst >= radius) return 0;

		float volume = (M_PI * pow(radius, 4)) / 6.0;
		return (radius - dst) * (radius - dst) / volume;
	}

	void calculateDensities() {
		// reset densities
		for (int i = 0; i < _numParticles; i++)
			_densities[i] = 0.0;

		// loop over all particles for each particle. 
		// TODO: make more efficient
		for (int i = 0; i < _numParticles; i++) {
			for (int j = 0; j < _numParticles; j++) {
				vec2 rij = _positions[i] - _positions[j];
				REAL dst = rij.length();
				_densities[i] += _mass * smoothingKernel(dst, _smoothingRadius);
			}
		}
	}

	vec2 gradKernel(const vec2& rij, REAL h) {
		REAL r = rij.length();
		if (r == 0.0 || r >= h) return vec2(0.0, 0.0);

		const REAL coeff = -45.0 / (M_PI * pow(h, 6));
		REAL factor = coeff * pow(h - r, 2.0);

		return (rij / r) * factor;
	}

	void buildSpatialGrid() {
		_spatialGrid.clear();

		for (int i = 0; i < _numParticles; i++) {
			int gx = (int)(_positions[i].x / _gridCellSize);
			int gy = (int)(_positions[i].y / _gridCellSize);
			gridCoord coord = std::make_pair(gx, gy);
			_spatialGrid[coord].particleIndices.push_back(i);
		}
	}

	void calculateDensitiesPredicted() {
		for (int i = 0; i < _numParticles; i++) {
			_densities[i] = 0.0;

			int gx = (int)(_predictedPositions[i].x / _gridCellSize);
			int gy = (int)(_predictedPositions[i].y / _gridCellSize);

			for (int dx = -1; dx <= 1; dx++) {
				for (int dy = -1; dy <= 1; dy++) {
					gridCoord neighborCoord = std::make_pair(gx + dx, gy + dy);
					auto it = _spatialGrid.find(neighborCoord);
					if (it == _spatialGrid.end()) continue;

					for (int j : it->second.particleIndices) {
						vec2 rij = _predictedPositions[i] - _predictedPositions[j];
						REAL dst = rij.length();
						_densities[i] += _mass * smoothingKernel(dst, _smoothingRadius);
					}
				}
			}
		}
	}

	void computePressureForcesPredicted() {
		for (int i = 0; i < _numParticles; i++) {
			vec2 force(0.0, 0.0);

			int gx = (int)(_predictedPositions[i].x / _gridCellSize);
			int gy = (int)(_predictedPositions[i].y / _gridCellSize);

			for (int dx = -1; dx <= 1; dx++) {
				for (int dy = -1; dy <= 1; dy++) {
					gridCoord neighborCoord = std::make_pair(gx + dx, gy + dy);
					auto it = _spatialGrid.find(neighborCoord);
					if (it == _spatialGrid.end()) continue;

					for (int j : it->second.particleIndices) {
						if (i == j) continue;

						vec2 rij = _predictedPositions[i] - _predictedPositions[j];
						REAL dst = rij.length();
						if (dst >= _smoothingRadius || dst == 0.0) continue;

						vec2 gradW = gradKernel(rij, _smoothingRadius);

						// Pressure terms (already using non-accumulative pressure)
						REAL pi = _delta * (_densities[i] - 1.0);
						REAL pj = _delta * (_densities[j] - 1.0);
						REAL sharedP = 0.5 * (pi + pj);

						// Optional: add near-pressure here if implemented
						REAL rhoi = _densities[i];
						REAL rhoj = _densities[j];

						force -= gradW * (_mass * _mass * (sharedP / (rhoi * rhoi) + sharedP / (rhoj * rhoj)));
					}
				}
			}

			_pressureForces[i] = force * _forceScale;
		}
	}

	void updatePCISPH(REAL dt) {
		const REAL eta = 0.01;
		const int maxIters = 5;
		const REAL restDensity = 1.0;

		for (int i = 0; i < _numParticles; i++) {
			if (_doGravity) _velocities[i].y -= _gravity;

			_pressures[i] = 0;
			_pressureForces[i] = vec2(0.0, 0.0);
		}

		// store predicted values
		for (int i = 0; i < _numParticles; i++) {
			_predictedPositions[i]  = _positions[i];
			_predictedVelocities[i] = _velocities[i];
		}

		// main while loop
		int iter = 0;
		REAL maxDensityError = 1.0;

		while (maxDensityError > eta && iter < maxIters) {
			// 1. predict positions
			for (int i = 0; i < _numParticles; i++) 
				_predictedPositions[i] = _positions[i] + _predictedVelocities[i] * dt;

			// 2. predict densities
			buildSpatialGrid();
			calculateDensitiesPredicted();

			// 3. update pressures
			maxDensityError = 0.0;
			for (int i = 0; i < _numParticles; i++) {
				REAL err = _densities[i] - restDensity;
				_pressures[i] += std::max(_delta * err, 0.0);
				maxDensityError = max(maxDensityError, fabs(err));
			}

			// 4. compute pressure force with updated pressures
			computePressureForcesPredicted();

			// 5. predict new velocities
			for (int i = 0; i < _numParticles; i++) 
				_predictedVelocities[i] = _velocities[i] + (_pressureForces[i] / _mass) * dt;

			iter++;
		}

		for (int i = 0; i < _numParticles; i++) {
			_velocities[i] += (_pressureForces[i] / _mass) * dt;
			_positions[i] += _velocities[i] * dt;
			checkBoundaries(i);
		}
	}

	void tweak(REAL delta, REAL scale) {
			_delta = delta;
			_forceScale = scale;
	}
};



#endif