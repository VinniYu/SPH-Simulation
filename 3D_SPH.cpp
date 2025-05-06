#include <iostream>
#include <ctime>

#include "PARTICLE_3D.h"

using namespace std;

// forward declarations
void runOnce();
void runEverytime();

// Text for the title bar of the window
string windowLabel("3D SPH");
// the resolution of the OpenGL window
int xScreenRes = 1440; 
int yScreenRes = 900;

// simulation 
Parallel *sim;

// animate the current runEverytime()?
bool animate = true;

// current zoom level into the field
float zoom = 1.0;

///////////////////////////////////////////////////////////////////////
// GL and GLUT callbacks
///////////////////////////////////////////////////////////////////////
void glutDisplay()
{
  runEverytime();
}

///////////////////////////////////////////////////////////////////////
// Map the arrow keys to something here
///////////////////////////////////////////////////////////////////////
void glutSpecial(int key, int x, int y)
{
  switch (key) {
  case GLUT_KEY_LEFT:
    sim->rotateCamLeft();
    break;
  case GLUT_KEY_RIGHT:
    sim->rotateCamRight();
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
  case 'd':
    break;
  case 'r': 
    sim->resetParticles();
    break;
  case 'l': 
    if (sim->doObstacle) sim->doObstacle = false;
    else if (!sim->doObstacle) {
      sim->setObject(glm::vec3(-1.0f, 1.0f, 0.0f));
      sim->doObstacle = true;
    }
    break;
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
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE);

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
  glLineWidth(0.5f);

  // for particles
  glEnable(GL_PROGRAM_POINT_SIZE);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  if (animate) {
    if (sim->doObstacle) sim->loopObject();
    sim->compute();
  }

  // Always render particles and objects
  sim->render();
  
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

  int numParticles = 48841;

  sim = new Parallel(numParticles);

  // initialize the simulation
  sim->initSimBounds();
  sim->initObject();
  sim->initParticleAndPrograms();
  sim->initRenderer("render3d/bound_vert.glsl" , "render3d/bound_frag.glsl",
                    "render3d/sphere_vert.glsl" , "render3d/sphere_frag.glsl");
}
  