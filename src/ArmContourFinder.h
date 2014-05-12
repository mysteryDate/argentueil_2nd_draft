#include "ContourFinder.h"
#include "ofMain.h"

class ArmContourFinder : public ofxCv::ContourFinder
{
public:

	ArmContourFinder();

	void setHandSize(int max, int min);
	void setWristSize(int max, int min);
	void setBounds(int left, int top, int right, int bottom);

	// The keys are the labels
	map< unsigned int, ofPoint > ends;
	map< unsigned int, ofPoint > tips;
	map< unsigned int, vector< ofPoint > > wrists;
	map< unsigned int, bool > handFound;
	map< unsigned int, int > side;

	void update();

	struct Hand
	{
		Hand():
			centroid( ofPoint(-1, -1) ),
			tip( ofPoint(-1, -1) ),
			end( ofPoint(-1, -1) ),
			boxCenter( ofPoint(-1, -1) ) {}
		ofPolyline line;
		ofPoint centroid;
		ofPoint tip;
		ofPoint end;
		ofPoint boxCenter;
		vector< ofPoint > wrists;
		int side;
		unsigned int label;
		ofVec2f velocity;

		//For sorting by label
		bool operator < (const Hand& str) const
		{
			return (label < str.label);
		}
	};
	map< unsigned int, Hand > hands;
	
	int MIN_HAND_SIZE;
	int MAX_HAND_SIZE;
	int MAX_WRIST_WIDTH;
	int MIN_WRIST_WIDTH;

	vector< int > bounds;

private:

	bool findHand(int n);
	ofPoint			 	findEnd(int n, Hand newHand);
	ofPoint				findTip(int n, Hand newHand);
	vector< ofPoint > 	findWrists(int n, Hand newHand);
	ofPoint 			refitTip(int n, Hand newHand);

	void addHand(int n);

	
};