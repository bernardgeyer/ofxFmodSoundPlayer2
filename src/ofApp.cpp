#include "ofApp.h"

void ofApp::initSounds(void)
{
	numSounds = 0;
	selectedSound = 0;
	selectedLevel = 1;
}

void ofApp::loadSounds(void)
{
	if(numSounds > 0) return;
	
	cout << "load sounds ..." << endl;
	
	sounds[numSounds].load("soundfiles/cloche_vache.mp3", 0); sounds[numSounds].setVolume(0.75f); ++numSounds;
	sounds[numSounds].load("soundfiles/grive-musicienne.mp3", 0); sounds[numSounds].setVolume(0.75f); ++numSounds;
	sounds[numSounds].load("soundfiles/Musique-De-Cirque1.mp3", 0); sounds[numSounds].setVolume(0.75f); ++numSounds;
	sounds[numSounds].load("soundfiles/plage.mp3", 0); sounds[numSounds].setVolume(0.75f); ++numSounds;
	sounds[numSounds].load("soundfiles/vent-bord-mer.mp3", 0); sounds[numSounds].setVolume(0.75f); ++numSounds;
	sounds[numSounds].load("soundfiles/Carnaval-Chars.wav", 0); sounds[numSounds].setVolume(0.75f); ++numSounds;

	cout << numSounds << " sounds loaded" << endl;
}

void ofApp::unloadSounds(void)
{
	if(numSounds == 0) return;

	cout << "unload " << numSounds << " sounds ..." << endl;
	
	for(int i = 0; i < numSounds; ++i) {
		sounds[i].unload();
	} numSounds = 0;

	cout << "sounds unloaded" << endl;
}

//--------------------------------------------------------------

void ofApp::setup(){
	ofBackgroundHex(0X555555);
	
	ofxFmodSetPreferedDevice(0); // <- put here the number of your device if you know it
	// Set the prefered device .
	// It will be set if possible, and if not 0 is set instead.
	
	initSounds();
	loadSounds();
}

//--------------------------------------------------------------
void ofApp::update(){
	ofxFmodSoundUpdate();
}

//--------------------------------------------------------------
void ofApp::draw(){
	int x = 16, y = 16; int ySize = 16;
	
	// MENU:
	ofDrawBitmapString("SOUND PLAYER EXAMPLE", x, y); y +=ySize*2;
	
	ofDrawBitmapString("press a z e r t y to play sounds 1 to 6", x, y); y +=ySize;
	ofDrawBitmapString("(will also become the selected sound)", x, y); y +=ySize*2;
	
	ofDrawBitmapString("press s to stop all sounds", x, y); y +=ySize*2;

	ofDrawBitmapString("press l to load the sounds, u to unload the sounds", x, y); y +=ySize*2;
	
	ofDrawBitmapString("press w x c v b n to select output devices 0 to 5", x, y);  y +=ySize;
	ofDrawBitmapString("(will be ignored if the device is not available)", x, y);  y +=ySize*2;
	
	ofDrawBitmapString("SELECTED SOUND:", x, y); y +=ySize;
	if(numSounds > 0) ofDrawBitmapString(sounds[selectedSound].getFileName(), x, y); y +=ySize*2;
	
	ofDrawBitmapString("LEVELS:", x, y);
	string  levels  = ofToString(sounds[selectedSound].levels[0])+" ";
		    levels += ofToString(sounds[selectedSound].levels[1])+" ";
			levels += ofToString(sounds[selectedSound].levels[2])+" ";
			levels += ofToString(sounds[selectedSound].levels[3])+" ";
			levels += ofToString(sounds[selectedSound].levels[4])+" ";
			levels += ofToString(sounds[selectedSound].levels[5])+" ";
			levels += ofToString(sounds[selectedSound].levels[6])+" ";
			levels += ofToString(sounds[selectedSound].levels[7])+" ";
	ofDrawBitmapString(levels, x+60, y); y +=ySize;

	ofDrawBitmapString("press 1 2 3 4 5 6 7 8 to select a level to modify", x, y); y +=ySize;
	ofDrawBitmapString("selected level: " + ofToString(selectedLevel), x, y); y +=ySize*2;
	
	ofDrawBitmapString("press + to increase a level, - to decrease.", x, y); y +=ySize;
	ofDrawBitmapString("(warning: depending on the soundcard, the ouput numbers are", x, y); y +=ySize;
	ofDrawBitmapString(" not necessary in the same order than on the card,", x, y); y +=ySize;
	ofDrawBitmapString(" please test)", x, y); y +=ySize;

	// Display file names :
	x = 512; y = 16;
	ofDrawBitmapString("AVAILABLE DEVICES:", x, y); y +=ySize;
	for(int i = 0; i < ofxFmodGetNumDevices(); ++i) {
		ofDrawBitmapString(ofToString(i), x, y);
		ofDrawBitmapString(ofxFmodGetDeviceNames()[i], x+24, y);
		y +=ySize;
	} y +=ySize;

	// Display devices :
	ofDrawBitmapString("SOUNDFILES:", x, y); y +=ySize;
	for(int i = 0; i < numSounds; ++i) {
		ofDrawBitmapString(ofToString(i), x, y);
		ofDrawBitmapString(sounds[i].getFileName(), x+24, y);
		y +=ySize;
	}
}

//--------------------------------------------------------------
void ofApp::keyPressed  (int key){
	// cout << "key = " << key << endl;
	float value = 0.0;
	
	switch(key) {
		case 'a' : sounds[0].play(); selectedSound = 0; break;
		case 'z' : sounds[1].play(); selectedSound = 1; break;
		case 'e' : sounds[2].play(); selectedSound = 2; break;
		case 'r' : sounds[3].play(); selectedSound = 3; break;
		case 't' : sounds[4].play(); selectedSound = 4; break;
		case 'y' : sounds[5].play(); selectedSound = 5; break;

		case 's' :
			for(int i = 0; i < numSounds; ++i)
				sounds[i].stop();
			break;
	
		case 'l' : loadSounds(); break;
		case 'u' : unloadSounds(); break;
			
		case 'w' : ofxFmodSetDevice(0); break;
		case 'x' : ofxFmodSetDevice(1); break;
		case 'c' : ofxFmodSetDevice(2); break;
		case 'v' : ofxFmodSetDevice(3); break;
		case 'b' : ofxFmodSetDevice(4); break;
		case 'n' : ofxFmodSetDevice(5); break;
			
		case '+' :
			value = sounds[selectedSound].getLevel(selectedLevel);
			value += 0.1; if(value > 0.9) value = 1;
			sounds[selectedSound].setLevel(selectedLevel, value);
			break;
		case '-' :
			value = sounds[selectedSound].getLevel(selectedLevel);
			value -= 0.1; if(value < 0.1) value = 0;
			sounds[selectedSound].setLevel(selectedLevel, value);
			break;

	}
	
	if((key >= '1') && (key <= '8')) {
		selectedLevel = key - '0';
		cout << "selectedLevel " << selectedLevel << endl;
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}
//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){

}
