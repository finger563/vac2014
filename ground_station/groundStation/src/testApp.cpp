#include "testApp.h"

#define RECONNECT_TIME 400
#define IMG_BUFFER_SIZE 10
#define MAXBUFLEN 10240

const char* IP_ADDR = "10.1.1.2";
const int IP_PORT = 9999;

//--------------------------------------------------------------
DWORD WINAPI recvImageFunction( LPVOID lpParam ) {
	testApp* app;
	app = (testApp *)lpParam;
	
	char tmpBuffer[50];

	sprintf(tmpBuffer,"Start Processing");
	app->udpConnection.Send(tmpBuffer,strlen(tmpBuffer));
	printf("Ground Station: Sent Initialization\n");
	app->udpConnection.Receive(tmpBuffer,50);
	printf("Ground Station: Got Response = %s\n",tmpBuffer);

	while (true) {
		app->receiveImage();
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
			printf("Ground Station: image size = %d\n",imgSize);
			if ( imgSize <= 0 ) {
				recvStart = false;
			}
			else {
				while (totalBytesReceived < imgSize && !recvEnd) {
					recvBytes = udpConnection.Receive((char *)data + totalBytesReceived,MAXBUFLEN);
					totalBytesReceived += recvBytes;
					if (!strcmp(tmpBuffer,"END")) {
						printf("Ground Station: image ended with %s\n",tmpBuffer);
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
	printf("Ground Station: Received %d bytes\n",totalBytesReceived);
}

//--------------------------------------------------------------
void testApp::setup(){
	iBuffer.allocate(IMG_BUFFER_SIZE,this->width,this->height);
	
	udpConnection.Create();
	udpConnection.Connect(IP_ADDR, IP_PORT);

	receiverThread = CreateThread( NULL, 0, recvImageFunction, this, 0, &dwThreadID);
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
	//ofDrawBitmapString("VAC Ground Station Monitor", 15, 30);
	if ( iBuffer.numImages() > 1 ) {
		sprintf(fname,"img%05d.png",imgNum);
		iBuffer.getReader()->saveImage(fname);
		iBuffer.incrementReader();
		imgNum++;
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
	//string message="";
	//udpConnection.Send(message.c_str(),message.length());
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
