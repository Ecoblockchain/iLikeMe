#pragma once

#include "ofMain.h"
#include "ofxCv.h"
#include "ofxOpenCv.h"
#include "ofxFaceTracker.h"
#include "ColorScheme.h"

#define MAX_SAVED_PICTURE_DIMENSION 900

class FaceFeatures{
public:
	ofPolyline face, outMouth, inMouth;
	ofPolyline rightEyebrow, leftEyebrow;
	ofPolyline rightEye, leftEye;
	ofVec2f rightEyeTrans, leftEyeTrans;
	ofVec2f faceTrans;
	ofRectangle cropArea;
	static void blowUpPolyline(ofPolyline &pl);
};

class iLikeMe : public ofBaseApp{
	
public:
	void setup();
	void update();
	void draw();
	void exit();

	void thresholdCam(ofVideoGrabber &in, ofImage &out);
	void makeBlackTransparent(ofxCvGrayscaleImage &in, ofImage &out);
    void saveImage(int sqrtOfNumberOfFaces);

	void keyPressed  (int key);
	void keyReleased(int key);
	
	ofVideoGrabber cam;

	ofxCvColorImage cImg;
	ofxCvGrayscaleImage grayDiff;
	ofxCvGrayscaleImage previousFrames[2];
	unsigned int previousFramesIndex;

	ofImage printLayer, hairLayer;
	unsigned char thresholdValue;

	long long int lastFaceTime, lastSavedTime;
    ofDirectory imgsDir;

	ColorScheme currentColorScheme;

	ofFbo faceFbos[12];
	FaceFeatures mFaceFeatures;
	unsigned int scaleFactor;

	ofxFaceTracker tracker;

private:
	void drawFace();
};
