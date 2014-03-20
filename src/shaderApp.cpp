#include "shaderApp.h"

#include <fstream>
#include <sys/time.h>

#define SHOW_OVERLAY 0
#define SHOW_PREVIEW 0
#define SLEEP_DELAY 1
#define SEND_IMAGE 1
#define SEND_SIZE (640*480*3)

const int SOCK_TIMEOUT_SEC = 10;  // timeout for establishing connection with ground station
const int FINTERVAL = 10;
const int BUFFER_LENGTH = 30;
const int FRAMERATE = 20;// FPS
float delay = 1.0/(float)FRAMERATE;// s
float uDelay = delay*1000000.0;// us

const int SEND_MODINTERVAL = FRAMERATE / 5;

const int MAX_TRANSMIT_SIZE = 10240;
const char* IP_PREFIX = "10";

static void *write_video_function( void* ptr );
static void *send_image_function( void* ptr );

inline double timespec_diff(const struct timespec after, const struct timespec before);
void *get_in_addr(struct sockaddr *sa);
u_short get_in_port(struct sockaddr *sa);
u_long get_my_ip_with_prefix(char* prefix);

//--------------------------------------------------------------
static void *send_image_function( void* ptr ) {
  shaderApp* app;
  app = (shaderApp *) ptr;

  int size = app->sendSize;
  char * rImg = app->vBuffer.read();

  char *buff = new char[size];
  int stride = app->vBuffer.size() / size;
  int rPos = 0;
  for (int wPos = 0; wPos < size; wPos++) {
    buff[wPos] = rImg[rPos];
    rPos += stride;
  }

  //printf("Camjet: sending image\n");
  app->sendImage(buff, size);
  delete buff;
  app->readyToSend = true;
}

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
    std::ofstream outfile;
    sprintf(fname,"img%04d.ppm",id++);
    outfile.open(fname,std::ofstream::binary);
    sprintf(fname,"P6 %d %d 255 ",app->vBuffer.width(),app->vBuffer.height());
    outfile.write(fname,strlen(fname));
    outfile.write(app->vBuffer.read(),
		  app->vBuffer.size());
    outfile.close();
#if SEND_IMAGE
    if ( app->readyToSend ) {
      app->readyToSend = false;
      pthread_create(&app->sendThread, NULL, send_image_function, (void *) app);
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
  if ( (sockfd = socket(AF_INET, SOCK_DGRAM,0)) < 0 ) {
    printf("Camjet: ERROR - initializing socket!\n");
    return;
  }
#if 1
  if (bind(sockfd,(struct sockaddr *)&local_addr,sizeof(local_addr))<0){
    printf("Camjet: ERROR - binding\n");
    return;
  }
#endif
  char tmp[50];
  sprintf(tmp,"START,%d",size);
  int numBytes = 0;
  sendto(sockfd, tmp, strlen(tmp),0, 
			    (struct sockaddr *)&(remote_addr), remote_addr_len);
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
  sendto(sockfd, tmp, strlen(tmp),0, 
			    (struct sockaddr *)&(remote_addr), remote_addr_len);
  
  close(sockfd);
}

//--------------------------------------------------------------
int shaderApp::socketSetup() {
  local_port = 9999;
  sprintf(local_ip,IP_PREFIX);
  local_addr.sin_family = AF_INET;
  local_addr.sin_port = htons(local_port);
  local_addr.sin_addr.s_addr = get_my_ip_with_prefix(local_ip);
  if (local_addr.sin_addr.s_addr ==0) {
    printf("Camjet: ERROR - can not find matching IP\n");
    return -1;
  }

  bool initialized = false;

  remote_addr_len = sizeof remote_addr;
  char sock_buffer[50];
  int num_bytes;
  
  while ( !initialized ) {
    memset(sock_buffer,0,50);
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM,0)) < 0 ) {
      printf("Camjet: ERROR - initializing socket!\n");
    }
    else {
      if (bind(sockfd,(struct sockaddr *)&local_addr,sizeof(local_addr))<0){
        printf("Camjet: ERROR - binding\n");
      }
      else {
        struct timeval tv;
        tv.tv_sec = SOCK_TIMEOUT_SEC;
        tv.tv_usec = 0;
        if ( setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0 ) {
          printf("Camjet: ERROR - set recv timeout\n");
        }
        else {
          printf("Camjet: waiting for ground station initialization\n");
          if ( (num_bytes = recvfrom(sockfd, sock_buffer, 50, 0, 
                   (struct sockaddr *) &remote_addr, &remote_addr_len) ) == -1 ) {
            printf("Camjet: ERROR - recvfrom\n");
          }
          else {
            printf("Camjet: got ground station initialization : %s\n",sock_buffer);
            if ((num_bytes = sendto(sockfd, &sock_buffer, strlen(sock_buffer),0, 
                 (struct sockaddr *)&(remote_addr), remote_addr_len)) == -1){
              printf("Camjet: ERROR - sendto\n");
            }
            else {
              printf("Camjet: done setting up sockets\n");
              initialized = true;
            }
          }
        }
      }
    }
    close(sockfd);
  }
  
#if 0
  if ( (sockfd = socket(AF_INET, SOCK_DGRAM,0)) < 0 ) {
    printf("Camjet: ERROR - initializing socket!\n");
    return -1;
  }
#endif
  return 0;
}

//--------------------------------------------------------------
void shaderApp::setup()
{
#if SEND_IMAGE
  if ( socketSetup() )
    ofExit(-1);
#endif

  ofSetLogLevel(OF_LOG_VERBOSE);
  printf("Camjet: done setting up log level\n");
	
  doDrawInfo	= true;
		
  consoleListener.setup(this);
  printf("Camjet: done setting up console listener\n");

  //ofSetPixelStorei( this->width, 8, 4 );

  omxCameraSettings.width = this->width;
  omxCameraSettings.height = this->height;
  omxCameraSettings.framerate = 30;
  omxCameraSettings.isUsingTexture = true;
  videoGrabber.setup(omxCameraSettings);
  printf("Camjet: done setting up camera\n");
	
  threshold = 0.15;

  doShader = true;

  ofEnableAlphaBlending();	
  printf("Camjet: done setting up alpha blending\n");
		
  filterCollection.setup(&videoGrabber.omxMaps);
  fbo.allocate(omxCameraSettings.width, omxCameraSettings.height, GL_RGBA);

  printf("Camjet: allocated fbo\n");

  picnum = 0;
  vBuffer.bytesPerPixel(3);
  vBuffer.allocate(BUFFER_LENGTH,fbo.getWidth(),fbo.getHeight());
  printf("Camjet: allocated vBuffer\n");
		
  edgeShader.load("edgeShader");
  passThrough.load("passThrough");
  distShader.load("distShader");
  printf("Camjet: Loaded shaders\n");

  fbo.begin();
  ofClear(0, 0, 0, 0);
  fbo.end();

  frameInterval = FINTERVAL;
  pthread_create(&videoThread, NULL, write_video_function, (void *) this);
  sendSize = SEND_SIZE;
  readyToSend = true;
  printf("Camjet: done with setup\n");
}	

//--------------------------------------------------------------
void shaderApp::update(){
  static timespec previous = {0,0}, now = {0,0};
  static double diff = -1;
  static bool process = true;

  fbo.begin();
  ofClear(0,0,0,0);

  clock_gettime(CLOCK_REALTIME,&now);
  diff = timespec_diff(now,previous);

  if ( process ) {
    edgeShader.begin();
    edgeShader.setUniformTexture("tex0", videoGrabber.getTextureReference(), videoGrabber.getTextureID());
    edgeShader.setUniform1f("thresh",threshold);
    edgeShader.setUniform1f("c_xStep",1.0/(double)width);
    edgeShader.setUniform1f("c_yStep",1.0/(double)height);
    videoGrabber.draw();
    edgeShader.end();
#if 0
    int NUM_SHADER_ITERATIONS = 7;
    for (int i=0; i < NUM_SHADER_ITERATIONS; i++) {
      fbo.end();
      fbo.begin();
      distShader.begin();
      //distShader.setUniformTexture("tex0", fbo.getTextureReference(), fbo.getDefaultTextureIndex());
      fbo.draw(0,0);
      distShader.setUniform1f("c_xStep",1.0/(double)width);
      distShader.setUniform1f("c_yStep",1.0/(double)height);
      distShader.end();
    }
#endif
  }
  else {
    passThrough.begin();
    passThrough.setUniformTexture("tex0", videoGrabber.getTextureReference(), videoGrabber.getTextureID());
    videoGrabber.draw();
    passThrough.end();
  }
  
  if ( !vBuffer.isFull() && diff >= delay ) {
    process = !process;
    clock_gettime(CLOCK_REALTIME,&previous);
    picnum++;
    vBuffer.write();
  }

  fbo.end();
}

//--------------------------------------------------------------
void shaderApp::draw(){

#if SHOW_PREVIEW
  if (doShader) {
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
#endif // show overlay
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
