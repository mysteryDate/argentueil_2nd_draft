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
#include <cmath>

class ofApp : public ofBaseApp{

public:
	void setup();
	void loadSettings();

	void update();
	void adjustPhase();

	void draw();
	void drawFeedback();
	void drawHandMask(ofColor color = ofColor(0,0,0), bool bDrawArms = true);
	void drawHandText();

	void keyPressed(int key);
	void mousePressed(int x, int y, int button);

	// Input processing
	ofxKinect 			kinect;	
	ofxCvGrayscaleImage	depthImage;
	ofxCvGrayscaleImage depthBackground;
	int 				nearThreshold;
	int 				farThreshold;
	bool 				bLearnBackground;
	// When using color as well
	ofxCvColorImage 	colorImage;
	ofxCvColorImage 	colorBackground;
	int 				dThreshold;
	int 				cThreshold;
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

	// For saving data
	ofxXmlSettings 		XML;
	// For determinind what's going on
	int currentPhase;
	int nextPhaseFrame;

	// Font display
	ofxTrueTypeFontUC 	font;

	// Calibration settings
	float video_x, video_y, video_w, video_h, video_r;
	float kinect_x, kinect_y, kinect_z;
	// Feedback
	bool bDisplayFeedback;

	//============ Non Permanent Variables ======//
	// Calibration
	int x, y, w, h;
	float r, z;
	int minArea;

};
