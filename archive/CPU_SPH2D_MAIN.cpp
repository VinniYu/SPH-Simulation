
#include <cmath>
#include <iostream>
#include <ctime>

#include <GL/glew.h>

#include "SETTINGS.h"
#include "PARTICLE_2D.h"
#include "SHADER.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#if _WIN32
#include <gl/glut.h>
#elif __APPLE__
#include <GLUT/glut.h>
#elif __linux__
#include <GL/glut.h>
#endif

using namespace std;

// forward declarations
void runOnce();
void runEverytime();

void initRenderer(int numParticles, const char* vertexPath, const char* fragmentPath);
void initComputeShaders(int numParticles, const char* forcePath);
void initComputeShadersSlow(int numParticles);
void render();
void compute(float gravity);
void computeSlow(float gravity);

// Text for the title bar of the window
string windowLabel("2D SPH");
// the resolution of the OpenGL window
int xScreenRes = 1440; 
int yScreenRes = 900;

// sim parameters
int numParticles = 1;
float radius = 0.001;
bool doGravity = true;
bool drawDensityFlag = false;

float dt = 0.0008f;
float gravity = 25.0f;
float restDensity = 11000.0f;
float smoothingRadius = 0.02f;
float stiffness = 0.0000002f;
float eta = 0.01f; // Acceptable density error

// for 4000 particles
// float dt = 0.0008f;
// float gravity = 45.0f;
// float restDensity = 1000.0f;
// float smoothingRadius = 0.0175f;
// float stiffness = 0.000019f;
// float eta = 0.01f; // Acceptable density error

float viscosityStrength = 0.00001;
float mouseStrength = 250.0;
float mouseRadius = 0.2;

vec2 mousePos;
int isDown = false;
float mouseX = 0.0f;
float mouseY = 0.0f;

// simulation particles
Particles *allParticles;

// SSBOs
GLuint posSSBO, velSSBO;
GLuint predPosSSBO, predVelSSBO;
GLuint densitySSBO, pressureSSBO;
GLuint maxDensityError;

// compute shaders
GLuint applyForceProgram;
GLuint densityPressureProgram;
GLuint updatePositionsProgram;
GLuint applyViscosityProgram;

// rendering
GLuint renderer;
GLuint vao, posVBO, velVBO;

// animate the current runEverytime()?
bool animate = true;

// the current viewer eye position
float eyeCenter[] = {0.5, 0.5, 1};

// current zoom level into the field
float zoom = 1.0;

///////////////////////////////////////////////////////////////////////
// GL and GLUT callbacks
///////////////////////////////////////////////////////////////////////
void glutDisplay()
{
  // Prepare for drawing
  // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Start the new ImGui frame AFTER OpenGL context is active
}


void glutMotion(int x, int y) {
  mouseX = (float)x / xScreenRes;
  mouseY = 1.0f - float(y)/yScreenRes;

}

void glutMouse(int button, int state, int x, int y) {
  if (button == GLUT_LEFT_BUTTON) {
    isDown = (state == GLUT_DOWN);
    mouseX = (float)x / xScreenRes;
    mouseY = 1.0f - (float)y / yScreenRes;
  }
}

///////////////////////////////////////////////////////////////////////
// Map the arrow keys to something here
///////////////////////////////////////////////////////////////////////
void glutSpecial(int key, int x, int y)
{
  switch (key) {
  case GLUT_KEY_LEFT:
    break;
  case GLUT_KEY_RIGHT:
    break;
  case GLUT_KEY_UP:
    break;
  case GLUT_KEY_DOWN:
    break;
  default:
    break;
  }
}

///////////////////////////////////////////////////////////////////////
// Map the keyboard keys to something here
///////////////////////////////////////////////////////////////////////
void glutKeyboard(unsigned char key, int x, int y)
{
  switch (key) {
  case 'a':
    animate = !animate;
    break;
  case 'c':
    allParticles->reset();
    break;
  case 'd':
    drawDensityFlag = !drawDensityFlag;
    break;
  case 'r': {
    REAL delta, scale;

    // update values 
    cout << "delta: ";
    cin >> delta;
    cout << "scale: ";
    cin >> scale;

    allParticles->tweak(delta, scale);
    allParticles->reset();
    allParticles->randomize();
  } break;
  
  case 'w': {
    static int count = 0;
    char buffer[256];
    sprintf(buffer, "output_%i.ppm", count);
    count++;
  } break;
  case 'q':
    exit(0);
    break;
  default:
    break;
  }
}

///////////////////////////////////////////////////////////////////////
// animate and display new result
///////////////////////////////////////////////////////////////////////
void glutIdle()
{
  if (animate) {
    runEverytime();
  }
  glutPostRedisplay();
}

//////////////////////////////////////////////////////////////////////////////
// open the GLVU window
//////////////////////////////////////////////////////////////////////////////
int glvuWindow()
{
  glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
  glutInitWindowSize(xScreenRes, yScreenRes);
  glutInitWindowPosition(10, 10);

  // request OpenGL 4.3 and core profile
  glutInitContextVersion(4, 3); 
  glutInitContextProfile(GLUT_CORE_PROFILE); 

  // create the context
  glutCreateWindow(windowLabel.c_str());

  // enable modern OpenGL extensions  
  glewExperimental = GL_TRUE; 
  if (glewInit() != GLEW_OK) {
      fprintf(stderr, "Failed to initialize GLEW\n");
      exit(EXIT_FAILURE);
  }

  // initialize everything
  runOnce();

  // set the viewport resolution (w x h)
  glViewport(0, 0, (GLsizei)xScreenRes, (GLsizei)yScreenRes);
  glClearColor(0.0, 0.0, 0.0, 0);

  // register all the callbacks
  glutDisplayFunc(&glutDisplay);
  glutIdleFunc(&glutIdle); 
  glutKeyboardFunc(&glutKeyboard);
  glutSpecialFunc(&glutSpecial);  
  glutMotionFunc(&glutMotion);
  glutMouseFunc(&glutMouse);

  // enter the infinite GL loop
  glutMainLoop();

  // Control flow will never reach here
  return EXIT_SUCCESS;
}  

/////////////////////////////////////////////////////////////////////// 
/////////////////////////////////////////////////////////////////////// 
int main(int argc, char **argv)
{
  // initialize GLUT and GL
  glutInit(&argc, argv); 

  // open the GL window
  glvuWindow();
  return 0;
}

void runEverytime()       
{          
  // clear the screen
  glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  // enable point size for particle rendering
  glEnable(GL_PROGRAM_POINT_SIZE);
  glDisable(GL_DEPTH_TEST);
  
  // Update simulation
  if (animate) {
    computeSlow(gravity);
  }
  
  // render particles
  render();
  glUseProgram(0);

  // swap buffers
  glutSwapBuffers();
}

///////////////////////////////////////////////////////////////////////
// This is called once at the beginning so you can precache
// something here
///////////////////////////////////////////////////////////////////////
void runOnce()
{
  // seed the RNG
  srand(time(NULL));

  numParticles = 16641;
  // numParticles = 4032;
  // numParticles = 1024;
  radius = 0.0025;
  dt = 0.001;

  // TODO: when this is fully on GPU, don't really need the class anymore
  allParticles = new Particles(numParticles, radius);
  allParticles->randomize();

  initRenderer(numParticles, "render/vert2.glsl", "render/frag2.glsl");
  initComputeShadersSlow(numParticles);

}
  
///////////////////////////////////////////////////////////////////////
// needs to be called AFTER Particles constructor
///////////////////////////////////////////////////////////////////////
void initRenderer(int numParticles, const char* vertexPath, const char* fragmentPath) {
  // flatten positions
  float *positions = new float[2 * numParticles];
  float *velocities = new float[2 * numParticles];
  
  for (int i = 0; i < numParticles; i++) {
    positions[2*i]    = allParticles->position(i).x;
    positions[2*i+1]  = allParticles->position(i).y;

    velocities[2*i]   = allParticles->velocity(i).x;
    velocities[2*i+1] = allParticles->velocity(i).y;
  }

  // create vao 
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  // position VBO 
  glGenBuffers(1, &posVBO);
  glBindBuffer(GL_ARRAY_BUFFER, posVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * numParticles, positions, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

  // velocity VBO 
  GLuint velVBO;
  glGenBuffers(1, &velVBO);
  glBindBuffer(GL_ARRAY_BUFFER, velVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * numParticles, velocities, GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

  delete[] positions;
  delete[] velocities;

  // unbind VAO
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  // create shader program
  renderer = createRenderProgram(vertexPath, fragmentPath);
}

void render() {
  glUseProgram(renderer);

  glBindVertexArray(vao);

  glBindBuffer(GL_ARRAY_BUFFER, velSSBO);
  float* velData = (float*)glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);

  float maxSpeed = 1e-5f; // Avoid division by zero
  if (velData) {
      for (int i = 0; i < numParticles; ++i) {
          float vx = velData[2 * i];
          float vy = velData[2 * i + 1];
          float speed = sqrt(vx * vx + vy * vy);
          if (speed > maxSpeed)
              maxSpeed = speed;
      }
      glUnmapBuffer(GL_ARRAY_BUFFER);
  }
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  // --- Pass maxSpeed to shader
  GLint loc = glGetUniformLocation(renderer, "maxSpeed");
  glUniform1f(loc, maxSpeed);



  // bind position buffer to attribute 0
  glBindBuffer(GL_ARRAY_BUFFER, posSSBO);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
  glEnableVertexAttribArray(0);

  // bind velocity buffer to attribute 1
  glBindBuffer(GL_ARRAY_BUFFER, velSSBO);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
  glEnableVertexAttribArray(1);

  glDrawArrays(GL_POINTS, 0, numParticles);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  glUseProgram(0);
}

void initComputeShadersSlow(int numParticles) {
  glGenBuffers(1, &posSSBO);
  glGenBuffers(1, &velSSBO);
  glGenBuffers(1, &predPosSSBO);
  glGenBuffers(1, &predVelSSBO);
  glGenBuffers(1, &densitySSBO);
  glGenBuffers(1, &pressureSSBO);

  float *positions = new float[2 * numParticles];
  float *velocities = new float[2 * numParticles];

  // can change position init here to get rid of PARTICLE_2D class
  for (int i = 0; i < numParticles; i++) {
    positions[2*i]   = allParticles->position(i).x;
    positions[2*i+1] = allParticles->position(i).y;

    velocities[2*i]   = 0.0f;
    velocities[2*i+1] = 0.0f;
  }

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, posSSBO);
  glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * 2 * numParticles, positions, GL_DYNAMIC_DRAW);

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

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

  applyForceProgram      = createComputeShader("comp/slowExtForces.glsl");
  densityPressureProgram = createComputeShader("comp/slowComputeDensities.glsl");
  updatePositionsProgram = createComputeShader("comp/slowApplyPressures.glsl");
  applyViscosityProgram  = createComputeShader("comp/slowViscosity.glsl");
}

void computeSlow(float gravity)
{
  GLuint groups = (numParticles + 63) / 64;

  // 1. apply external forces
  glUseProgram(applyForceProgram);

  glUniform1f(glGetUniformLocation(applyForceProgram, "dt"), dt);
  glUniform1f(glGetUniformLocation(applyForceProgram, "gravity"), gravity);

  glUniform1f(glGetUniformLocation(applyForceProgram, "mouseX"), mouseX);
  glUniform1f(glGetUniformLocation(applyForceProgram, "mouseY"), mouseY);
  glUniform1f(glGetUniformLocation(applyForceProgram, "mouseStrength"), mouseStrength);
  glUniform1f(glGetUniformLocation(applyForceProgram, "mouseRadius"), mouseRadius);
  glUniform1i(glGetUniformLocation(applyForceProgram, "isDown"), isDown ? 1 : 0);


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
    glUseProgram(densityPressureProgram);

    glUniform1f(glGetUniformLocation(densityPressureProgram, "dt"), dt);
    glUniform1f(glGetUniformLocation(densityPressureProgram, "restDensity"), restDensity);
    glUniform1f(glGetUniformLocation(densityPressureProgram, "smoothingRadius"), smoothingRadius);
    glUniform1f(glGetUniformLocation(densityPressureProgram, "delta"), delta);
    glUniform1ui(glGetUniformLocation(densityPressureProgram, "numParticles"), numParticles);

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

    // --- 2b: Apply Pressure Corrections ---
    glUseProgram(updatePositionsProgram);

    glUniform1f(glGetUniformLocation(updatePositionsProgram, "dt"), dt);
    glUniform1f(glGetUniformLocation(updatePositionsProgram, "restDensity"), restDensity);
    glUniform1f(glGetUniformLocation(updatePositionsProgram, "stiffness"), stiffness);
    glUniform1f(glGetUniformLocation(updatePositionsProgram, "smoothingRadius"), smoothingRadius);
    glUniform1ui(glGetUniformLocation(updatePositionsProgram, "numParticles"), numParticles);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, posSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, velSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, pressureSSBO);

    glDispatchCompute(groups, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // 2c: Apply Viscosity Forces
    glUseProgram(applyViscosityProgram);
    glUniform1f(glGetUniformLocation(applyViscosityProgram, "viscosityStrength"), viscosityStrength);
    glUniform1f(glGetUniformLocation(applyViscosityProgram, "smoothingRadius"), smoothingRadius);
    glUniform1ui(glGetUniformLocation(applyViscosityProgram, "numParticles"), numParticles);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, posSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, velSSBO);

    glDispatchCompute(groups, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    iter++;
  }
}
