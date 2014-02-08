#include "ofMain.h"
#include "ofGLProgrammableRenderer.h"

//#define use_texturedRemoteApp
//#define use_textureApp
//#define use_nonTextureApp
#define use_shaderApp

int width = 640;
int height = 480;

#ifdef use_texturedRemoteApp
#include "texturedRemoteApp.h"
int main()
{
	ofSetLogLevel(OF_LOG_VERBOSE);
	ofSetCurrentRenderer(ofGLProgrammableRenderer::TYPE);
	ofSetupOpenGL(width, height, OF_WINDOW);
	ofRunApp( new texturedRemoteApp());
}
#endif

#ifdef use_textureApp
#include "textureApp.h"
int main()
{
	ofSetLogLevel(OF_LOG_VERBOSE);
	ofSetCurrentRenderer(ofGLProgrammableRenderer::TYPE);
	ofSetupOpenGL(width, height, OF_WINDOW);
	ofRunApp( new textureApp());
}
#endif

#ifdef use_nonTextureApp
#include "nonTextureApp.h"
int main()
{
	ofSetLogLevel(OF_LOG_VERBOSE);
	ofSetCurrentRenderer(ofGLProgrammableRenderer::TYPE);
	ofSetupOpenGL(width, height, OF_WINDOW);
	ofRunApp( new nonTextureApp());
}
#endif

#ifdef use_shaderApp
#include "shaderApp.h"
int main()
{
	ofSetLogLevel(OF_LOG_VERBOSE);
	ofSetCurrentRenderer(ofGLProgrammableRenderer::TYPE);
	ofSetupOpenGL(width, height, OF_WINDOW);
	ofRunApp( new shaderApp(width,height));
}
#endif
