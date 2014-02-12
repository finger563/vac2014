
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

enum messageType {
  START,
  STOP
};

const int MAX_TRANSMIT_SIZE = 65500;

using namespace std;

class imgBuffer {
 
 public:

  imgBuffer() {
    buffer = NULL;
    read_ptr = -1;
    write_ptr = -1;
    buffer_size = -1;
    num_images = 0;
  }

  ~imgBuffer() {
    if (buffer != NULL){
      for (int i =0;i<buffer_size;i++) {
	if (buffer[i] != NULL)
	  delete[] buffer[i];
      }
      delete[] buffer;
    }
  }

  int allocate(int size,int w, int h) {
    try {
      buffer = new char*[size];
      for (int i=0;i<size;i++) {
	buffer[i] = new char[w * h * 3];
      }
    }
    catch ( ... ) {
      printf("Error: couldn't allocate image buffer of size %d\n",size);
      if (buffer != NULL)
	delete[] buffer;
      return -1;
    }
    width = w;
    height = h;
    buffer_size = size;
    printf("buffer size : %d\n",buffer_size);
    read_ptr = 0;
    write_ptr = 0;
    num_images = 0;
    return 0;
  }

  char* read() {
    return buffer[read_ptr];
  }

  void remove() {
    //printf("read ptr = %d\n",read_ptr);
    read_ptr++;
    if ( read_ptr >= buffer_size ) {
      read_ptr = 0;
    }
    num_images--;
    //printf("exiting remove()\n");
  }

  int write() {
    //printf("write ptr = %d\n",write_ptr);
    glReadPixels(0, 0,
		 width,height,
		 GL_RGB,GL_UNSIGNED_BYTE,
		 buffer[write_ptr]);
    write_ptr++;
    num_images++;
    if ( write_ptr >= buffer_size ) {
      write_ptr = 0;
    }
    //printf("exiting write()\n");
    return 0;
  }

  bool isEmpty() {
    //printf("is empty: %d\n",num_images <= 0);
    return num_images <= 0;
  }

  bool isFull() {
    //printf("is full: %d\n",num_images >= buffer_size);
    return num_images >= buffer_size;
  }

  int numImages() {
    return num_images;
  }
  
 private:
  int width,height;
  int num_images;
  int buffer_size;
  int read_ptr,write_ptr;
  char** buffer;
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
  ofShader shader;
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

