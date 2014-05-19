#include "Critter.h"

Critter::Critter(int numFrames) {

	hidden = false;
	currentFrame = ofRandom(numFrames);
	v = ofRandom(MIN_VELOCITY, MAX_VELOCITY/4);
	nextFrame = currentFrame; // Probably should call this framerate
	d = ofRandom(0, 360);  // Angle BELOW the x-axis (because y ascends as you move down the page)
	p = ofPoint(ofRandom(0,1920), ofRandom(0,1080));

	offsets.push_back(ofMap(p.x + p.y, 0, 3000, -100, 100));
	offsets.push_back(ofMap(d, 0, 360, -100, 100));
    
	this-> numFrames = numFrames;

}

void Critter::update(ofVec2f nearestHand) {

	nextFrame += ofMap(v, 0, 10, 0, 2);
	currentFrame = floor(nextFrame);
	if(currentFrame >= numFrames) {
		currentFrame = 0;
		nextFrame = 0;
	}
	bool draw = true;
	for (int i = 0; i < previousFrames.size(); ++i)
	{
		if(previousFrames[i])
			draw = false;
	}
	if(!hidden and draw) {
		p.x += v * cos(d/180*PI);
		p.y += v * sin(d/180*PI); 
	}
    
	float time = ofGetElapsedTimef();
	v += 	ofSignedNoise(time * TIME_SCALE + offsets[0]) * VELOCITY_DISPLACEMENT_SCALE;
	d += 	ofSignedNoise(time * TIME_SCALE + offsets[1]) * DIRECTION_DISPLACEMENT_SCALE;

	v = fmax(MIN_VELOCITY, fmin(v, MAX_VELOCITY));
	d = fmax(0, fmin(d, 360));

	if(nearestHand.length() != 0) {
		ofVec2f vel = ofVec2f(v * cos(d/180*PI), v * sin(d/180*PI));
		float fear = ofMap(nearestHand.length(), 500, 0, 0, 1, true);
        float angle = vel.angle(nearestHand);
		if(angle < 0)
			angle += 360;
		d += ofMap((angle - 180) * fear, -180, 180, -20, 20);
		v += ofMap(fear, 0, 1, 0, 0.5);
	}
}