#ifndef DRAW_2D_H
#define DRAW_2D_H

#if _WIN32
#include <GL/freeglut.h>
#elif __APPLE__
#include <GLUT/glut.h>
#elif __linux__
#include <GL/freeglut.h>
#endif


void drawHardParticle(float centerX, float centerY, float radius, float speed, int segments = 32) 
{
	float aspectRatio = (float)xScreenRes / (float)yScreenRes;
	
	// set circle color
	float clampedVel = std::min(std::max(speed, 0.0f), 3.0f);
  clampedVel /= 3.0;

  // interpolate from blue to red
  float red   = clampedVel;
  float green = 0.0f;
  float blue  = 1.0f - clampedVel;

  glColor3f(red, green, blue);
	
	// use triangles for a filled circle
	glBegin(GL_TRIANGLE_FAN);
	glVertex2f(centerX, centerY); 
	
	for(int i = 0; i <= segments; i++) {
		float angle = 2.0f * M_PI * i / segments;

		float x = centerX + (radius * cos(angle));
		float y = centerY + (radius * sin(angle) * aspectRatio);
		glVertex2f(x, y);
	}
	glEnd();
}

void drawDensity(Particles* allParticles) {
  int divisor = 4; // to control resolution
  int xRes = xScreenRes / divisor;
  int yRes = yScreenRes / divisor;

  float aspectRatio = (float)xScreenRes / (float)yScreenRes;

  REAL cellWidth = 1.0f / xRes;
  REAL cellHeight = 1.0f / yRes;
  REAL maxDensity = 0.0;

  // find max density to normalize
  for (int i = 0; i < allParticles->getNumParticles(); i++) 
    maxDensity = max(maxDensity, (REAL)allParticles->density(i));
  
  // draw density as grid
  glBegin(GL_QUADS); 
  for (int y = 0; y < yRes; y++) {
    for (int x = 0; x < xRes; x++) {
      REAL xPos = x*cellWidth;  
      REAL yPos = y*cellHeight;

      vec2 pos(xPos + cellWidth/2, yPos + cellHeight/2);

      float density = 0.0f;
      for (int i = 0; i < allParticles->getNumParticles(); i++) {
        vec2 rij = pos - allParticles->position(i);
        rij.x *= aspectRatio;

        float dst = rij.length();
        float adjustedSmoothingRadius = allParticles->getSmoothingRadius() * aspectRatio;
        density += allParticles->smoothingKernel(dst, adjustedSmoothingRadius);
      }

      REAL normalizedDensity = min((REAL)1.0f, density / maxDensity);

      float r = normalizedDensity * 0.1f;
      float g = normalizedDensity * 0.5f + normalizedDensity;
      float b = normalizedDensity * 0.9f + normalizedDensity;
      float alpha = normalizedDensity * 0.9f;
      
      glColor4f(r, g, b, alpha);  // Slightly transparent
      glVertex2f(xPos, yPos);
      glVertex2f(xPos + cellWidth, yPos);
      glVertex2f(xPos + cellWidth, yPos + cellHeight);
      glVertex2f(xPos, yPos + cellHeight);
    }  
  }
  glEnd();
}

// 10% of boundaries left for box
void drawBoundingBox() {
  glColor3f(1.0f, 1.0f, 1.0f);

  glLineWidth(10.0f);

  glBegin(GL_LINE_LOOP);
    glVertex2f(0.9f, 0.1f); // bottom right
    glVertex2f(0.9f, 0.9f); // top right
    glVertex2f(0.1f, 0.9f); // top left
    glVertex2f(0.1f, 0.1f); // bottom left 
  glEnd();
}


#endif