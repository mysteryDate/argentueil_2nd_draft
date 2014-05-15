// Argenteuil final
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
	// firstVideo.play();
	video = &firstVideo;
	// video->setFrame(1200);
	currentPhase = 1;
	// nextPhaseFrame = 1290;
	nextPhaseFrame = video->getCurrentFrame() + 1;
	speed = 1;

	//kinect instructions
	kinect.init();
	kinect.setRegistration(REGISTRATION);
	kinect.open();
	depthImage.allocate(kinect.width, kinect.height);
	depthBackground.allocate(kinect.width, kinect.height);

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
	riverMask.loadImage("masks/river_mask_v3.png");
	bRipple = false;

	// Setting up shaders for phase 5
	shader.setupShaderFromFile(GL_FRAGMENT_SHADER, "shadersGL2/shader.frag");
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
	// brushImg.loadImage("brush.png");

	// Creating church regions
	// activeRegion = 0;
	// ofBuffer buffer = ofBufferFromFile("river_names.txt");
	// while(!buffer.isLastLine() ) {
	// 	string name = buffer.getNextLine().c_str();
	// 	regionNames.push_back( name );
	// 	regions.insert(pair<string, ofPolyline>(name, ofPolyline()));
	// }

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

		input = ofxCv::toCv(depthImage);
		cv::Rect crop_roi = cv::Rect(crop_left, crop_top, 
			kinect.width - crop_left - crop_right,
			kinect.height - crop_top - crop_bottom);
		croppedInput = input(crop_roi).clone();

		ContourFinder.findContours(croppedInput);
		if(bHandText)
			ContourFinder.update();
		else
			ContourFinder.hands.clear();
	}
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
			float alpha = ofMap((nextPhaseFrame - video->getCurrentFrame()), 100, 0, 0, 7);
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

//--------------------------------------------------------------
void ofApp::draw(){

	ofPushMatrix();
		ofRotateZ(video_r);
		if(bRipple)
			bounce.draw(video_x, video_y, video_w, video_h);
		else { 
			video->draw(video_x, video_y, video_w, video_h);
			if(currentPhase == 5)
				fbo.draw(video_x, video_y, video_w, video_h);
		}
	ofPopMatrix();

	if(bHandText)
		drawHandMask(ofColor(0,0,0));
		drawHandText();

	drawBeavers();

	if(bDisplayFeedback)
		drawFeedback();

}

//--------------------------------------------------------------
// Custom functions
//--------------------------------------------------------------
void ofApp::adjustPhase() {

	video->update();
	XML.pushTag("PHASEINFORMATION");

	int frame = video->getCurrentFrame();
	if( frame >= nextPhaseFrame) { // Change phase
		currentPhase++;
		if(currentPhase >= 9)
			currentPhase = 0;
		nextPhaseFrame = XML.getValue("PHASE:STARTFRAME", nextPhaseFrame + 1000, currentPhase + 1);
		if(currentPhase == 5) {
			secondVideo.setFrame(1900);
			secondVideo.update();
		}
		if(currentPhase == 6) {
			secondVideo.play();
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
		if(XML.getValue("PHASE:RIPPLE", 0, currentPhase))
			bRipple = true;
		else
			bRipple = false;
		farThreshold = XML.getValue("PHASE:FARTHRESHOLD", 2, currentPhase);
		nearThreshold = XML.getValue("PHASE:NEARTHRESHOLD", 42, currentPhase);
		if(XML.getValue("PHASE:HANDTEXT", 0, currentPhase) == 1)
			bHandText = true;
		else
			bHandText = false;
		updateRegions();
	}
	XML.popTag();

	// // Phase 5 is the magical phase
	// if(currentPhase == 5) {
	// 	// ofImage img;
	// 	// img.setFromPixels(secondVideo.getPixels(), secondVideo.getWidth(), secondVideo.getHeight(), OF_IMAGE_COLOR);
	// 	// img.saveImage("second_video_still.png");
	// 	// secondVideo.update();
	// }

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

		ofSetColor(0,0,0);
		ofFill();
		if(currentPhase != 0)
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
			if( iter->second.inside(center) ) {
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
			// ofScale(size/tSize, size/tSize);
			font.drawString(palmText, 0, 0);
		ofPopMatrix();
	}

}

void ofApp::drawFeedback() {

	ofPushStyle();
	depthImage.draw(0,0);
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
	int i = 0;
	for(auto iter=regions.begin(); iter!=regions.end(); ++iter) {
		ofSetColor( (i&1) * 255, (i&2) * 128, (i&4) * 64); // A color varying tool I'm quite proud of
		iter->second.draw();
		i++;
	}
	ofSetColor(255,255,255);
	// regions[regionNames[activeRegion]].draw();
	ofPopStyle();

	stringstream reportStream;
	reportStream
	// << "nearThreshold: " << nearThreshold << endl
	// << "farThreshold: " << farThreshold << endl
	<< "frame: " << video->getCurrentFrame() << endl
	<< "currentPhase: " << currentPhase << endl
	<< "nextPhaseFrame: " << nextPhaseFrame << endl
	// << "playing: " << ofToString(video->isPlaying()) << endl
	// << "Paused: " << ofToString(video->isPaused()) << endl
	// << "speed: " << speed << endl
	// << "minVelocity: " << minVelocity << endl
	<< "framerate: " << ofToString(ofGetFrameRate()) << endl;
	if(ContourFinder.hands.size() == 1)
		reportStream << "velocity: " << ofToString(ContourFinder.hands[0].velocity) << endl;
	// font.drawString(regionNames[activeRegion], 20, 800);
	// if  ( ContourFinder.size() == 1 ) {
	// 	ofRectangle rect = ofxCv::toOf(ContourFinder.getBoundingRect(0));
	// 	ofPoint min = rect.getMin();
	// 	ofPoint max = rect.getMax();
	// 	reportStream 
	// 	<< "minx: " << min.x << endl
	// 	<< "miny: " << min.y << endl
	// 	<< "maxx: " << max.x << endl
	// 	<< "maxy: " << max.y << endl;
	// }

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
					kinect_x = XML.getValue("X", 0) + crop_left * kinect_z;
					kinect_y = XML.getValue("Y", 0) + crop_top * kinect_z;
				XML.popTag();
		XML.popTag();
	XML.popTag();

	// ContourFinder
	ContourFinder.setMinArea(XML.getValue("CV:MINAREA", 1000));
	ContourFinder.bounds[0] = 1;
	ContourFinder.bounds[1] = 1;
	ContourFinder.bounds[2] = kinect.width - crop_left - crop_right - 1;
	ContourFinder.bounds[3] = kinect.height - crop_top - crop_bottom - 1;

	farThreshold = XML.getValue("KINECT:FARTHRESHOLD", 2);
	nearThreshold = XML.getValue("KINECT:NEARTHRESHOLD", farThreshold + 40);

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

	switch(key) {

		// case OF_KEY_DOWN: 
		// 	speed *= 0.9;
		// 	firstVideo.setSpeed(speed);
		// 	secondVideo.setSpeed(speed);
		// 	break;

		// case OF_KEY_UP: 
		// 	speed *= 1.1;
		// 	firstVideo.setSpeed(speed);
		// 	secondVideo.setSpeed(speed);
		// 	break;

		case OF_KEY_LEFT: 
			if(activeRegion == 0)
				activeRegion = regions.size() - 1;
			else
				activeRegion--;
			break;

		case OF_KEY_RIGHT: 
			if(activeRegion == regions.size() - 1)
				activeRegion = 0;
			else
				activeRegion++;
			break;

		// case OF_KEY_LEFT: 
		// 	kinect_x--;
		// 	break;

		// case OF_KEY_RIGHT: 
		// 	kinect_x++;
		// 	break;

		// case OF_KEY_UP: 
		// 	kinect_y--;
		// 	break;

		// case OF_KEY_DOWN: 
		// 	kinect_y++;
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

		// case OF_KEY_LEFT:
		// 	if(video->isPaused()) 
		// 		video->setFrame(video->getCurrentFrame() - 1);

		// case OF_KEY_RIGHT:
		// 	if(video->isPaused()) 
		// 		video->setFrame(video->getCurrentFrame() + 1);

		case 'Z': 
			kinect_z += 0.01;
			break;

		case 'z': 
			kinect_z -= 0.01;
			break;

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

		case ' ': {
			if(video->isPaused())
				video->setPaused(false);
			else
				video->setPaused(true);
			break;
		}

		case 'V':
			minVelocity += 0.1;
			break;

		case 'v':
			minVelocity -= 0.1;
			break;

		case OF_KEY_RETURN:
			bLearnBackground = true;
			break;

		case 'S': {
			// For saving regions
			// XML.pushTag("PHASEINFORMATION");
			// XML.pushTag("PHASE", currentPhase); // push phase, have to push in one at a time (annoying)
			// if (XML.getNumTags("REGIONS") == 0)
			// 	XML.addTag("REGIONS");
			// XML.pushTag("REGIONS");
			// XML.clear();
			// for(auto iterator=regions.begin(); iterator!=regions.end(); ++iterator) {
			// 	int rNum = XML.addTag("REGION");
			// 	XML.setValue("REGION:NAME", iterator->first, rNum);
			// 	XML.pushTag("REGION", rNum);
			// 	for (int i = 0; i < iterator->second.size(); ++i)
			// 	{
			// 		int vNum = XML.addTag("PT");
			// 		XML.setValue("PT:X", iterator->second[i].x, vNum);
			// 		XML.setValue("PT:Y", iterator->second[i].y, vNum);
			// 	}
			// 	XML.popTag();
			// }
			// Saving calibration
			// XML.pushTag(ofToString(PLATFORM));
			// 	XML.pushTag("KINECT");
			// 	if(REGISTRATION)
			// 		XML.pushTag("REGISTRATION");
			// 	else
			// 		XML.pushTag("NOREGISTRATION");

			// 			XML.setValue("X", kinect_x);
			// 			XML.setValue("Y", kinect_y);
			// 			XML.setValue("Z", kinect_z);
			// 		XML.popTag();
			// 	XML.popTag();
			// XML.popTag();
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
