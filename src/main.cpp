#include "ofMain.h"
#include "ofGLProgrammableRenderer.h"
#include "ofAppNoWindow.h"

#include "shaderApp.h"

int width = 640;
int height = 480;

int main()
{
  //ofAppNoWindow window;
  ofSetLogLevel(OF_LOG_VERBOSE);
  ofSetCurrentRenderer(ofGLProgrammableRenderer::TYPE);
  ofSetupOpenGL(width, height, OF_WINDOW);
  //ofSetupOpenGL(&window, width, height, OF_WINDOW);
  ofRunApp( new shaderApp(width,height));
}
