#include "iLikeMe.h"

using namespace ofxCv;

void iLikeMe::setup(){
	ofSetVerticalSync(true);
	ofEnableAlphaBlending();

	mCamera.setup();
	// HACK !
	imageDimensions = ofVec2f(1056/2, 704/2);

	cImg.allocate(imageDimensions.x, imageDimensions.y);
	grayDiff.allocate(imageDimensions.x, imageDimensions.y);
	for(int i=0; i<2; ++i){
		previousFrames[i].allocate(imageDimensions.x, imageDimensions.y);
	}
	previousFramesIndex = 0;

	hairLayer.allocate(imageDimensions.x, imageDimensions.y, OF_IMAGE_COLOR_ALPHA);
	printLayer.allocate(imageDimensions.x, imageDimensions.y, OF_IMAGE_COLOR_ALPHA);
	thresholdValue = 200;

	lastFaceTime = ofGetElapsedTimeMillis();
    lastSavedTime = ofGetElapsedTimeMillis();

	currentColorScheme = ColorScheme::getScheme(0);

	for(int i=0; i<12; ++i){
		faceFbos[i].allocate(imageDimensions.x, imageDimensions.y);
	}
	mFaceFeatures.cropArea.set(0,0, imageDimensions.x, imageDimensions.y);
	scaleFactor = 1;

	tracker.setup();
	tracker.setRescale(.5);
}

void iLikeMe::update(){
	mCamera.update();

    if(mCamera.isFrameNew()) {
		// face detection
		ofPixels mPix = mCamera.getLivePixels();
		// resize to ~(640, 480)
		mPix.resize(imageDimensions.x, imageDimensions.y);
		tracker.update(toCv(mPix));

        // get a copy for the print layer
		thresholdCam(mPix,printLayer);

		// keep previous camera color image as a grayscale cv image
		previousFrames[previousFramesIndex] = cImg;
		previousFrames[previousFramesIndex].blur(23);
		previousFramesIndex = (previousFramesIndex+1)%2;

		// get new camera image
		cImg.setFromPixels(mPix);
		grayDiff = cImg;
		//grayDiff.blur();

		// get diff between frames with blur to get person outline
		previousFrames[previousFramesIndex].absDiff(grayDiff);
		previousFrames[previousFramesIndex].blur(27);
		previousFrames[previousFramesIndex].threshold(4,false);
		// makes a layer where outline is white and everything else is transparent
		makeBlackTransparent(previousFrames[previousFramesIndex],hairLayer);
	}
	
	////
	if(tracker.getFound()) {
		// if this is a new face, get a new color
		if(ofGetElapsedTimeMillis()-lastFaceTime > 1000){
			currentColorScheme = ColorScheme::getScheme();
		}

		// get poly lines
		mFaceFeatures.face = tracker.getImageFeature(ofxFaceTracker::FACE_OUTLINE);
		mFaceFeatures.outMouth = tracker.getImageFeature(ofxFaceTracker::OUTER_MOUTH);
		mFaceFeatures.inMouth = tracker.getImageFeature(ofxFaceTracker::INNER_MOUTH);
		mFaceFeatures.rightEyebrow = tracker.getImageFeature(ofxFaceTracker::RIGHT_EYEBROW);
		mFaceFeatures.leftEyebrow = tracker.getImageFeature(ofxFaceTracker::LEFT_EYEBROW);
		mFaceFeatures.rightEye = tracker.getImageFeature(ofxFaceTracker::RIGHT_EYE);
		mFaceFeatures.leftEye = tracker.getImageFeature(ofxFaceTracker::LEFT_EYE);

		// for forehead
		mFaceFeatures.faceTrans = tracker.getImageFeature(ofxFaceTracker::NOSE_BASE).getCentroid2D()-mFaceFeatures.inMouth.getCentroid2D();
		mFaceFeatures.faceTrans.normalize();

		// for eye lids
		FaceFeatures::blowUpPolyline(mFaceFeatures.rightEye);
		FaceFeatures::blowUpPolyline(mFaceFeatures.leftEye);
		mFaceFeatures.rightEyeTrans = mFaceFeatures.rightEye.getBoundingBox().getCenter()-mFaceFeatures.rightEyebrow.getBoundingBox().getCenter();
		mFaceFeatures.leftEyeTrans = mFaceFeatures.leftEye.getBoundingBox().getCenter()-mFaceFeatures.leftEyebrow.getBoundingBox().getCenter();

		// bounding box
		float x0 = mFaceFeatures.face.getBoundingBox().x-mFaceFeatures.face.getBoundingBox().width/1.666;
		float y0 = (mFaceFeatures.face.getBoundingBox().y+mFaceFeatures.face.getBoundingBox().height*1.1)-mFaceFeatures.face.getBoundingBox().width*2.2;
		if(y0 < 0) y0 = 0;
		mFaceFeatures.cropArea.set(x0,y0,mFaceFeatures.face.getBoundingBox().width*2.2,mFaceFeatures.face.getBoundingBox().width*2.2);
		
		// update timer
		lastFaceTime = ofGetElapsedTimeMillis();
	}
}

void iLikeMe::drawFace(){
	if(tracker.getFound()) {
		ofPushMatrix();
		ofTranslate(-mFaceFeatures.cropArea.x,-mFaceFeatures.cropArea.y);

		// draw shapes and color
		// BACKGROUND
		ofSetColor(currentColorScheme.background);
		ofDrawRectangle(0,0, imageDimensions.x, imageDimensions.y);
		
		// HAIR
		ofSetColor(currentColorScheme.hair);
		hairLayer.draw(0,0);
		
		// FACE
		ofSetColor(currentColorScheme.face);
		ofBeginShape();
		ofVertices(mFaceFeatures.face.getVertices());
		ofEndShape();
		
		// FOREHEAD
		ofSetColor(currentColorScheme.face);
		ofPushMatrix();
		ofTranslate(0.29*mFaceFeatures.faceTrans*mFaceFeatures.face.getBoundingBox().height);
		ofBeginShape();
		ofVertices(mFaceFeatures.face.getVertices());
		ofEndShape();
		ofPopMatrix();
		
		// MOUTH
		ofSetColor(currentColorScheme.mouth);
		ofBeginShape();
		ofVertices(mFaceFeatures.outMouth.getVertices());
		ofEndShape();
		ofSetColor(currentColorScheme.teeth);
		ofBeginShape();
		ofVertices(mFaceFeatures.inMouth.getVertices());
		ofEndShape();
		
		// RIGHT EYE LID
		ofPushMatrix();
		ofSetColor(currentColorScheme.eyelid);
		ofTranslate(-mFaceFeatures.rightEyeTrans*0.3);
		ofBeginShape();
		ofVertices(mFaceFeatures.rightEye.getVertices());
		ofEndShape();
		ofSetColor(currentColorScheme.face);
		ofTranslate(mFaceFeatures.rightEyeTrans*0.3);
		ofBeginShape();
		ofVertices(mFaceFeatures.rightEye.getVertices());
		ofEndShape();
		ofPopMatrix();
		
		// LEFT EYE LID
		ofPushMatrix();
		ofSetColor(currentColorScheme.eyelid);
		ofTranslate(-mFaceFeatures.leftEyeTrans*0.3);
		ofBeginShape();
		ofVertices(mFaceFeatures.leftEye.getVertices());
		ofEndShape();
		ofSetColor(currentColorScheme.face);
		ofTranslate(mFaceFeatures.leftEyeTrans*0.3);
		ofBeginShape();
		ofVertices(mFaceFeatures.leftEye.getVertices());
		ofEndShape();
		ofPopMatrix();
		
		// RIGHT EYE
		ofSetColor(currentColorScheme.eye);
        ofDrawCircle(mFaceFeatures.rightEye.getCentroid2D(), 0.5*min(mFaceFeatures.rightEye.getBoundingBox().getWidth(),
                                                                     mFaceFeatures.rightEye.getBoundingBox().getHeight()));
		
		// LEFT EYE
		ofSetColor(currentColorScheme.eye);
		ofDrawCircle(mFaceFeatures.leftEye.getCentroid2D().x*0.96,
                     mFaceFeatures.leftEye.getCentroid2D().y, 0.5*min(mFaceFeatures.leftEye.getBoundingBox().getWidth(),
                                                                      mFaceFeatures.leftEye.getBoundingBox().getHeight()));
		
		// PRINT
		ofSetColor(currentColorScheme.print);
		printLayer.draw(0,0);
		ofPopMatrix();
	}
	else{
		ofClear(0);
	}
}

void iLikeMe::draw(){
	ofBackground(0);
	ofSetColor(255);

	ofSetColor(255,255,0);
	ofDrawBitmapString(ofToString((int) ofGetFrameRate()), 10, imageDimensions.y+20);
	if(!(ofGetFrameNum()%60)){
		cout << ofGetFrameRate() << endl;
	}

	// create 12
	for(int i=0; i<12; ++i){
		currentColorScheme = ColorScheme::getScheme(i);
		faceFbos[i].begin();
		drawFace();
		faceFbos[i].end();
	}

    // save one of the fbos
    imgsDir.open(ofToDataPath("imgs"));
    imgsDir.allowExt("png");
    imgsDir.listDir();
    if((ofGetElapsedTimeMillis() - lastSavedTime > 1000) && (imgsDir.size() < 12) && tracker.getFound()){
        saveImage((int)(ofClamp(abs(ofRandom(-2.0, 10.0)), 1.0, 10.0)));
        lastSavedTime = ofGetElapsedTimeMillis();
    }
    imgsDir.close();

	int i=0;
	int cx=0, cy=0;
	float s = 1.0/scaleFactor;

	// can draw more than 12
	while(cy<ofGetHeight()){
		ofPushMatrix();
		ofTranslate(cx,cy);
		ofScale(s,s);
		ofSetColor(255);
		faceFbos[i].getTexture().drawSubsection(0, 0, mFaceFeatures.cropArea.width, mFaceFeatures.cropArea.height, 1,1);
		i = (i+1)%12;
		cx = (cx+mFaceFeatures.cropArea.width*s);
		if(cx > ofGetWidth()){
			cx = 0;
			cy += mFaceFeatures.cropArea.height*s;
		}
		ofPopMatrix();
	}
}

void FaceFeatures::blowUpPolyline(ofPolyline &pl){
	ofPoint center = pl.getBoundingBox().getCenter();
	for(int i=0; i<pl.size(); ++i){
		pl[i] += pl[i]-center;
	}
}

// destructive: changes incoming img
void iLikeMe::thresholdCam(const ofPixels &ip, ofImage &out){
    ofPixels op = out.getPixels();
	for(int i=0; i<ip.getHeight()*ip.getWidth(); ++i){
		float gray = 0.21*float(ip[i*3+0]) + 0.71*float(ip[i*3+1]) + 0.07*float(ip[i*3+2]);
		op[i*4+0] = op[i*4+1] = op[i*4+2] = (gray < thresholdValue)?255:0;
		op[i*4+3] = (gray < thresholdValue)?255:0;
	}
    out.setFromPixels(op);
}

// makes black pixels transparent
void iLikeMe::makeBlackTransparent(ofxCvGrayscaleImage &in, ofImage &out){
    ofPixels op = out.getPixels();
    ofPixels ip = in.getPixels();
	for(int i=0; i<in.height*in.width; ++i){
		op[i*4+0] = op[i*4+1] = op[i*4+2] = (ip[i] < 10)?0:255;
		op[i*4+3] = (ip[i] < 10)?0:255;
	}
    out.setFromPixels(op);
}

void iLikeMe::saveImage(int sqrtOfNumberOfFaces){
    ofFbo saveFbo;
    saveFbo.allocate(sqrtOfNumberOfFaces*mFaceFeatures.cropArea.width,
                     sqrtOfNumberOfFaces*mFaceFeatures.cropArea.height);
    saveFbo.begin();
    ofSetColor(255);
    int fboIndex = ((int)ofRandom(120))%12;
    for(int i=0; i<sqrtOfNumberOfFaces; i++){
        for(int j=0; j<sqrtOfNumberOfFaces; j++){
            faceFbos[fboIndex].getTexture().drawSubsection(i*mFaceFeatures.cropArea.width, j*mFaceFeatures.cropArea.height,
                                                                    mFaceFeatures.cropArea.width, mFaceFeatures.cropArea.height,
                                                                    1,1);
            fboIndex = (fboIndex+1)%12;
        }
    }
    saveFbo.end();
    ofPixels savePixels;
    saveFbo.getTexture().readToPixels(savePixels);
    if(savePixels.getWidth() > MAX_SAVED_PICTURE_DIMENSION || savePixels.getHeight() > MAX_SAVED_PICTURE_DIMENSION){
        savePixels.resize(MAX_SAVED_PICTURE_DIMENSION, MAX_SAVED_PICTURE_DIMENSION);
    }
    ofSaveImage(savePixels, ofToDataPath("imgs/img."+ofToString(ofGetFrameNum())+".png"));
}

void iLikeMe::keyReleased(int key){
	tracker.reset();
	if(key == '-' || key == '_'){
		thresholdValue = max(thresholdValue-1, 0);
	}
	if(key == '+' || key == '='){
		thresholdValue = min(thresholdValue+1, 255);
	}
	if(key == 356){
		scaleFactor = max(scaleFactor-1, 1);
	}
	if(key == 358){
		scaleFactor = min(scaleFactor+1, 10);
	}
}

void iLikeMe::keyPressed(int key){}

void iLikeMe::exit(){
    cout << "exit\n";
    mCamera.close();
    ofBaseApp::exit();
}
