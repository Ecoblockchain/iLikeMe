#include "ofMain.h"
#include "iLikeMe.h"
#include "ofAppGlutWindow.h"

int main( ){
    ofAppGlutWindow window;
	ofSetupOpenGL(&window, 1280,720, OF_WINDOW);
	ofRunApp( new iLikeMe());
}
