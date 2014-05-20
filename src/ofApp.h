// argentueil_final
#pragma once

#ifdef __APPLE__
	#define PLATFORM "MAC"
#else
	#define PLATFORM "WINDOWS"
#endif

#include "ofMain.h"
// Computer vision
#include "ofxOpenCv.h"
#include "ofxCv.h"
#include "ofxKinect.h"
// Ripples effect
#include "ofxRipples.h"
#include "ofxBounce.h" 
// Special font characters
#include "ofxTrueTypeFontUC.h"
// Saving data
#include "ofxXmlSettings.h"
// My headers
#include "utility.h"
#include "ArmContourFinder.h"
#include "Critter.h"
// Standards
#include <cmath>

#define START_PHASE 0

#define REGISTRATION false
// Don't ask
#define UULONG_MAX 18446744073709551615

class ofApp : public ofBaseApp{

public:
	void setup();
	void loadSettings();

	void update();
	void adjustPhase();
	void updateRegions();
	void updateRipples();
	void updateBeavers();

	void draw();
	void drawHandMask(ofColor color = ofColor(0,0,0), bool bDrawArms = true, bool scale = false);
	void drawBeavers();
	void drawHandText();
	void drawHandImages();
	void drawFeedback();

	void startScreensaver();

	void keyPressed(int key);
	void mousePressed(int x, int y, int button);

	// Input processing
	ofxKinect 			kinect;	
	ofxCvGrayscaleImage	kinectImg;
	ofxCvGrayscaleImage kinectBackground;
	int 				nearThreshold;
	int 				farThreshold;
	bool 				bLearnBackground;
	// For cropping out un-desired regions
	cv::Mat 			input;
	cv::Mat 			croppedInput;

	// Computer vision
	ArmContourFinder 	ContourFinder;

	// Videos
	ofVideoPlayer 		firstVideo;
	ofVideoPlayer 		secondVideo;
	ofVideoPlayer *		video;
	float 				speed;

	// Video blending
	ofShader 			shader;
	ofFbo 				fbo;
	ofFbo 				maskFbo;
	ofImage 			brushImg;

	// Ripples effect
	ofxRipples			ripples;
	ofxBounce 			bounce;
	ofImage 			riverMask;
	map<int, ofImage> 	animatedMask;
	ofVideoPlayer 		maskVid;
	ofImage 			currentMask;
	int 				maskNumber;
	bool 				bRipple;

	// Beaver game
	vector< Critter > 	Beavers;
	vector< ofImage > 	gifFrames;
	int 				numBeaverFrames;

	// Labeling
	map<string, ofPolyline> 	regions;

	// For saving data
	ofxXmlSettings 		XML;
	// For determining what's going on
	int currentPhase;
	int nextPhaseFrame;
	// For transitions between phases
	bool bTransition;

	// Font display
	ofxTrueTypeFontUC 	font;
	bool 				bHandText;
	map< unsigned int, vector<ofPoint> > stableHands;
	float 				minVelocity;

	// Calibration settings
	float video_x, video_y, video_w, video_h, video_r;
	float kinect_x, kinect_y, kinect_z;
	int crop_left, crop_top, crop_right, crop_bottom;
	// Feedback
	bool bDisplayFeedback;

	// Just as a test
	ofImage handImage;

	// For screensaver
	unsigned long long  lastTime;
	unsigned long long  screensaverTime;
	unsigned long long  screensaverEndTime;
	bool 				bScreensaver;
	ofVideoPlayer 		screensaver;


	//============ Non Permanent Variables ======//
	// Calibration
	int x, y, w, h;
	float r, z;
	int minArea;

	// Adding regions
	int 			activeRegion;
	vector<string> regionNames;

};
