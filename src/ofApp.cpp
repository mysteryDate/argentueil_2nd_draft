#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){

	ofSetFrameRate(60);
	ofBackground(0,0,0);  
	loadSettings();

	video.loadMovie("videos/Map_Argenteuil_P1_v11.mov");
	video.play();

	//kinect instructions
	kinect.init();
	kinect.setRegistration(true);
	kinect.open();
	depthImage.allocate(kinect.width, kinect.height);
	depthBackground.allocate(kinect.width, kinect.height);

	nearThreshold = 255;

}

//--------------------------------------------------------------
void ofApp::update(){

	kinect.update();
	video.update();

	if(kinect.isFrameNew()) {

		depthImage.setFromPixels(kinect.getDepthPixels(), kinect.width, kinect.height);
		if(bLearnBackground) {
			depthBackground = depthImage;
			bLearnBackground = false;
		}
		// Take out too close pixels
		depthImage -= depthBackground;
		depthImage.threshold(5);

		ContourFinder.findContours(depthImage);
		contours = utility::transform(ContourFinder, x, y, z);
	}

}

//--------------------------------------------------------------
void ofApp::draw(){

	ofPushMatrix();
		ofRotateZ(video_r);
		video.draw(video_x, video_y, video_w, video_h);
	ofPopMatrix();

	ofPushMatrix();
		ofScale(z, z);
		ofTranslate(x, y);
		depthImage.draw(0,0);
	ofPopMatrix();

	ofPushStyle();
		ofSetColor(0,255,0);
		ContourFinder.draw();
	ofPopStyle();

	// ofPushStyle();
	// 	ofSetColor(255,255,255);
	// 	for (int i = 0; i < contours.size(); ++i)
	// 	{
	// 		ofBeginShape();
	// 		ofPolyline line = contours[i];
	// 		for (int j = 0; j < line.size(); ++j)
	// 		{
	// 			ofVertex(line[j]);
	// 		}
	// 		ofEndShape();
	// 	}
	// ofPopStyle();

}

void ofApp::loadSettings() {

	XML.loadFile("settings.xml");
	XML.pushTag(ofToString(PLATFORM));
		video_x = XML.getValue("VIDEO:X", 0);
		video_y = XML.getValue("VIDEO:Y", 0);
		video_w = XML.getValue("VIDEO:W", video.getWidth());
		video_h = XML.getValue("VIDEO:H", video.getHeight());
		video_r = XML.getValue("VIDEO:R", 0);

		x = XML.getValue("KINECT:X", 0);
		y = XML.getValue("KINECT:Y", 0);
		z = XML.getValue("KINECT:Z", 2.77);
	XML.popTag();

	nearThreshold = XML.getValue("KINECT:NEARTHRESHOLD", 255);

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

	switch(key) {

		case OF_KEY_LEFT: 
			x--;
			break;

		case OF_KEY_RIGHT: 
			x++;
			break;

		case OF_KEY_UP: 
			y--;
			break;

		case OF_KEY_DOWN: 
			y++;
			break;

		case 'W': 
			w++;
			h = w / video.getWidth() * video.getHeight();
			break;

		case 'w': 
			w--;
			h = w / video.getWidth() * video.getHeight();
			break;

		case 'H': 
			h++;
			break;

		case 'h': 
			h--;
			break;

		case 'R': 
			r+= 0.1;
			break;

		case 'r': 
			r-= 0.1;
			break;

		case 'Z': 
			z += 0.01;
			break;

		case 'z': 
			z -= 0.01;
			break;

		case ' ':
			if(video.isPaused()) 
				video.setPaused(false);
			else
				video.setPaused(true);

		case OF_KEY_RETURN:
			bLearnBackground = true;

		case 's': {
			if (XML.getNumTags("MAC") == 0)
				XML.addTag("MAC");
			XML.pushTag("MAC");
				if (XML.getNumTags("KINECT") == 0)
					XML.addTag("KINECT");
				XML.setValue("KINECT:IMGX", x);
				XML.setValue("KINECT:IMGY", y);
				XML.setValue("KINECT:IMGZ", z);
			XML.popTag();
			XML.saveFile("settings.xml");
			cout << "Settings saved!";
		}
	}

}
//--------------------------------------------------------------
void ofApp::mousePressed(int mx, int my, int button){

}
