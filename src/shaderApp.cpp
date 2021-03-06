#include "shaderApp.h"

extern "C" {

#include <IL/OMX_Core.h>
#include <IL/OMX_Component.h>
#include <IL/OMX_Index.h>
#include <IL/OMX_Image.h>
#include <IL/OMX_Video.h>
#include <IL/OMX_Broadcom.h>

#include "bcm_host.h"
#include "ilclient.h"
}

#define WRITE_BINARY_FILE 1
#include <fstream>

#define ENCODE_MULTIPLE_VIDEOS 0
#define SHOW_OVERLAY 1
#define SHOW_PREVIEW 1
#define SLEEP_DELAY 1

#define WIDTH     640
#define PITCH     ((WIDTH+31)&~31)
// PITCH IS 640
#define HEIGHT    480
#define HEIGHT16  ((HEIGHT+15)&~15)
#define SIZE      ((WIDTH * HEIGHT16 * 3)/2)

static void *write_video_function( void* ptr );

const int FINTERVAL = 20;
const int BUFFER_LENGTH = 40;
const int MEGABYTE_IN_BITS = 8388608;
const int FRAMERATE = 10;// FPS
float delay = 1.0/(float)FRAMERATE;// s
float uDelay = delay*1000000.0;// us
//int bitrate = FRAMERATE * SIZE;// * 8 ;// * 1 / 4;// bits/s
int bitrate = MEGABYTE_IN_BITS * 2;

const int MODINTERVAL = 1;

void *get_in_addr(struct sockaddr *sa);
u_short get_in_port(struct sockaddr *sa);
u_long get_my_ip_with_prefix(char* prefix);

static int
fill_buffer_with_image(void *buf, 
		       OMX_U32 *filledLen,
		       char* data
		       ) {
  int i, j;
  char *y = (char *)buf, *u = y + PITCH * HEIGHT16, *v =
    u + (PITCH >> 1) * (HEIGHT16 >> 1);
  //y = 0
  //u = y + 640*480
  //v = u + 320*240
  int i0,i1,i2,i3;

  for (j=0;j < HEIGHT; j+=2) {
    for (i=0;i < WIDTH; i+=2) {
      i0 = i + j*WIDTH;  // 0
      i1 = i0 + 1;         // 1
      i2 = i0 + WIDTH;   // WIDTH
      i3 = i2 + 1;         // WIDTH + 1
      y[i0] = data[i0 * 3];
      y[i1] = data[i1 * 3];
      y[i2] = data[i2 * 3];
      y[i3] = data[i3 * 3];

      v[i/2 + j/2 * WIDTH/2] = (char)((
			 (int)data[i0 * 3 + 1] +
			 (int)data[i1 * 3 + 1] +
			 (int)data[i2 * 3 + 1] +
			 (int)data[i3 * 3 + 1] ) / 4);
      u[i/2 + j/2 * WIDTH/2] = (char)((
			 (int)data[i0 * 3 + 2] +
			 (int)data[i1 * 3 + 2] +
			 (int)data[i2 * 3 + 2] +
			 (int)data[i3 * 3 + 2] ) / 4);
    }
  }
  //printf("finished buffer copy\n");
  *filledLen = SIZE;  
  return 1; 
}

//--------------------------------------------------------------
void fill_input_buffer_done(void* data, COMPONENT_T* comp) {
}

//--------------------------------------------------------------
void empty_input_buffer_done(void* data, COMPONENT_T* comp) {
}

//--------------------------------------------------------------
void fill_output_buffer_done(void* data, COMPONENT_T* comp) {
}

//--------------------------------------------------------------
void empty_output_buffer_done(void* data, COMPONENT_T* comp) {
}

//--------------------------------------------------------------
static void *write_video_function( void* ptr ) {
  static int id=0;
  char fname[50];
  shaderApp* app;
  app = (shaderApp *) ptr;

#if WRITE_BINARY_FILE
  while (1) {
    // printf("Waiting for images!\n",fname);
    while ( app->vBuffer.numImages() < app->frameInterval ) {
      usleep(10000);
    }
    sprintf(fname,"img%04d.ppm",id++);
    //printf("Writing %s\n",fname);
    std::ofstream outfile (fname,std::ofstream::binary);
    sprintf(fname,"P6 %d %d 255 ",WIDTH,HEIGHT);
    outfile.write(fname,strlen(fname));
    outfile.write(app->vBuffer.read(),WIDTH*HEIGHT*3);
    outfile.close();
    app->vBuffer.remove();
    usleep(uDelay);
  }

#else
  OMX_VIDEO_PARAM_PORTFORMATTYPE format;
  OMX_PARAM_PORTDEFINITIONTYPE def;
  OMX_BUFFERHEADERTYPE *buf;
  OMX_BUFFERHEADERTYPE *out;
  OMX_ERRORTYPE error;

  COMPONENT_T *video_encode = NULL;
  COMPONENT_T *list[5];
  ILCLIENT_T *client;

  int status = 0;
  int framenumber = 0;
  FILE *outf;

  memset(list, 0, sizeof(list));

  if ((client = ilclient_init()) == NULL) {
    return (void *)-3;
  }

  // create video_encode
  error = (OMX_ERRORTYPE)ilclient_create_component(client, &video_encode, "video_encode",
				   (ILCLIENT_CREATE_FLAGS_T)(ILCLIENT_DISABLE_ALL_PORTS |
				    ILCLIENT_ENABLE_INPUT_BUFFERS |
				    ILCLIENT_ENABLE_OUTPUT_BUFFERS));
  if (error!= 0) {
    printf
      ("ilclient_create_component() for video_encode failed with %x!\n",
       error);
    return (void *)-1;
  }
  list[0] = video_encode;

  // get current settings of video_encode component from port 200
  memset(&def, 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
  def.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
  def.nVersion.nVersion = OMX_VERSION;
  def.nPortIndex = 200;

  if (OMX_GetParameter
      (ILC_GET_HANDLE(video_encode), OMX_IndexParamPortDefinition,
       &def) != OMX_ErrorNone) {
    printf("%s:%d: OMX_GetParameter() for video_encode port 200 failed!\n",
	   __FUNCTION__, __LINE__);
    return (void *)-1;
  }

  // Port 200: in 1/1 115200 16 enabled,not pop.,not cont. 320x240 320x240 @1966080 20
  def.format.video.nFrameWidth = app->width;//WIDTH;
  def.format.video.nFrameHeight = app->height;//HEIGHT;
  def.format.video.xFramerate = FRAMERATE << 16;//30 << 16;
  def.format.video.nSliceHeight = def.format.video.nFrameHeight;
  def.format.video.nStride = def.format.video.nFrameWidth;
  def.format.video.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
   
  error= OMX_SetParameter(ILC_GET_HANDLE(video_encode),
		       OMX_IndexParamPortDefinition, &def);
  if (error!= OMX_ErrorNone) {
    printf
      ("%s:%d: OMX_SetParameter() for video_encode port 200 failed with %x!\n",
       __FUNCTION__, __LINE__, error);
    return (void *)-1;
  }

  memset(&format, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
  format.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
  format.nVersion.nVersion = OMX_VERSION;
  format.nPortIndex = 201;
  format.eCompressionFormat = OMX_VIDEO_CodingAVC;

  printf("OMX_SetParameter for video_encode:201...\n");
  error= OMX_SetParameter(ILC_GET_HANDLE(video_encode),
		       OMX_IndexParamVideoPortFormat, &format);
  if (error!= OMX_ErrorNone) {
    printf
      ("%s:%d: OMX_SetParameter() for video_encode port 201 failed with %x!\n",
       __FUNCTION__, __LINE__, error);
    return (void *)-1;
  }

  OMX_VIDEO_PARAM_BITRATETYPE bitrateType;
  // set current bitrate to 1Mbit -> 2 MBps
  memset(&bitrateType, 0, sizeof(OMX_VIDEO_PARAM_BITRATETYPE));
  bitrateType.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
  bitrateType.nVersion.nVersion = OMX_VERSION;
  bitrateType.eControlRate = OMX_Video_ControlRateVariable;
  bitrateType.nTargetBitrate = bitrate;
  bitrateType.nPortIndex = 201;
  error= OMX_SetParameter(ILC_GET_HANDLE(video_encode),
                       OMX_IndexParamVideoBitrate, &bitrateType);
  if (error!= OMX_ErrorNone) {
    printf
      ("%s:%d: OMX_SetParameter() for bitrate for video_encode port 201 failed with %x!\n",
       __FUNCTION__, __LINE__, error);
    return (void *)-1;
  }

  // get current bitrate
  memset(&bitrateType, 0, sizeof(OMX_VIDEO_PARAM_BITRATETYPE));
  bitrateType.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
  bitrateType.nVersion.nVersion = OMX_VERSION;
  bitrateType.nPortIndex = 201;

  if (OMX_GetParameter
      (ILC_GET_HANDLE(video_encode), OMX_IndexParamVideoBitrate,
       &bitrateType) != OMX_ErrorNone) {
    printf("%s:%d: OMX_GetParameter() for video_encode for bitrate port 201 failed!\n",
	   __FUNCTION__, __LINE__);
    return (void *)-1;
  }
  printf("Current Bitrate=%u\n",bitrateType.nTargetBitrate);

  printf("encode to idle...\n");
  if (ilclient_change_component_state(video_encode, OMX_StateIdle) == -1) {
    printf
      ("%s:%d: ilclient_change_component_state(video_encode, OMX_StateIdle) failed",
       __FUNCTION__, __LINE__);
  }

  printf("enabling port buffers for 200...\n");
  if (ilclient_enable_port_buffers(video_encode, 200, NULL, NULL, NULL) != 0) {
    printf("enabling port buffers for 200 failed!\n");
    return (void *)-1;
  }

  printf("enabling port buffers for 201...\n");
  if (ilclient_enable_port_buffers(video_encode, 201, NULL, NULL, NULL) != 0) {
    printf("enabling port buffers for 201 failed!\n");
    return (void *)-1;
  }

  printf("encode to executing...\n");
  ilclient_change_component_state(video_encode, OMX_StateExecuting);

#if ENCODE_MULTIPLE_VIDEOS
  while ( 1 ) {
#endif
    framenumber = 0;

    while ( app->vBuffer.numImages() < app->frameInterval) {
      //printf("%d , %d\n",app->vBuffer.numImages(), app->frameInterval);
      usleep(50000);
    }

    sprintf(fname,"vid%04d.h264",id++);
  
    outf = fopen(fname, "w");
    if (outf == NULL) {
      printf("Failed to open '%s' for writing video\n", fname);
      return (void *)-1;
    }

    printf("looping for buffers...\n");
    do {
      while ( app->vBuffer.isEmpty() );
      //printf("getting next buffer\n");
      // GET INPUT BUFFER TO OMX VIDEO_ENCODE COMPONENT
      buf = ilclient_get_input_buffer(video_encode, 200, 1);
      if (buf == NULL) {
	printf("Doh, no buffers for me!\n");
      }
      else {
	//printf("alloc = %d\n",buf->nAllocLen);
	// FILL INPUT BUFFER WITH IMAGE DATA
	fill_buffer_with_image(buf->pBuffer, &buf->nFilledLen, app->vBuffer.read());
	framenumber++;
	app->vBuffer.remove();
	// TELL OMX TO GET THE DATA FROM THE BUFFER
	if (OMX_EmptyThisBuffer(ILC_GET_HANDLE(video_encode), buf) !=
	    OMX_ErrorNone) {
	  printf("Error emptying buffer!\n");
	}
	// GET THE OUTPUT BUFFER FROM THE OMX VIDEO_ENCODE COMPONENT
	out = ilclient_get_output_buffer(video_encode, 201, 1);
	// FILL THE BUFFER WITH OMX OUTPUT DATA
	error = OMX_FillThisBuffer(ILC_GET_HANDLE(video_encode), out);
	if (error != OMX_ErrorNone) {
	  printf("Error filling buffer: %x\n", error);
	}

	if (out != NULL) {
	  if (out->nFlags & OMX_BUFFERFLAG_CODECCONFIG) {
	    int i;
	    for (i = 0; i < out->nFilledLen; i++)
	      printf("%x ", out->pBuffer[i]);
	    printf("\n");
	  }

	  error = (OMX_ERRORTYPE)fwrite(out->pBuffer, 1, out->nFilledLen, outf);
	  if (error != out->nFilledLen) {
	    printf("fwrite: Error emptying buffer: %d!\n", error);
	  }
	  else {
	    printf("Writing frame %d\n", framenumber);
	  }
	  out->nFilledLen = 0;
	}
	else {
	  printf("Not getting it :(\n");
	}
#if SLEEP_DELAY
	usleep((int)uDelay);
#endif
      }
    }
#if ENCODE_MULTIPLE_VIDEOS
    while (framenumber < app->frameInterval);
#else
    while (true);
#endif

    fclose(outf);

#if ENCODE_MULTIPLE_VIDEOS
  }
#endif

  printf("Teardown.\n");

  printf("disabling port buffers for 200 and 201...\n");
  ilclient_disable_port_buffers(video_encode, 200, NULL, NULL, NULL);
  ilclient_disable_port_buffers(video_encode, 201, NULL, NULL, NULL);

  ilclient_state_transition(list, OMX_StateIdle);
  ilclient_state_transition(list, OMX_StateLoaded);

  ilclient_cleanup_components(list);

  OMX_Deinit();

  ilclient_destroy(client);

#endif

  return (void *)0;
}

void shaderApp::sendImage(char *img,int size) {
  char tmp[10];
  sprintf(tmp,"%d",START);
  int numBytes = 0;
  while ((numBytes = sendto(sockfd, tmp, strlen(tmp),0, 
			    (struct sockaddr *)&(remote_addr), remote_addr_len)) == -1);
  char* ptr = img;
  int bytesLeft = size;
  int numMessages = ceil((double)size/(double)MAX_TRANSMIT_SIZE);
  for (int i=0;i<numMessages;i++) {
    numBytes = sendto(sockfd, ptr, min(bytesLeft,MAX_TRANSMIT_SIZE),0,
		      (struct sockaddr *)&(remote_addr), remote_addr_len);
    ptr += numBytes;
    bytesLeft = bytesLeft - numBytes;
  }

  sprintf(tmp,"%d",STOP);
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
  sprintf(local_ip,"192");
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
  int num_bytes, recv_bytes = 0;

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

  ofSetLogLevel(OF_LOG_VERBOSE);
	
  doDrawInfo	= true;
		
  consoleListener.setup(this);

  omxCameraSettings.width = this->width;
  omxCameraSettings.height = this->height;
  omxCameraSettings.framerate = 30;
  omxCameraSettings.isUsingTexture = true;
  videoGrabber.setup(omxCameraSettings);
  videoGrabber.setLEDStatus(false);
	
  threshold = 0.1;

  doShader = true;
	
  ofEnableAlphaBlending();
		
  filterCollection.setup(&videoGrabber.omxMaps);
  fbo.allocate(omxCameraSettings.width, omxCameraSettings.height, GL_RGBA);

  picnum = 0;
  vBuffer.allocate(BUFFER_LENGTH,fbo.getWidth(),fbo.getHeight());
		
  shader.load("shaderExample");

  fbo.begin();
  ofClear(0, 0, 0, 0);
  fbo.end();

  frameInterval = FINTERVAL;
  int ret = pthread_create(&videoThread, NULL, write_video_function, (void *) this);
}	

//--------------------------------------------------------------
void shaderApp::update()
{

  if(!doShader) return;
	
  fbo.begin();

  ofClear(0,0,0,0);
  shader.begin();
  shader.setUniformTexture("tex0", videoGrabber.getTextureReference(), videoGrabber.getTextureID());
  shader.setUniform1f("time", ofGetElapsedTimef());
  shader.setUniform2f("resolution", ofGetWidth(), ofGetHeight());
  shader.setUniform1f("thresh",threshold);
  shader.setUniform1f("c_xStep",1.0/ofGetWidth());
  shader.setUniform1f("c_yStep",1.0/ofGetHeight());
  videoGrabber.draw();
  shader.end();
  
  if ( !vBuffer.isFull() && ofGetFrameNum() % MODINTERVAL == 0 ) {
    //printf("writing frame %d\n",picnum);
    picnum++;
    vBuffer.write();
  }

  fbo.end();

  //printf("exiting update\n");
	
}


//--------------------------------------------------------------
void shaderApp::draw(){

#if SHOW_PREVIEW
  if (doShader) {
    fbo.draw(0, 0);
  }
  else {
    videoGrabber.draw();
  }
#endif

#if SHOW_OVERLAY
  stringstream info;
  info << "App FPS: " << ofGetFrameRate() << "\n";
  //info << "Camera frameCounter: " << videoGrabber.frameCounter << "\n";
  //info << "App frameCounter: " << ofGetFrameNum() << "\n";
  info << "Written frames : " << picnum << "\n";
  info << "Camera Resolution: " << videoGrabber.getWidth() << "x" << videoGrabber.getHeight()	<< " @ "<< videoGrabber.getFrameRate() <<"FPS"<< "\n";
  info << "CURRENT FILTER: " << filterCollection.getCurrentFilterName() << "\n";
  info << "SHADER ENABLED: " << doShader << "\n";
  //info <<	filterCollection.filterList << "\n";
	
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
