// Argenteuil final
#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){

	ofSetFrameRate(60);
	ofBackground(0,0,0);  
	XML.loadFile("settings.xml");

	firstVideo.loadMovie("videos/Argenteuille_edit_p1_V12.mov");
	secondVideo.loadMovie("videos/Argenteuille_edit_p2_v12.mov");
	firstVideo.setLoopState(OF_LOOP_NONE);
	secondVideo.setLoopState(OF_LOOP_NONE);

	loadSettings();
	currentPhase = START_PHASE - 1;
	XML.pushTag("PHASEINFORMATION");
		nextPhaseFrame = XML.getValue("PHASE:STARTFRAME", nextPhaseFrame + 1000, currentPhase + 1);
		int v = XML.getValue("PHASE:VIDEO", 1, currentPhase + 1);
		if(v == 1)
			video = &firstVideo;
		if(v == 2)
			video = &secondVideo;
		video->setFrame(nextPhaseFrame - 1);
	XML.popTag();
	speed = 1;

	//kinect instructions
	kinect.init();
	kinect.open();
	kinectImg.allocate(kinect.width, kinect.height);
	kinectBackground.allocate(kinect.width, kinect.height);

	// Hand display
	font.loadFont("fonts/AltoPro-Normal.ttf", 12);
	bHandText = false;
	minVelocity = 1;

	// Set up beavers for phase 4
	XML.pushTag("BEAVERS");
	numBeaverFrames = XML.getValue("NUMFRAMES", 24);
	float beaverScale = XML.getValue("IMGSCALE", 0.2);
	for (int i = 0; i < numBeaverFrames; ++i)
	{	
		ofImage img;
		img.loadImage("beaver/beaver-"+ofToString(i)+".png");
		img.resize(img.getWidth()*beaverScale, img.getHeight()*beaverScale);
		gifFrames.push_back(img);
	}
	XML.popTag();

	//for water ripples
	ofEnableAlphaBlending();
	ripples.allocate(video->getWidth(), video->getHeight());
	bounce.allocate(video->getWidth(), video->getHeight());
	// Animated mask
	ofDirectory dir("masks/animated_mask");
	dir.allowExt("png");
	dir.listDir();
	for (int i = 0; i < dir.numFiles(); ++i)
	{
		string name = dir.getName(i);
		ofImage img;
		img.loadImage("masks/animated_mask/"+name);
		name = name.substr(0,4);
		int frame = std::atoi(name.c_str());
		animatedMask[frame] = img;
	}
	maskNumber = 0;
	riverMask.loadImage("masks/river_mask_v3.png");
	bRipple = false;

	// Setting up shaders for phase 5
	shader.setupShaderFromFile(GL_FRAGMENT_SHADER, "shadersGL2/shader.frag");
	shader.linkProgram();
	fbo.allocate(video->getWidth(), video->getHeight());
	maskFbo.allocate(video->getWidth(), video->getHeight());
	maskFbo.begin();
		ofClear(0,0,0,255);
	maskFbo.end();
	fbo.begin();
		ofClear(0,0,0,0);
	fbo.end();

	handImage.loadImage("hand_images/Gare_IMG_7769.JPG");
	screensaver.loadMovie("videos/Argenteuille_ScreenSaver_v1.mov");
    screensaver.setFrame(0);
    screensaver.update();
	lastTime = ofGetSystemTimeMicros();
	bScreensaver = false;
}	

//--------------------------------------------------------------
void ofApp::update(){

	kinect.update();
	adjustPhase();

	if(kinect.isFrameNew()) {

		kinectImg.setFromPixels(kinect.getDepthPixels(), kinect.width, kinect.height);
		if(bLearnBackground) {
			kinectBackground = kinectImg;
			bLearnBackground = false;
		}
		// Background subtraction
		kinectImg -= kinectBackground;
		// Remove out of bounds
		unsigned char *pix = kinectImg.getPixels();
		for (int i = 0; i < kinectImg.getHeight() * kinectImg.getWidth(); ++i)
		{
			if(pix[i] > nearThreshold || pix[i] < farThreshold)
				pix[i] = 0;
			else
				pix[i] = 255;
		}

		input = ofxCv::toCv(kinectImg);
		cv::Rect crop_roi = cv::Rect(crop_left, crop_top, 
			kinect.width - crop_left - crop_right,
			kinect.height - crop_top - crop_bottom);
		croppedInput = input(crop_roi).clone();

		ContourFinder.findContours(croppedInput);
		if(bHandText || currentPhase == 9)
			ContourFinder.update();
		else
			ContourFinder.hands.clear();

		if(ContourFinder.size() > 0) {
			if(bScreensaver) {
				screensaverEndTime = ofGetSystemTimeMicros();
				maskFbo.begin();
					ofClear(0,0,0,255);
				maskFbo.end();
			}
			bScreensaver = false;
			lastTime = ofGetSystemTimeMicros();
		}
	}

	unsigned long long idleTime = (ofGetSystemTimeMicros() - lastTime)/1000;
	if(idleTime >= (screensaverTime - 2000) && !bScreensaver) { // Screensaver fade
		maskFbo.begin();
			// For a fade
			float alpha = ofMap(screensaverTime - idleTime, 2000, 0, 0, 255);
			ofPushStyle();
			ofSetColor(255,255,255,round(alpha));
			ofRect(0,0,video->getWidth(), video->getHeight());
			ofPopStyle();
		maskFbo.end();

		fbo.begin();
			ofClear(0,0,0,0);
			shader.begin();
				shader.setUniformTexture("maskTex", maskFbo.getTextureReference(), 1);
				screensaver.update();
				screensaver.draw(0,0);
			shader.end();
		fbo.end();
	}
	if(idleTime >= screensaverTime && !bScreensaver) {
		bScreensaver = true;
		startScreensaver();
	}
	if(screensaverEndTime - ofGetSystemTimeMicros() < 1000) {
		maskFbo.begin();
			// For a fade
			float alpha = ofMap(screensaverEndTime - ofGetSystemTimeMicros(), 0, 1000, 255, 0);
			ofPushStyle();
			ofSetColor(255,255,255,round(alpha));
			ofRect(0,0,video->getWidth(), video->getHeight());
			ofPopStyle();
		maskFbo.end();

		fbo.begin();
			ofClear(0,0,0,0);
			shader.begin();
				shader.setUniformTexture("maskTex", maskFbo.getTextureReference(), 1);
				screensaver.update();
				screensaver.draw(0,0);
			shader.end();
		fbo.end();
	}
	if(screensaverEndTime - ofGetSystemTimeMicros() > 1000 && !bScreensaver) {
		screensaver.stop();
		screensaver.setFrame(0);
	}

	if(!bScreensaver) {
	// Water ripples on 1,2,3,4,7,8
	if(bRipple) 
		updateRipples();

	// Beaver phase
	if(currentPhase == 4) {
		if(ofGetFrameNum() % 30 == 0 && Beavers.size() < 120) {
			Critter newBeaver = Critter(numBeaverFrames); 
			Beavers.push_back(newBeaver);
		}
	}
	if(Beavers.size() > 0)
		updateBeavers();

	// Transition phase
	if(currentPhase == 5) {
		maskFbo.begin();
			ofPushMatrix();
			ofTranslate(-video_x, -video_y);
			ofScale(video->getWidth() / video_w, video->getHeight() / video_h);
			ofRotateZ(-video_r);
				drawHandMask(ofColor(255,255,255,127), true);
			ofPopMatrix();
			// For a fade
			float alpha = ofMap((nextPhaseFrame - video->getCurrentFrame()), 100, 0, 0, 10);
			ofPushStyle();
			ofSetColor(255,255,255,round(alpha));
			ofRect(0,0,video->getWidth(), video->getHeight());
			ofPopStyle();
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

}

//--------------------------------------------------------------
void ofApp::draw(){

	if(bRipple)
		bounce.draw(video_x, video_y, video_w, video_h);
	else { 
		video->draw(video_x, video_y, video_w, video_h);
	}
	fbo.draw(video_x, video_y, video_w, video_h);


	if(bHandText)
		drawHandMask(ofColor(0,0,0));
		drawHandText();

	drawBeavers();

	// Picture phase
	if(currentPhase == 9) {
		drawHandImages();
	}

	if(bDisplayFeedback)
		drawFeedback();

}

//--------------------------------------------------------------
// Custom functions
//--------------------------------------------------------------
void ofApp::startScreensaver() {
	maskFbo.begin();
		ofClear(0,0,0,255);
	maskFbo.end();
	fbo.begin();
		ofClear(0,0,0,0);
	fbo.end();
	video->stop();
	video->setFrame(0);
	video = &screensaver;
	bRipple = false;
	currentPhase = -1;
	nextPhaseFrame = -1;
	video->play();
	video->update();
	video->setLoopState(OF_LOOP_NORMAL);
}

void ofApp::adjustPhase() {

	video->update();

	XML.pushTag("PHASEINFORMATION");

	int frame = video->getCurrentFrame();
	if( frame >= nextPhaseFrame && !bScreensaver) { // Change phase
		maskFbo.begin();
			ofClear(0,0,0,255);
		maskFbo.end();
		fbo.begin();
			ofClear(0,0,0,0);
		fbo.end();
		currentPhase++;
		if(currentPhase >= 10)
			currentPhase = 0;
		if(currentPhase == 0) {
			secondVideo.stop();
			secondVideo.setFrame(0);
			firstVideo.play();
			video = &firstVideo;
		}
		if(currentPhase == 5) {
			secondVideo.setFrame(1300);
			secondVideo.update();
		}
		if(currentPhase == 6) {
			secondVideo.play();
			video = &secondVideo;
			firstVideo.stop();
			firstVideo.setFrame(0);
		}

		if(XML.getValue("PHASE:RIPPLE", 0, currentPhase))
			bRipple = true;
		else
			bRipple = false;

		if(XML.getValue("PHASE:HANDTEXT", 0, currentPhase))
			bHandText = true;
		else
			bHandText = false;

		nextPhaseFrame = XML.getValue("PHASE:STARTFRAME", nextPhaseFrame + 1000, currentPhase + 1);
		farThreshold = XML.getValue("PHASE:FARTHRESHOLD", 2, currentPhase);
		nearThreshold = XML.getValue("PHASE:NEARTHRESHOLD", 42, currentPhase);
		updateRegions();
	}
	XML.popTag();
}

void ofApp::updateRegions() {

	regions.clear();
	XML.pushTag("PHASE", currentPhase);
	if(XML.getNumTags("REGIONS") > 0) {
		XML.pushTag("REGIONS");
		for (int i = 0; i < XML.getNumTags("REGION"); ++i)
		{
			XML.pushTag("REGION", i);
			string name = XML.getValue("NAME", "unknown");
			regions.insert(pair<string, ofPolyline>(name, ofPolyline()));
			for (int j = 0; j < XML.getNumTags("PT"); ++j)
			{
				int x = XML.getValue("PT:X", 0, j);
				int y = XML.getValue("PT:Y", 0, j);
				regions[name].addVertex(ofPoint(x,y));
			}
			XML.popTag();
		}
	    XML.popTag();
	}
    XML.popTag();

}

void ofApp::updateRipples() {

	bounce.setTexture(video->getTextureReference(), 1);
	int frameDiff = nextPhaseFrame - video->getCurrentFrame();
	if(currentPhase != 0)
		ripples.damping = ofMap(frameDiff, 100, 0, 0.995, 0, true);
	// Water ripples
	ripples.begin();
		ofPushStyle();
		ofPushMatrix();
			ofFill();
			// Put the data reference frame into the untransformed video reference
			ofTranslate(-video_x, -video_y);
			ofScale(video->getWidth() / video_w,  video->getHeight() / video_h);
			ofRotateZ(-video_r);
			drawHandMask(ofColor(255,255,255), !bHandText, true);
			drawBeavers();

		ofPopMatrix();

		if(currentPhase == 0) {
            int frame = video->getCurrentFrame();
			while(true) {
				if( animatedMask.find(frame) != animatedMask.end() ) {
					animatedMask[frame].draw(0,0, video->getWidth(), video->getHeight());
					break;
				}
				frame--;
				if(frame < 0) {
					ofPushStyle();
					ofSetColor(0,0,0);
					ofFill();
					ofRect(0,0, video->getWidth(), video->getHeight());
					ofPopStyle();
                    break;
				}
			}
		}
		else
			riverMask.draw(0,0);
		ofPopStyle();
	ripples.end();
	
	ripples.update();
	bounce << ripples;
}

void ofApp::updateBeavers() {

	float Iw = gifFrames[0].getWidth();
	float Ih = gifFrames[0].getHeight();
	float beaverScaleUp = 1.4; // makes them easier to catch

	for (int i = 0; i < Beavers.size(); ++i)
	{
		// Find vector to closest hand
		ofVec2f nearestHand = ofVec2f(0,0);
		float minDist = INFINITY; 

		for (int j = 0; j < ContourFinder.size(); ++j)
		{
			ofPoint center = utility::transform(ContourFinder.getPolyline(j).getCentroid2D(), kinect_x, kinect_y, kinect_z);
			float dist = ofDistSquared(Beavers[i].p.x, Beavers[i].p.y, center.x, center.y);
			if(dist < minDist) {
				minDist = dist;
				nearestHand = ofVec2f(center.x - Beavers[i].p.x, center.y - Beavers[i].p.y);
			}
		}
		Beavers[i].update(nearestHand);
		Critter * B = &Beavers[i];
		ofPolyline corners;
		for (int j = 0; j < 4; ++j)
		{
			int xd = (j & 1)*2 - 1;
			int yd = (j & 2) - 1;
			float x = B->p.x + beaverScaleUp*cos(B->d*PI/180)*Iw*xd/2 + beaverScaleUp*sin(B->d*PI/180)*Ih*yd/2;
			float y = B->p.y - beaverScaleUp*sin(B->d*PI/180)*Iw*xd/2 + beaverScaleUp*cos(B->d*PI/180)*Ih*yd/2;
			corners.addVertex(ofPoint(x,y));
		}
		// Beaver has left the building
		if(B->p.x > ofGetWindowWidth() + Iw or B->p.x < 0 - Iw or B->p.y > ofGetWindowHeight() + Iw or B->p.y < 0 - Iw)
				Beavers.erase(Beavers.begin() + i);
		// Collision
		B->previousFrames.resize(5);
		B->previousFrames.insert(B->previousFrames.begin(), B->hidden);
		B->hidden = false; 
		for (int j = 0; j < ContourFinder.size(); ++j)
		{
			ofPolyline line = utility::transform(ContourFinder.getPolyline(j), kinect_x, kinect_y, kinect_z);
			for (int k = 0; k < line.size(); ++k)
			{
				if(corners.inside(line[k].x, line[k].y)) {
					B->v = 10;
					B->hidden = true;
					continue;
				}
			}
		}

	}
}

void ofApp::drawBeavers() {

	for (int i = 0; i < Beavers.size(); ++i)
	{		
		Critter CB = Beavers[i];
		bool draw = true;		
		for (int j = 0; j < CB.previousFrames.size(); ++j)
		{
			if(CB.previousFrames[j]) // Hidden was true
				draw = false;
		}
		if(!CB.hidden and draw) {
			ofPushMatrix();
	        ofTranslate(CB.p.x, CB.p.y); // Translate to the center of the beaver
			ofRotateZ(CB.d);
				gifFrames[CB.currentFrame].setAnchorPercent(0.5,0.5); // So that we draw from the middle
				gifFrames[CB.currentFrame].draw(0,0);
			ofPopMatrix();
		}
	}

}
// third argument scales the color based on hand velocity
void ofApp::drawHandMask(ofColor color, bool bDrawArms, bool scale) {

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
			if(scale) {
				float vel = ContourFinder.hands[i].velocity.length();
				float s = ofMap(vel, 1, 8, 0, 1, true);
				color.r *= s;
				color.g *= s;
				color.b *= s;
			}
			ofSetColor(color);
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

void ofApp::drawHandImages() {

	// maskFbo.begin();
	// 	ofClear(0,0,0,255);
	// 	ofPushMatrix();
	// 		// ofTranslate(-video_x, -video_y);
	// 		// ofScale(video->getWidth() / video_w, video->getHeight() / video_h);
	// 		// ofRotateZ(-video_r);
	// 		drawHandMask(ofColor(255,255,255), false);
	// 	ofPopMatrix();
	// maskFbo.end();

	// fbo.begin();
	// 	ofClear(0,0,0,0);
	// fbo.end();

	for (int i = 0; i < ContourFinder.hands.size(); ++i)
	{
		unsigned int label = ContourFinder.hands[i].label;
		stableHands[label].resize(2);

		ofPoint center 	= ContourFinder.hands[i].centroid;
		ofPoint tip 	= ContourFinder.hands[i].tip;
		int side 		= ContourFinder.hands[i].side;
		if( ContourFinder.hands[i].velocity.length() < minVelocity ) {
			center = stableHands[label][0];
			tip = stableHands[label][1];
		}
		else {
			stableHands[label][0] = center;
			stableHands[label][1] = tip;
		}
		// Transform to proper reference frame
		center.x = center.x * kinect_z + kinect_x;
		center.y = center.y * kinect_z + kinect_y;
		tip.x = tip.x * kinect_z + kinect_x;
		tip.y = tip.y * kinect_z + kinect_y;

		// ofImage img = handImage;

		// fbo.begin();
		// shader.begin();
		// shader.setUniformTexture("maskTex", maskFbo.getTextureReference(), 1);
		ofPushMatrix();
			ofTranslate(center.x, center.y);
			// Proper rotation
			float h = sqrt( pow(center.x - tip.x, 2) + pow(center.y - tip.y, 2) );
			float angle =  ofRadToDeg( asin( (tip.y - center.y) / h ));
			if(tip.x < center.x) angle *= -1;
			if (side == 1) angle += 180;
			if ( (side == 0 or side == 2 ) and tip.y < center.y ) 
				angle += 180;
			ofRotateZ(angle);


			float size = ofDist(tip.x, tip.y, center.x, center.y) * 2;
			int imgWidth = handImage.getWidth();
			ofScale(size/imgWidth, size/imgWidth);
			handImage.setAnchorPercent(0.5,0.5);
			handImage.draw(0,0);
		ofPopMatrix();
		// shader.end();
		// fbo.end();

	}
	// fbo.draw(video_x, video_y, video_w, video_h);
	// fbo.draw(0,0);
}

void ofApp::drawHandText() {

	for (int i = 0; i < ContourFinder.hands.size(); ++i)
	{
		string palmText;
		unsigned int label = ContourFinder.hands[i].label;
		stableHands[label].resize(2);

		ofPoint center 	= ContourFinder.hands[i].centroid;
		ofPoint tip 	= ContourFinder.hands[i].tip;
		int side 		= ContourFinder.hands[i].side;
		if( ContourFinder.hands[i].velocity.length() < minVelocity ) {
			center = stableHands[label][0];
			tip = stableHands[label][1];
		}
		else {
			stableHands[label][0] = center;
			stableHands[label][1] = tip;
		}
		// Transform to proper reference frame
		center.x = center.x * kinect_z + kinect_x;
		center.y = center.y * kinect_z + kinect_y;
		tip.x = tip.x * kinect_z + kinect_x;
		tip.y = tip.y * kinect_z + kinect_y;

		for (auto iter=regions.begin(); iter!=regions.end(); ++iter)
		{
			ofPolyline trans = utility::transform(iter->second, video_x, video_y, video_w/video->getWidth());
			if( trans.inside(center) ) {
				palmText = iter->first;
			}
		}

		ofPushMatrix();
			ofTranslate(center.x, center.y);
			// Proper rotation
			float h = sqrt( pow(center.x - tip.x, 2) + pow(center.y - tip.y, 2) );
			float angle =  ofRadToDeg( asin( (tip.y - center.y) / h ));
			if(tip.x < center.x) angle *= -1;
			if (side == 1) angle += 180;
			if ( (side == 0 or side == 2 ) and tip.y < center.y ) 
				angle += 180;
			ofRotateZ(angle);
			ofRectangle textBox = font.getStringBoundingBox(palmText, 0, 0);
			ofPoint textCenter = textBox.getCenter();
			float tWidth = textBox.getWidth();
			float tHeight = textBox.getHeight();
			float tSize = max(tWidth, 2*tHeight);
			ofTranslate(-textCenter.x, -textCenter.y);
			float size = ofDist(tip.x, tip.y, center.x, center.y);
			ofScale(size/tSize, size/tSize);
			font.drawString(palmText, 0, 0);
		ofPopMatrix();
	}

}

void ofApp::drawFeedback() {

	ofPushStyle();
	kinectImg.draw(0,0);
	ofSetColor(0,255,0);
	ofTranslate(crop_left, crop_top);
		ContourFinder.draw();

	for (int i = 0; i < ContourFinder.hands.size(); ++i)
	{
		ofSetColor(255,0,255);
		ofFill();
		ofCircle(ContourFinder.hands[i].end, 3);
		ofCircle(ContourFinder.hands[i].tip, 3);
		ofCircle(ContourFinder.hands[i].wrists[0], 3);
		ofCircle(ContourFinder.hands[i].wrists[1], 3);
		ofCircle(ContourFinder.hands[i].centroid, 3);
		ofSetColor(255,255,255);
		ofNoFill();
		ofCircle(ContourFinder.hands[i].tip, ContourFinder.MAX_HAND_SIZE);
		ofCircle(ContourFinder.hands[i].tip, ContourFinder.MIN_HAND_SIZE);
		ofCircle(ContourFinder.hands[i].wrists[0], ContourFinder.MAX_WRIST_WIDTH);
		ofCircle(ContourFinder.hands[i].wrists[0], ContourFinder.MIN_WRIST_WIDTH);
	}
	ofTranslate(-crop_left, -crop_top);

	ofPushStyle();
	ofPushMatrix();
	ofTranslate(video_x, video_y);
	ofScale(video_w/video->getWidth(), video_h/video->getHeight());
	int i = 0;
	for(auto iter=regions.begin(); iter!=regions.end(); ++iter) {
		ofSetColor( (i&1) * 255, (i&2) * 128, (i&4) * 64); // A color varying tool I'm quite proud of
		iter->second.draw();
		i++;
	}
	ofSetColor(255,255,255);
	ofPopStyle();
	ofPopMatrix();

	stringstream reportStream;
	reportStream
	<< "frame: " << video->getCurrentFrame() << endl
	<< "currentPhase: " << currentPhase << endl
	<< "nextPhaseFrame: " << nextPhaseFrame << endl
	<< "speed: " << speed << endl
	<< "idle time: " << ofToString((ofGetSystemTimeMicros() - lastTime)/1000000) << endl
	<< "framerate: " << ofToString(ofGetFrameRate()) << endl;

	ofDrawBitmapString(reportStream.str(), 100, 600);
	ofPopStyle();

}

void ofApp::loadSettings() {

	XML.pushTag(ofToString(PLATFORM));
		video_x = XML.getValue("VIDEO:X", 0);
		video_y = XML.getValue("VIDEO:Y", 0);
		video_w = XML.getValue("VIDEO:W", firstVideo.getWidth());
		video_h = XML.getValue("VIDEO:H", firstVideo.getHeight());
		video_r = XML.getValue("VIDEO:R", 0);

		XML.pushTag("KINECT");
				XML.pushTag("CROP");
					crop_left = XML.getValue("LEFT", 0);
					crop_right = XML.getValue("RIGHT", 0);
					crop_top = XML.getValue("TOP", 0);
					crop_bottom = XML.getValue("BOTTOM", 0);
				XML.popTag();
			if(REGISTRATION) 
				XML.pushTag("REGISTRATION");
			else
				XML.pushTag("NOREGISTRATION");
					kinect_z = XML.getValue("Z", 2.77);
					kinect_x = XML.getValue("X", 0);
					kinect_y = XML.getValue("Y", 0);
				XML.popTag();
		XML.popTag();
	XML.popTag();

	// ContourFinder
	ContourFinder.setMinArea(XML.getValue("CV:MINAREA", 1000));
	ContourFinder.bounds[0] = 1;
	ContourFinder.bounds[1] = 38; // TODO WHY???
	ContourFinder.bounds[2] = kinect.width - crop_left - crop_right - 1;
	ContourFinder.bounds[3] = kinect.height - crop_top - crop_bottom - 1;

	farThreshold = XML.getValue("KINECT:FARTHRESHOLD", 2);
	nearThreshold = XML.getValue("KINECT:NEARTHRESHOLD", farThreshold + 40);
	screensaverTime = XML.getValue("SCREENSAVERTIME", 1800);

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

	switch(key) {

		case OF_KEY_LEFT:
			kinect_x--;
			break;

		case OF_KEY_RIGHT:
			kinect_x++;
			break;

		case OF_KEY_UP:
			kinect_y--;
			break;

		case OF_KEY_DOWN:
			kinect_y++;
			break;

		case 'f':
			bDisplayFeedback = !bDisplayFeedback;
			break;

		case ' ': {
			if(video->isPaused())
				video->setPaused(false);
			else
				video->setPaused(true);
			break;
		}

		case OF_KEY_RETURN:
			bLearnBackground = true;
			break;

		case 'S': {
			// Saving calibration
			XML.pushTag(ofToString(PLATFORM));
				XML.pushTag("KINECT");
				if(REGISTRATION)
					XML.pushTag("REGISTRATION");
				else
					XML.pushTag("NOREGISTRATION");

						XML.setValue("X", kinect_x);
						XML.setValue("Y", kinect_y);
						XML.setValue("Z", kinect_z);
					XML.popTag();
				XML.popTag();
			XML.popTag();
			XML.saveFile("settings.xml");
			cout << "Settings saved!";
			break;
		}
	}

}
//--------------------------------------------------------------
void ofApp::mousePressed(int mx, int my, int button){

	// auto iter = regions.begin();
	// int j = 0;
	// while(j < activeRegion) {
	// 	iter++;
	// 	j++;
	// }
	// string name = iter->first;
	// regions[name].addVertex(mx, my);
	return;

}
