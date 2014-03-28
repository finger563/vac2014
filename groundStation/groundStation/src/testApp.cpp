#include "testApp.h"

#define IMG_BUFFER_SIZE 10
#define MAXBUFLEN 10240

#define WRITE_IMAGES 0

const char* IP_ADDR = "10.1.1.2";
const int IP_PORT = 9999;

//--------------------------------------------------------------
static void *recvImageFunction( void* lpParam ) {
	testApp* app;
	app = (testApp *)lpParam;
	
	char tmpBuffer[50];
	int imgNum = 0;

	sprintf(tmpBuffer,"Start Processing");
	app->udpConnection.Send(tmpBuffer,strlen(tmpBuffer));
	printf("Ground Station: Sent Initialization\n");
	app->udpConnection.Receive(tmpBuffer,50);
	printf("Ground Station: Got Response = %s\n",tmpBuffer);

	while (true) {
		app->receiveImage();
		printf("Ground Station: Received image %d\n",imgNum++);
	}
}

//--------------------------------------------------------------
void testApp::receiveImage() {
	unsigned char *data;
	char tmpBuffer[MAXBUFLEN];
	memset(tmpBuffer,0,MAXBUFLEN);
	int imgSize = 0;
	int recvBytes = 0;
	bool recvEnd = false;
	int totalBytesReceived = 0;

	bool recvStart = false;
	while ( !recvStart ) {
		while ( (recvBytes = udpConnection.Receive(tmpBuffer,MAXBUFLEN) ) <= 0 );
		char *p = strtok(tmpBuffer,",");
		if ( p != NULL && !strcmp(p,"START") ){
			recvStart = true;
			p = strtok(NULL,","); // want second argument
			imgSize = atoi(p);
			data = new unsigned char[imgSize];
			//printf("Ground Station: image size = %d\n",imgSize);
			if ( imgSize <= 0 ) {
				recvStart = false;
			}
			else {
				while (totalBytesReceived < imgSize && !recvEnd) {
					recvBytes = udpConnection.Receive((char *)data + totalBytesReceived,MAXBUFLEN);
					totalBytesReceived += recvBytes;
					if (!strcmp(tmpBuffer,"END")) {
						//printf("Ground Station: image ended with %s\n",tmpBuffer);
						recvEnd = true;
					}
				}
			}
		}
	}
	for (int i=0;i<imgSize;i++)
		iBuffer.getWriter()->getPixels()[i] = data[i];
	delete data;
	iBuffer.incrementWriter();
	//printf("Ground Station: Received %d bytes\n",totalBytesReceived);
}

//--------------------------------------------------------------
void testApp::setup(){
	iBuffer.allocate(IMG_BUFFER_SIZE,this->width,this->height);
	
	udpConnection.Create();
	udpConnection.Connect(IP_ADDR, IP_PORT);
	//udpConnection.SetTimeoutReceive(5);
	//udpConnection.SetTimeoutSend(1);

	pthread_create( &receiverThread, NULL, recvImageFunction, (void *) this);
}

//--------------------------------------------------------------
void testApp::update(){
	if ( !iBuffer.isEmpty() )
		iBuffer.getReader()->update();
}

//--------------------------------------------------------------
void testApp::draw(){
	static int imgNum = 0;
	char fname[50];
	if ( !iBuffer.isEmpty() )
		iBuffer.getReader()->draw(0,0);
	ofDrawBitmapString("VAC Ground Station Monitor", 15, 30);
	if ( iBuffer.numImages() > 1 ) {
#if WRITE_IMAGES
		sprintf(fname,"img%05d.png",imgNum);
		iBuffer.getReader()->saveImage(fname);
		imgNum++;
#endif
		iBuffer.incrementReader();
	}
}

//--------------------------------------------------------------
void testApp::exit(){
}


//--------------------------------------------------------------
void testApp::keyPressed(int key){

}

//--------------------------------------------------------------
void testApp::keyReleased(int key){


}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){
}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){
}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){
}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){ 

}
