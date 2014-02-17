
#pragma once

#include "ofMain.h"
#include "ofAppEGLWindow.h"
#include "ConsoleListener.h"
#include "ofxRPiCameraVideoGrabber.h"
#include "ImageFilterCollection.h"


#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>

#include <pthread.h>

#include "OMXCameraSettings.h"
#include "OMXCameraUtils.h"

using namespace std;

#define USE_FBO_TO_DRAW 1

class imgBuffer {
 
 public:

  imgBuffer() {
    _bytesPerPixel = 4;
    buffer = NULL;
    read_ptr = -1;
    write_ptr = -1;
    buffer_size = -1;
    num_images = 0;
  }

  ~imgBuffer() {
    if (buffer != NULL){
#if USE_FBO_TO_DRAW
#else
      for (int i =0;i<buffer_size;i++) {
	if (buffer[i] != NULL)
	  delete[] buffer[i];
      }
#endif
      delete[] buffer;
    }
  }

  int allocate(int size,int w, int h) {
    try {
#if USE_FBO_TO_DRAW
      buffer = new ofPixels[size]();
#else
      buffer = new char*[size];
#endif
      for (int i=0;i<size;i++) {
#if USE_FBO_TO_DRAW
	if ( _bytesPerPixel == 3 ) {
	  buffer[i].allocate(w,h,OF_PIXELS_RGB);
	}
	else if ( _bytesPerPixel == 4 ) {
	  buffer[i].allocate(w,h,OF_PIXELS_RGBA);
	}
#else
	buffer[i] = new char[w * h * _bytesPerPixel];
#endif
      }
    }
    catch ( ... ) {
      printf("Error: couldn't allocate image buffer of size %d\n",size);
      if (buffer != NULL)
#if USE_FBO_TO_DRAW
	delete buffer;
#else
	delete[] buffer;
#endif
      return -1;
    }
    _width = w;
    _height = h;
    buffer_size = size;
    printf("buffer size : %d\n",buffer_size);
    read_ptr = 0;
    write_ptr = 0;
    num_images = 0;
    return 0;
  }

#if USE_FBO_TO_DRAW
  char* read() { return (char *)buffer[read_ptr].getPixels(); }
#else
  char* read() { return buffer[read_ptr]; }
#endif

  void remove() {
    //printf("read ptr = %d\n",read_ptr);
    read_ptr++;
    if ( read_ptr >= buffer_size ) {
      read_ptr = 0;
    }
    num_images--;
    //printf("exiting remove()\n");
  }

  int write(ofTexture* tex) {
    //printf("write ptr = %d\n",write_ptr);
    tex->readToPixels(buffer[write_ptr]);
    write_ptr++;
    num_images++;
    if ( write_ptr >= buffer_size ) {
      write_ptr = 0;
    }
    return 0;
  }

#if USE_FBO_TO_DRAW
  int write(ofFbo* fbo) {
    //printf("write ptr = %d\n",write_ptr);
    fbo->readToPixels(buffer[write_ptr]);
    write_ptr++;
    num_images++;
    if ( write_ptr >= buffer_size ) {
      write_ptr = 0;
    }
    return 0;
  }
#else
  int write() {
    //printf("write ptr = %d\n",write_ptr);
    if ( _bytesPerPixel == 3 ) {
      glReadPixels(0, 0,
		   _width,_height,
		   GL_RGB,GL_UNSIGNED_BYTE,
		   buffer[write_ptr]);
    }
    else if ( _bytesPerPixel == 4 ) {
      glReadPixels(0, 0,
		   _width,_height,
		   GL_RGBA,GL_UNSIGNED_BYTE,
		   buffer[write_ptr]);
    }
    else {
      printf("Error: Image format unsupported : BPP = %d\n", _bytesPerPixel);
      return -1;
    }
    write_ptr++;
    num_images++;
    if ( write_ptr >= buffer_size ) {
      write_ptr = 0;
    }
    return 0;
  }
#endif

  bool isEmpty() { return num_images <= 0; }

  bool isFull() { return num_images >= buffer_size; }

  int numImages() { return num_images; }
  
  int bytesPerPixel() { return _bytesPerPixel; }
  void bytesPerPixel(int b) { _bytesPerPixel = b; }

  int width() { return _width; }
  int height() { return _height; }

  ofPixels operator[](int i) {
    return buffer[i%buffer_size];
  }
  
 private:
  int _bytesPerPixel;
  int _width,_height;
  int num_images;
  int buffer_size;
  int read_ptr,write_ptr;
#if USE_FBO_TO_DRAW
  ofPixels* buffer;
#else
  char** buffer;
#endif
};

class shaderApp : public ofBaseApp, public SSHKeyListener{

 public:

  shaderApp(int w, int h) {
    width = w;
    height = h;
  }

  void setup();
  void update();
  void draw();
  void keyPressed(int key);

  int socketSetup();
  void sendImage(char* img, int size);

  void onCharacterReceived(SSHKeyListenerEventData& e);
  ConsoleListener consoleListener;
  ofxRPiCameraVideoGrabber videoGrabber;
	
  ImageFilterCollection filterCollection;
	
  bool doDrawInfo;
	
  ofFbo fbo;
  ofShader blurShader;
  ofShader edgeShader;
  ofShader distShader;
  bool doShader;

  float threshold;  // threshold for image detection
  int width;
  int height;

  int picnum;

  int sockfd;
  int local_port;
  int remote_port;
  char local_ip[50];
  char remote_ip[50];
  struct sockaddr remote_addr;
  struct sockaddr_in local_addr;
  socklen_t remote_addr_len;
  socklen_t local_addr_len;
  int sock_buffer_len;

  int frameInterval;  // how many images make up a video?
  imgBuffer vBuffer;  // image buffer for video

  pthread_t imageThread;
  pthread_t videoThread;

  OMXCameraSettings omxCameraSettings;
  //OMXCameraUtils omxCameraUtils;

};

