#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){

	ofSetFrameRate(60);
	ofBackground(0,0,0);  
	XML.loadFile("settings.xml");
	loadSettings();

	firstVideo.loadMovie("videos/Map_Argenteuil_P1_v11.mov");
	firstVideo.setLoopState(OF_LOOP_NONE);
	secondVideo.loadMovie("videos/Map_Argenteuil_P2_v11.mov");
	secondVideo.setLoopState(OF_LOOP_NONE);
	firstVideo.play();
	video = &firstVideo;
	// video->setFrame(7000);
	speed = 1;

	//kinect instructions
	kinect.init();
	kinect.setRegistration(true);
	kinect.open();
	depthImage.allocate(kinect.width, kinect.height);
	depthBackground.allocate(kinect.width, kinect.height);

	// Hand display
	font.loadFont("fonts/AltoPro-Normal.ttf", 12);

	currentPhase = 0;
	XML.pushTag("PHASEINFORMATION");
	nextPhaseFrame = XML.getValue("PHASE:STARTFRAME", 1200, currentPhase+1);
	XML.popTag();

	string shaderProgram = "#version 120\n \
	#extension GL_ARB_texture_rectangle : enable\n \
	\
	uniform sampler2DRect tex0;\
	uniform sampler2DRect maskTex;\
	\
	void main (void){\
	vec2 pos = gl_TexCoord[0].st;\
	\
	vec3 src = texture2DRect(tex0, pos).rgb;\
	float mask = texture2DRect(maskTex, pos).r;\
	\
	gl_FragColor = vec4( src , mask);\
	}";
	shader.setupShaderFromSource(GL_FRAGMENT_SHADER, shaderProgram);
	shader.linkProgram();
	fbo.allocate(video->getWidth(), video->getHeight());
	maskFbo.allocate(video->getWidth(), video->getHeight());
	// Apparently I need to clear these too
	maskFbo.begin();
		ofClear(0,0,0,255);
	maskFbo.end();
	fbo.begin();
		ofClear(0,0,0,255);
	fbo.end();
	brushImg.loadImage("brush.png");

}

//--------------------------------------------------------------
void ofApp::update(){

	kinect.update();
	adjustPhase();

	if(kinect.isFrameNew()) {

		depthImage.setFromPixels(kinect.getDepthPixels(), kinect.width, kinect.height);
		if(bLearnBackground) {
			depthBackground = depthImage;
			bLearnBackground = false;
		}
		// Background subtraction
		depthImage -= depthBackground;
		// Remove out of bounds
		unsigned char *pix = depthImage.getPixels();
		for (int i = 0; i < depthImage.getHeight() * depthImage.getWidth(); ++i)
		{
			if(pix[i] > nearThreshold || pix[i] < farThreshold)
				pix[i] = 0;
			else
				pix[i] = 255;
		}

		ContourFinder.findContours(depthImage);
		ContourFinder.update();
	}

	if(currentPhase == 5) {
		maskFbo.begin();
			ofPushMatrix();
			ofTranslate(-video_x, -video_y);
			ofScale(video->getWidth() / video_w, video->getHeight() / video_h);
			ofRotateZ(-video_r);
				drawHandMask(ofColor(255,255,255,100), false);
			ofPopMatrix();
		maskFbo.end();

		fbo.begin();
			ofClear(0,0,0,0);
			shader.begin();
				shader.setUniformTexture("maskTex", maskFbo.getTextureReference(), 1);
				secondVideo.draw(0,0);
			shader.end();
		fbo.end();
	}

}

//--------------------------------------------------------------
void ofApp::draw(){

	ofPushMatrix();
		ofRotateZ(video_r);
		video->draw(video_x, video_y, video_w, video_h);
		if(currentPhase == 5)
			fbo.draw(video_x, video_y, video_w, video_h);
	ofPopMatrix();

	drawHandMask(ofColor(0,0,0));

	// if(currentPhase == 1 || currentPhase == 2 || currentPhase == 6 or currentPhase == 7 or currentPhase == 8)
		drawHandText();

	if(bDisplayFeedback)
		drawFeedback();

}

void ofApp::adjustPhase() {

	video->update();
	XML.pushTag("PHASEINFORMATION");

	int frame = video->getCurrentFrame();
	if( frame >= nextPhaseFrame) { // Change phase
		currentPhase++;
		if(currentPhase >= 10)
			currentPhase = 0;
		nextPhaseFrame = XML.getValue("PHASE:STARTFRAME", nextPhaseFrame + 1000, currentPhase + 1);
		int v = XML.getValue("PHASE:VIDEO", 1, currentPhase);
		if( v == 1 )
			video = &firstVideo;
		if( v == 2 )
			video = &secondVideo;
		if(currentPhase == 5) {
			secondVideo.play();
		}
		if(currentPhase == 6) {
			video = &secondVideo;
			firstVideo.stop();
			firstVideo.setFrame(0);
		}
		if(currentPhase == 0) {
			secondVideo.stop();
			secondVideo.setFrame(0);
			firstVideo.play();
			video = &firstVideo;
		}
	}
	XML.popTag();

	// Phase 5 is the magical phase
	if(currentPhase == 5) {
		secondVideo.update();
	}

}

void ofApp::drawHandMask(ofColor color, bool bDrawArms) {

	ofPushStyle();
	ofSetColor(color);
	ofFill();
	ofPushMatrix();
	ofTranslate(kinect_x, kinect_y);
	ofScale(kinect_z, kinect_z);

	if(bDrawArms) {
	for (int i = 0; i < ContourFinder.size(); ++i)
	{
		ofPolyline blob = ContourFinder.getPolyline(i);
		blob = blob.getSmoothed(4);

		ofBeginShape();
			for (int j = 0; j < blob.size(); ++j) {
				ofVertex(blob[j]);
			}
		ofEndShape();
	}
	}
	else{
		for (int i = 0; i < ContourFinder.hands.size(); ++i)
		{
			ofPolyline blob = ContourFinder.hands[i].line;
			blob = blob.getSmoothed(4);

			ofBeginShape();
				for (int j = 0; j < blob.size(); ++j) {
					ofVertex(blob[j]);
				}
			ofEndShape();
		}
	}

	ofPopMatrix();
	ofPopStyle();

}

void ofApp::drawHandText() {

	for (int i = 0; i < ContourFinder.hands.size(); ++i)
	{
		string palmText = "hand";
		ofPoint center 	= ContourFinder.hands[i].centroid;
		ofPoint tip 	= ContourFinder.hands[i].tip;
		int side 		= ContourFinder.hands[i].side;
		// Transform to proper reference frame
		center.x = center.x * kinect_z + kinect_x;
		center.y = center.y * kinect_z + kinect_y;
		tip.x = tip.x * kinect_z + kinect_x;
		tip.y = tip.y * kinect_z + kinect_y;

		ofPushMatrix();
			ofTranslate(center.x, center.y);
			// Proper rotation
			float h = sqrt( pow(center.x - tip.x, 2) + pow(center.y - tip.y, 2) );
			float angle =  ofRadToDeg( asin( (tip.y - center.y) / h ));
			if(tip.x < center.x) angle *= -1;
			ofRotateZ(angle);
			ofPoint textCenter = font.getStringBoundingBox(palmText, 0, 0).getCenter();
			float width = font.getStringBoundingBox(palmText, 0, 0).getWidth();
			ofTranslate(-textCenter.x, -textCenter.y);
			float size = ofDist(tip.x, tip.y, center.x, center.y);
			ofScale(size/width*0.75, size/width*0.75);
			font.drawString(palmText, 0, 0);
		ofPopMatrix();
	}

}

void ofApp::drawFeedback() {

	ofPushStyle();
	depthImage.draw(0,0);
	ofSetColor(0,255,0);
	ContourFinder.draw();

	ofSetColor(255,0,255);
	for (int i = 0; i < ContourFinder.hands.size(); ++i)
	{
		ofCircle(ContourFinder.hands[i].end, 3);
		ofCircle(ContourFinder.hands[i].tip, 3);
		ofCircle(ContourFinder.hands[i].wrists[0], 3);
		ofCircle(ContourFinder.hands[i].wrists[1], 3);
	}

	stringstream reportStream;
	reportStream
	<< "nearThreshold: " << nearThreshold << endl
	<< "farThreshold: " << farThreshold << endl
	<< "frame: " << video->getCurrentFrame() << endl
	<< "currentPhase: " << currentPhase << endl
	<< "nextPhaseFrame: " << nextPhaseFrame << endl
	// << "speed: " << speed << endl
	<< "framerate: " << ofToString(ofGetFrameRate()) << endl;
	if  ( ContourFinder.size() == 1 ) {
		ofRectangle rect = ofxCv::toOf(ContourFinder.getBoundingRect(0));
		ofPoint min = rect.getMin();
		ofPoint max = rect.getMax();
		reportStream 
		<< "minx: " << min.x << endl
		<< "miny: " << min.y << endl
		<< "maxx: " << max.x << endl
		<< "maxy: " << max.y << endl;
	}

	ofDrawBitmapString(reportStream.str(), 20, 600);
	ofPopStyle();

}

void ofApp::loadSettings() {

	XML.pushTag(ofToString(PLATFORM));
		video_x = XML.getValue("VIDEO:X", 0);
		video_y = XML.getValue("VIDEO:Y", 0);
		video_w = XML.getValue("VIDEO:W", firstVideo.getWidth());
		video_h = XML.getValue("VIDEO:H", firstVideo.getHeight());
		video_r = XML.getValue("VIDEO:R", 0);

		kinect_x = XML.getValue("KINECT:X", 0);
		kinect_y = XML.getValue("KINECT:Y", 0);
		kinect_z = XML.getValue("KINECT:Z", 2.77);
	XML.popTag();

	ContourFinder.setMinArea(XML.getValue("CV:MINAREA", 1000));


	farThreshold = XML.getValue("KINECT:FARTHRESHOLD", 2);
	nearThreshold = XML.getValue("KINECT:NEARTHRESHOLD", farThreshold + 40);

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

	switch(key) {

		case OF_KEY_DOWN: 
			speed *= 0.9;
			firstVideo.setSpeed(speed);
			secondVideo.setSpeed(speed);
			break;

		case OF_KEY_UP: 
			speed *= 1.1;
			firstVideo.setSpeed(speed);
			secondVideo.setSpeed(speed);
			break;

		// case OF_KEY_LEFT: 
		// 	x--;
		// 	break;

		// case OF_KEY_RIGHT: 
		// 	x++;
		// 	break;

		// case OF_KEY_UP: 
		// 	y--;
		// 	break;

		// case OF_KEY_DOWN: 
		// 	y++;
		// 	break;

		// case 'W': 
		// 	w++;
		// 	h = w / video.getWidth() * video.getHeight();
		// 	break;

		// case 'w': 
		// 	w--;
		// 	h = w / video.getWidth() * video.getHeight();
		// 	break;

		// case 'H': 
		// 	h++;
		// 	break;

		// case 'h': 
		// 	h--;
		// 	break;

		// case 'R': 
		// 	r+= 0.1;
		// 	break;

		// case 'r': 
		// 	r-= 0.1;
		// 	break;

		// case 'Z': 
		// 	z += 0.01;
		// 	break;

		// case 'z': 
		// 	z -= 0.01;
		// 	break;

		case '>':
		case '.':
			farThreshold ++;
			if (farThreshold > 255) farThreshold = 255;
			break;
			
		case '<':
		case ',':
			farThreshold --;
			if (farThreshold < 0) farThreshold = 0;
			break;
			
		case '+':
		case '=':
			nearThreshold ++;
			if (nearThreshold > 255) nearThreshold = 255;
			break;
			
		case '-':
			nearThreshold --;
			if (nearThreshold < 0) nearThreshold = 0;
			break;

		case 'f':
			bDisplayFeedback = !bDisplayFeedback;
			break;

		case ' ': 
			if(video->isPaused())
				video->setPaused(false);
			else
				video->setPaused(true);
			break;

		case OF_KEY_RETURN:
			bLearnBackground = true;
			break;

		// case 's': {
		// 	XML.saveFile("settings.xml");
		// 	cout << "Settings saved!";
		// 	break;
		// }
	}

}
//--------------------------------------------------------------
void ofApp::mousePressed(int mx, int my, int button){
}
