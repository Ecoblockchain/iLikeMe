#pragma once

#include "ofMain.h"
#include "ofxCv.h"
#include "ofxOpenCv.h"
#include "ofxFaceTracker.h"
#include "ofxEdsdk.h"
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
	
    void thresholdCam(const ofPixels &ip, ofImage &out);
	void makeBlackTransparent(ofxCvGrayscaleImage &in, ofImage &out);
    void saveImage(int sqrtOfNumberOfFaces);

	void keyPressed  (int key);
	void keyReleased(int key);
	
	ofxEdsdk::Camera mCamera;
    ofVec2f imageDimensions;

	ofxCvColorImage cImg;
	ofxCvGrayscaleImage grayDiff;
	ofxCvGrayscaleImage previousFrames[2];
	unsigned int previousFramesIndex;

	ofImage printLayer, hairLayer;
	int thresholdValue;

	long long int lastFaceTime, lastSavedTime;
    ofDirectory imgsDir;

	ColorScheme currentColorScheme;

	ofFbo faceFbos[12];
	FaceFeatures mFaceFeatures;
	int scaleFactor;

	ofxFaceTracker tracker;

private:
	void drawFace();
};
