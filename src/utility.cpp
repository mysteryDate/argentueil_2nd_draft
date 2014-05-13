#include "utility.h"

vector< ofPolyline > utility::transform(vector< ofPolyline > input, int dx, int dy, float z, int r)
{
	vector< ofPolyline > output;		
	for (int i = 0; i < input.size(); ++i)
	{
		ofPolyline line = input[i];
		ofPolyline tLine;
		for (int j = 0; j < line.size(); ++j)
		{
			ofPoint pt = line[j];
			float tx = pt.x * z + dx;
			float ty = pt.y * z + dy;
			tLine.addVertex(tx, ty);
		}
		output.push_back(tLine);
	}
	return output;
}

vector< ofPolyline > utility::transform(ofxCv::ContourFinder input, int dx, int dy, float z, int r)
{
	vector< ofPolyline > output;		
	for (int i = 0; i < input.size(); ++i)
	{
		ofPolyline line = input.getPolyline(i);
		ofPolyline tLine;
		for (int j = 0; j < line.size(); ++j)
		{
			ofPoint pt = line[j];
			float tx = pt.x * z + dx;
			float ty = pt.y * z + dy;
			tLine.addVertex(tx, ty);
		}
		output.push_back(tLine);
	}
	return output;
}

vector< ofPoint > utility::transform(vector< ofPoint > input, int dx, int dy, float z, int r)
{
	vector< ofPoint > output;		
	for (int i = 0; i < input.size(); ++i)
	{
		ofPoint pt = input[i];
		float tx = pt.x * z + dx;
		float ty = pt.y * z + dy;
		output.push_back(ofPoint(tx, ty));
	}
	return output;
}

ofPolyline utility::transform(ofPolyline input, int dx, int dy, float z, int r)
{
	ofPolyline output;		
	for (int i = 0; i < input.size(); ++i)
	{
		ofPoint pt = input[i];
		float tx = pt.x * z + dx;
		float ty = pt.y * z + dy;
		output.addVertex(ofPoint(tx, ty));
	}
	return output;
}

ofPoint utility::transform(ofPoint input, int dx, int dy, float z, int r)
{
	ofPoint output;		
	float tx = input.x * z + dx;
	float ty = input.y * z + dy;
	output = ofPoint(tx, ty);
	return output;
}

vector< ofPoint > utility::findClosestPoints(ofPolyline one, ofPolyline two)
{
	float shortestDist = INFINITY;
	vector< ofPoint > closest;
	closest.resize(2);
	for (int i = 0; i < one.size(); ++i)
	{
		for (int j = i; j < two.size(); ++j)
		{
			float dsquared = ofDistSquared(one[i].x, one[i].y, two[j].x, two[j].y);
			if(dsquared <= shortestDist) {
				closest[0] = one[i];
				closest[1] = two[j];
			}
		}
	}

	return closest;
}














