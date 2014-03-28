#pragma once

#include "ofxNetwork.h"
#include "ofMain.h"

#include <pthread.h>

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
      delete[] buffer;
    }
  }

  int allocate(int size, int w, int h) {
    try {
      buffer = new ofImage[size];
	  for (int i=0;i<size;i++) {
		  buffer[i].allocate(w,h,OF_IMAGE_COLOR);
	  }
    }
    catch ( ... ) {
      printf("Error: couldn't allocate image buffer of size %d\n",size);
      if (buffer != NULL)
		delete[] buffer;
      return -1;
    }
    buffer_size = size;
    read_ptr = 0;
    write_ptr = 0;
    num_images = 0;
    return 0;
  }

  ofImage* getReader() { 
	  return &buffer[read_ptr]; 
  }

  ofImage* getWriter() {
	  return &buffer[write_ptr];
  }

  void incrementWriter() {
    write_ptr++;
    if ( write_ptr >= buffer_size ) {
      write_ptr = 0;
    }
    num_images++;
  }
  void incrementReader() {
    read_ptr++;
    if ( read_ptr >= buffer_size ) {
      read_ptr = 0;
    }
    num_images--;
  }

  bool isEmpty() { return num_images <= 0; }

  bool isFull() { return num_images >= buffer_size; }

  int numImages() { return num_images; }
  
 private:
  int num_images;
  int buffer_size;
  int read_ptr,write_ptr;
  ofImage* buffer;
};

static void *recvImageFunction( void* lpParam );

class testApp : public ofBaseApp{

	public:
		testApp(int w, int h){
			width = w; height = h;
		}

		void setup();
		void update();
		void draw();
		void exit();
		
		void receiveImage();
		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);		

		ofxUDPManager udpConnection;

		int width,height;
		imgBuffer iBuffer;

		pthread_t receiverThread;

		ofTrueTypeFont  mono;
		ofTrueTypeFont  monosm;
		//vector<ofPoint> stroke;
};

