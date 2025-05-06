#include <iostream>
#include <ctime>

#include "PARTICLE_2D.h"

using namespace std;

// forward declarations
void runOnce();
void runEverytime();

// Text for the title bar of the window
string windowLabel("2D SPH");

int xScreenRes = 1024; 
int yScreenRes = 768;

int isDown = false;
int forceType = 1;
float mouseX = 0.0f;
float mouseY = 0.0f;

// simulation
Parallel *sim;

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
  sim->injectForce((float)x, (float)(yScreenRes - y), isDown, forceType);
}

void glutMouse(int button, int state, int x, int y) {
  float px = (float)x;
  float py = (float)(yScreenRes - y);  // Flip Y to match OpenGL bottom-left origin

  if (button == GLUT_LEFT_BUTTON) {
    isDown = (state == GLUT_DOWN);
    forceType = -1;
    sim->injectForce(px, py, isDown, forceType);
  }

  if (button == GLUT_RIGHT_BUTTON) {
    isDown = (state == GLUT_DOWN);
    forceType = 1;
    sim->injectForce(px, py, isDown, forceType);
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
  case ' ':
    animate = !animate;
    break;
  case 'c':
    break;
  case 'o':
    sim->showObstacle = !sim->showObstacle;
    break;
  case 'l':
    
    break;
  case 'r': 
    sim->resetParticles();
    break;
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
  glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  // for bounding box
  glEnable(GL_LINE_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
  glEnable(GL_MULTISAMPLE);
  glLineWidth(3.0f);

  // for particles
  glEnable(GL_PROGRAM_POINT_SIZE);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Update simulation
  if (animate) sim->compute();
  
  sim->render();

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

  int numParticles = 10000;
  // numParticles = 4032;
  // numParticles = 1024;

  sim = new Parallel(numParticles);

  // initialize the simulation
  sim->initParticlesAndProgram();
  sim->initObject();
  sim->initRenderer("render2d/fluid_vert.glsl", "render2d/fluid_frag.glsl");
}
  
