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

	vectors[0] = ofVec2f(v * cos(d/180*PI) * 10, v * sin(d/180*PI) * 10);

}

void Critter::update(vector< ofPolyline > hands) {

	ofVec2f nearestHand; // The vector to the closest hand

	nextFrame += ofMap(v, 0, 10, 0, 2);
	currentFrame = floor(nextFrame);
	if(currentFrame >= numFrames) {
		currentFrame = 0;
		nextFrame = 0;
	}
	if(!hidden) {
		p.x += v * cos(d/180*PI);
		p.y += v * sin(d/180*PI); // Because y descends upwards
	}
    
	float time = ofGetElapsedTimef();
	v += 	ofSignedNoise(time * TIME_SCALE + offsets[0]) * VELOCITY_DISPLACEMENT_SCALE;
	d += 	ofSignedNoise(time * TIME_SCALE + offsets[1]) * DIRECTION_DISPLACEMENT_SCALE;

	v = fmax(MIN_VELOCITY, fmin(v, MAX_VELOCITY));
	d = fmax(0, fmin(d, 360));

	vectors[0] = ofVec2f(v * cos(d/180*PI) * 10, v * sin(d/180*PI) * 10);

	if(hands.size() != 0) {
		nearestHand = findClosestHand(hands); 
		vectors[2] = nearestHand;
		float fear = ofMap(nearestHand.length(), 500, 0, 0, 1, true);
        float angle = vectors[0].angle(nearestHand);
		if(angle < 0)
			angle += 360;
		d += ofMap((angle - 180) * fear, -180, 180, -20, 20);
		v += ofMap(fear, 0, 1, 0, 0.5);
	}

	vectors[1] = ofVec2f(v * cos(d/180*PI) * 10, v * sin(d/180*PI) * 10);
}

// Returns a vector from critter to closest hand
ofVec2f Critter::findClosestHand(vector < ofPolyline > hands) {

	ofVec2f nearestHand;
	float minDist = 999999; // I hate doing this

	for (int i = 0; i < hands.size(); ++i)
	{
		ofPoint center = hands[i].getCentroid2D();
		float dist = ofDistSquared(p.x, p.y, center.x, center.y);
		if(dist < minDist) {
			minDist = dist;
			nearestHand = ofVec2f(center.x - p.x, center.y - p.y);
		}
	}

	return nearestHand;
}