#include "shaderApp.h"

#include <fstream>
#include <sys/time.h>

#define WRITE_BINARY_FILE 1

#define RENDER_IN_UPDATE 0

#define SHOW_OVERLAY 1
#define SHOW_PREVIEW 1
#define SLEEP_DELAY 1
#define SEND_IMAGE 0
//#define SEND_IMAGE 1
#define WRITE_PPM 0

#define WIDTH     640
#define HEIGHT    480

const int FINTERVAL = 10;
const int BUFFER_LENGTH = 30;
const int FRAMERATE = 10;// FPS
float delay = 1.0/(float)FRAMERATE;// s
float uDelay = delay*1000000.0;// us

const int MODINTERVAL = 1;
const int SEND_MODINTERVAL = 20;

enum messageType {
  START,
  STOP
};

const int MAX_TRANSMIT_SIZE = 1500;
const char* IP_PREFIX = "10";

const char bmp_header_rgb[54] = {
  0x42,0x4D, // "BM"
  0,0,0,0, // SIZE
  0,0,0,0, // unused
  0x36,0,0,0, // 54 bytes from start of file
  0x28,0,0,0, // 40 bytes in DIB header
  0,0,0,0, // WIDTH
  0,0,0,0, // HEIGHT
  1,0,
  0x18,0, // 24 bpp
  0,0,0,0,
  0,0,0,0, // SIZE OF DATA ARRAY (incl padding)
  0x13,0x0B,0,0,
  0x13,0x0B,0,0
};

const char bmp_header_rgba[122] = {
  0x42,0x4D,// "BM"
  00,0xC0,0x12,0, // SIZE
  0,0,0,0, // unused
  0x7A,0,0,0, // 122 bytes from start of file
  0x6C,0,0,0, // 108 bytes in DIB header
  0x80,0x02,0,0, // WIDTH
  0xE0,0x01,0,0, // HEIGHT
  0x01,0,
  0x20,0,
  0x03,0,0,0,
  0,0xC0,0x12,0, // SIZE OF DATA ARRAY (incl padding)
  0x13,0x0B,0,0,
  0x13,0x0B,0,0,
  0,0,0,0,
  0,0,0,0,
  0xFF,0,0,0,
  0,0xFF,0,0,
  0,0,0xFF,0,
  0,0,0,0xFF,
  0x20,0x6E,0x69,0x57
};

static void *write_video_function( void* ptr );
inline double timespec_diff(const struct timespec after, const struct timespec before);
void *get_in_addr(struct sockaddr *sa);
u_short get_in_port(struct sockaddr *sa);
u_long get_my_ip_with_prefix(char* prefix);

//--------------------------------------------------------------
static void *write_video_function( void* ptr ) {
  static int id=0;
  char fname[50];
  shaderApp* app;
  app = (shaderApp *) ptr;
  timespec before,after;
  double diff;

  while ( app->vBuffer.numImages() < app->frameInterval ) {
    usleep(10000);
  }

  while (1) {
#if SLEEP_DELAY
    clock_gettime(CLOCK_REALTIME,&before);
#endif
    while ( app->vBuffer.isEmpty() ) {
      usleep(5000);
    }
#if WRITE_PPM
    sprintf(fname,"img%04d.ppm",id++);
    std::ofstream outfile (fname,std::ofstream::binary);
    sprintf(fname,"P6 %d %d 255 ",WIDTH,HEIGHT);
    outfile.write(fname,strlen(fname));
#else
    sprintf(fname,"img%04d.bmp",id++);
    std::ofstream outfile (fname,std::ofstream::binary);
    outfile.write(bmp_header_rgba,122);
#endif
    outfile.write(app->vBuffer.read(),
		  app->vBuffer.width() * app->vBuffer.width() * app->vBuffer.bytesPerPixel()
		  );
    outfile.close();
#if SEND_IMAGE
    if (id%SEND_MODINTERVAL==0) {
      printf("Camjet: sending image\n");
      app->sendImage(app->vBuffer.read(),
		     app->vBuffer.width() * app->vBuffer.width() * app->vBuffer.bytesPerPixel()
		     );
    }
#endif
    app->vBuffer.remove();
#if SLEEP_DELAY
    clock_gettime(CLOCK_REALTIME,&after);
    diff = timespec_diff(after,before);
    diff *= 1000000.0;
    if ( diff < uDelay )
      usleep(uDelay - diff);
#endif
  }

  return (void *)0;
}

void shaderApp::sendImage(char *img,int size) {
  char tmp[50];
  sprintf(tmp,"START,%d",size);
  int numBytes = 0;
  while ((numBytes = sendto(sockfd, tmp, strlen(tmp),0, 
			    (struct sockaddr *)&(remote_addr), remote_addr_len)) == -1);
  char* ptr = img;
  int bytesLeft = size;
  int numMessages = ceil((double)size/(double)MAX_TRANSMIT_SIZE);
  for (int i=0;i<numMessages;i++) {
    //printf("sending message %d out of %d\n",i,numMessages);
    numBytes = sendto(sockfd, ptr, min(bytesLeft,MAX_TRANSMIT_SIZE),0,
		      (struct sockaddr *)&(remote_addr), remote_addr_len);
    ptr += numBytes;
    bytesLeft = bytesLeft - numBytes;
  }

  sprintf(tmp,"END");
  while ((numBytes = sendto(sockfd, tmp, strlen(tmp),0, 
			    (struct sockaddr *)&(remote_addr), remote_addr_len)) == -1);
}

//--------------------------------------------------------------
int shaderApp::socketSetup() {
  if ( (sockfd = socket(AF_INET, SOCK_DGRAM,0)) < 0 ) {
    printf("Error initializing socket!\n");
    return -1;
  }
  local_port = 9999;
  sprintf(local_ip,IP_PREFIX);
  local_addr.sin_family = AF_INET;
  local_addr.sin_port = htons(local_port);
  local_addr.sin_addr.s_addr = get_my_ip_with_prefix(local_ip);
  if (local_addr.sin_addr.s_addr ==0) {
    printf("can not find matching IP\n");
    return -1;
  }

  if (bind(sockfd,(struct sockaddr *)&local_addr,sizeof(local_addr))<0){
    printf("Error binding\n");
    return -1;
  }

  remote_addr_len = sizeof remote_addr;
  char sock_buffer[50];
  int num_bytes;

  printf("Camjet: waiting for ground station initialization\n");
  if ( (num_bytes = recvfrom(sockfd, sock_buffer, 50, 0, 
			     (struct sockaddr *) &remote_addr, &remote_addr_len) ) == -1 ) {
    printf("Error: recvfrom\n");
    return -1;
  }

  printf("Camjet: got ground station initialization : %s\n",sock_buffer);
  if ((num_bytes = sendto(sockfd, &sock_buffer, strlen(sock_buffer),0, 
			 (struct sockaddr *)&(remote_addr), remote_addr_len)) == -1){
    printf("Error: sendto\n");
    return -1;
  }
  printf("Camjet: done setting up sockets\n");
  return 0;
}

//--------------------------------------------------------------
void shaderApp::setup()
{
  socketSetup();
  
#if SHOW_PREVIEW
  //ofSetupOpenGL(this->width, this->height, OF_WINDOW);
#endif

  ofSetLogLevel(OF_LOG_VERBOSE);
  printf("Camjet: done setting up log level\n");
	
  doDrawInfo	= true;
		
  consoleListener.setup(this);
  printf("Camjet: done setting up console listener\n");

  ofSetPixelStorei( this->width, 8, 4 );

  omxCameraSettings.width = this->width;
  omxCameraSettings.height = this->height;
  omxCameraSettings.framerate = 30;
  omxCameraSettings.isUsingTexture = true;
  videoGrabber.setup(omxCameraSettings);
  printf("Camjet: done setting up camera\n");
	
  threshold = 0.1;

  doShader = true;
	
  //ofEnableAlphaBlending();
  printf("Camjet: done setting up alpha blending\n");
		
  filterCollection.setup(&videoGrabber.omxMaps);
  fbo.allocate(omxCameraSettings.width, omxCameraSettings.height, GL_RGBA);

  printf("Camjet: allocated fbo\n");

  picnum = 0;
  vBuffer.allocate(BUFFER_LENGTH,fbo.getWidth(),fbo.getHeight());
  printf("Camjet: allocated vBuffer\n");
		
  edgeShader.load("edgeShader");
  //distShader.load("distShader");

  fbo.begin();
  ofClear(0, 0, 0, 1);
  fbo.end();

  frameInterval = FINTERVAL;
  pthread_create(&videoThread, NULL, write_video_function, (void *) this);
  printf("Camjet: done with setup\n");
}	

//--------------------------------------------------------------
void shaderApp::update()
{
#if RENDER_IN_UPDATE
  if(!doShader) return;
	
  fbo.begin();

  ofClear(0,0,0,1);
  edgeShader.begin();
  edgeShader.setUniformTexture("tex0", videoGrabber.getTextureReference(), videoGrabber.getTextureID());
  //edgeShader.setUniform1f("time", ofGetElapsedTimef());
  //edgeShader.setUniform2f("resolution", WIDTH, HEIGHT);
  edgeShader.setUniform1f("thresh",threshold);
  edgeShader.setUniform1f("c_xStep",1.0/(double)WIDTH);
  edgeShader.setUniform1f("c_yStep",1.0/(double)HEIGHT);
  videoGrabber.draw();
  edgeShader.end();
  
  if ( !vBuffer.isFull() && ofGetFrameNum() % MODINTERVAL == 0 ) {
    picnum++;
#if USE_FBO_TO_DRAW
    vBuffer.write(&fbo);
#else
    vBuffer.write();
#endif
  }

  fbo.end();	
#endif
}

//--------------------------------------------------------------
void shaderApp::draw(){

#if SHOW_PREVIEW
  if (doShader) {
#if RENDER_IN_UPDATE
    fbo.draw(0, 0);
#else //render in update
    fbo.begin();
    ofClear(0,0,0,1);
    edgeShader.begin();
    edgeShader.setUniformTexture("tex0", videoGrabber.getTextureReference(), videoGrabber.getTextureID());
    //edgeShader.setUniform1f("time", ofGetElapsedTimef());
    //edgeShader.setUniform2f("resolution", WIDTH, HEIGHT);
    edgeShader.setUniform1f("thresh",threshold);
    edgeShader.setUniform1f("c_xStep",1.0/(double)WIDTH);
    edgeShader.setUniform1f("c_yStep",1.0/(double)HEIGHT);
    videoGrabber.draw();
    edgeShader.end();
    //fbo.draw(0,0);
    if ( !vBuffer.isFull() && ofGetFrameNum() % MODINTERVAL == 0 ) {
      picnum++;
#if USE_FBO_TO_DRAW
      vBuffer.write(&fbo);
#else // use fbo to draw
      vBuffer.write();
#endif // use fbo to draw
    }

    fbo.end();	
#endif  // render in update

    fbo.draw(0,0);
  }
  else {
    videoGrabber.draw();
  }
#endif // show preview

#if SHOW_OVERLAY
  stringstream info;
  info << "App FPS: " << ofGetFrameRate() << "\n";
  info << "Written frames : " << picnum << "\n";
  info << "Camera Resolution: " << videoGrabber.getWidth() << "x" << videoGrabber.getHeight()	<< " @ "<< videoGrabber.getFrameRate() <<"FPS"<< "\n";
  info << "CURRENT FILTER: " << filterCollection.getCurrentFilterName() << "\n";
  info << "SHADER ENABLED: " << doShader << "\n";
	
  info << "\n";
  info << "Press e to increment filter" << "\n";
  info << "Press g to Toggle info" << "\n";
  info << "Threshold : " << threshold << "\n";
	
  if (doDrawInfo) {
    ofDrawBitmapStringHighlight(info.str(), 100, 100, ofColor::black, ofColor::yellow);
  }
#endif
}

//--------------------------------------------------------------
void shaderApp::keyPressed  (int key)
{
  ofLogVerbose(__func__) << key;
  static bool stabilize = true;
  if (key == 's') {
    videoGrabber.setFrameStabilization(stabilize);
    stabilize = !stabilize;
  }
  if (key == 'e') {
    videoGrabber.applyImageFilter(filterCollection.getNextFilter());
  }
	
  if (key == 'g') {
    doDrawInfo = !doDrawInfo;
  }
  if (key == 't') {
    threshold += 0.05;
    if (threshold > 1.0)
      threshold = 0.2;
  }
  if (key == 'y') {
    threshold -= 0.05;
    if (threshold <= 0)
      threshold = 1.0;
  }
  if (key == 'd') {
    doShader = !doShader;
  }
	
}

void shaderApp::onCharacterReceived(SSHKeyListenerEventData& e)
{
  keyPressed((int)e.character);
}

void *get_in_addr(struct sockaddr *sa){
  if (sa->sa_family == AF_INET){
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
u_short get_in_port(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return ((struct sockaddr_in*)sa)->sin_port;
  }

  return ((struct sockaddr_in6*)sa)->sin6_port;
}

u_long get_my_ip_with_prefix(char* prefix){

  struct ifaddrs * ifAddrStruct=NULL;
  struct ifaddrs * ifa=NULL;
  void * tmpAddrPtr=NULL;

  getifaddrs(&ifAddrStruct);

  for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
    if (ifa ->ifa_addr->sa_family==AF_INET) { 
      // is a valid IP4 Address
      tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
      char addressBuffer[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
      printf("%s IP v4 Address %s\n", ifa->ifa_name, addressBuffer); 

      if (strncmp(addressBuffer,prefix,strnlen(prefix,15)) == 0)
	return ((struct in_addr *)tmpAddrPtr)->s_addr;
    } else if (ifa->ifa_addr->sa_family==AF_INET6) { // check it is IP6
      // is a valid IP6 Address
      tmpAddrPtr=&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
      char addressBuffer[INET6_ADDRSTRLEN];
      inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
      printf("%s IP v6 Address %s\n ", ifa->ifa_name, addressBuffer); 
      printf("This function does not support IP v6 for now\n"); 
    } 
  }
  if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
  return 0;
}

inline double timespec_diff(const struct timespec after, const struct timespec before) {
  return (double)(after.tv_sec - before.tv_sec)
    + (double)(after.tv_nsec - before.tv_nsec) / 1000000000.0;
}
