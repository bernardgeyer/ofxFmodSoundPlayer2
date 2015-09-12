#include "ofxFmodSoundPlayer2.h"
#include "ofUtils.h"

static bool bFmodInitialized_ = false;
static bool bUseSpectrum_ = false;
static float fftValues_[8192];			//
static float fftInterpValues_[8192];			//
static float fftSpectrum_[8192];		// maximum #ofxFmodSoundPlayer2 is 8192, in fmodex....
static unsigned int buffersize = 1024;

// ADDITION:
static int ofxFmodPreferedDevice = 0;
static int ofxFmodDevice = 0;
static int ofxFmodNumDevices = 0;
static std::vector<std::string> ofxFmodDeviceNames;

// ---------------------  static vars
static FMOD_CHANNELGROUP * channelgroup;
static FMOD_SYSTEM       * sys;

// these are global functions, that affect every sound / channel:
// ------------------------------------------------------------
// ------------------------------------------------------------

//--------------------
void ofxFmodSoundStopAll(){
	ofxFmodSoundPlayer2::initializeFmod();
	FMOD_ChannelGroup_Stop(channelgroup);
}

//--------------------
void ofxFmodSoundSetVolume(float vol){
	ofxFmodSoundPlayer2::initializeFmod();
	FMOD_ChannelGroup_SetVolume(channelgroup, vol);
}

//--------------------
void ofxFmodSoundUpdate(){
	if (bFmodInitialized_){
		FMOD_System_Update(sys);
	}
}

//--------------------
float * ofxFmodSoundGetSpectrum(int nBands){

	ofxFmodSoundPlayer2::initializeFmod();


	// 	set to 0
	for (int i = 0; i < 8192; i++){
		fftInterpValues_[i] = 0;
	}

	// 	check what the user wants vs. what we can do:
	if (nBands > 8192){
		ofLogWarning("ofxFmodSoundPlayer2") << "ofFmodGetSpectrum(): requested number of bands " << nBands << ", using maximum of 8192";
		nBands = 8192;
	} else if (nBands <= 0){
		ofLogWarning("ofxFmodSoundPlayer2") << "ofFmodGetSpectrum(): requested number of bands " << nBands << ", using minimum of 1";
		nBands = 1;
		return fftInterpValues_;
	}

	// 	FMOD needs pow2
	int nBandsToGet = ofNextPow2(nBands);
	if (nBandsToGet < 64) nBandsToGet = 64;  // can't seem to get fft of 32, etc from fmodex

	// 	get the fft
	FMOD_System_GetSpectrum(sys, fftSpectrum_, nBandsToGet, 0, FMOD_DSP_FFT_WINDOW_HANNING);

	// 	convert to db scale
	for(int i = 0; i < nBandsToGet; i++){
        fftValues_[i] = 10.0f * (float)log10(1 + fftSpectrum_[i]) * 2.0f;
	}

	// 	try to put all of the values (nBandsToGet) into (nBands)
	//  in a way which is accurate and preserves the data:
	//

	if (nBandsToGet == nBands){

		for(int i = 0; i < nBandsToGet; i++){
			fftInterpValues_[i] = fftValues_[i];
		}

	} else {

		float step 		= (float)nBandsToGet / (float)nBands;
		//float pos 		= 0;
		// so for example, if nBands = 33, nBandsToGet = 64, step = 1.93f;
		int currentBand = 0;

		for(int i = 0; i < nBandsToGet; i++){

			// if I am current band = 0, I care about (0+1) * step, my end pos
			// if i > endPos, then split i with me and my neighbor

			if (i >= ((currentBand+1)*step)){

				// do some fractional thing here...
				float fraction = ((currentBand+1)*step) - (i-1);
				float one_m_fraction = 1 - fraction;
				fftInterpValues_[currentBand] += fraction * fftValues_[i];
				currentBand++;
				// safety check:
				if (currentBand >= nBands){
					ofLogWarning("ofxFmodSoundPlayer2") << "ofFmodGetSpectrum(): currentBand >= nBands";
				}

				fftInterpValues_[currentBand] += one_m_fraction * fftValues_[i];

			} else {
				// do normal things
				fftInterpValues_[currentBand] += fftValues_[i];
			}
		}

		// because we added "step" amount per band, divide to get the mean:
		for (int i = 0; i < nBands; i++){
			fftInterpValues_[i] /= step;
			if (fftInterpValues_[i] > 1)fftInterpValues_[i] = 1; 	// this seems "wrong"
		}

	}

	return fftInterpValues_;
}

void ofxFmodSetBuffersize(unsigned int bs){
	buffersize = bs;
}

// ------------------------------------------------------------
// ADDITION:

void ofxFmodSetPreferedDevice(int device){
	// Set the prefered device .
	// It will be set if possible, and if not 0 is set instead.
	ofxFmodPreferedDevice = device;
}

int ofxFmodGetPreferedDevice(void) {
	// Return the prefered device 
	return ofxFmodPreferedDevice;
}

int ofxFmodGetDevice(void) {
	// Return the selected device 
	return ofxFmodDevice;
}

int ofxFmodGetNumDevices(void) {
	// return the number of available devices
	return ofxFmodNumDevices;
}

std::vector<std::string> ofxFmodGetDeviceNames(void) {
	// Return a vector of strings containing the names of the devices
	return ofxFmodDeviceNames;
}

bool ofxFmodSetDevice(int deviceIndex){
	// Change the device
	// Warning: can make conflicts with other sound libraries
	FMOD_RESULT result = FMOD_System_SetDriver(sys, deviceIndex);
	
	if(result == FMOD_OK) {
		ofxFmodDevice = deviceIndex;
		return true;
	}
	else return false;
}

// ------------------------------------------------------------
// ------------------------------------------------------------


// now, the individual sound player:
//------------------------------------------------------------
ofxFmodSoundPlayer2::ofxFmodSoundPlayer2(){
	bLoop 			= false;
	bLoadedOk 		= false;
	pan 			= 0.0; // range for oF is -1 to 1
	volume 			= 1.0f;
	internalFreq 	= 44100;
	speed 			= 1;
	bPaused 		= false;
	isStreaming		= false;

// Additions:
    bIsPlaying      = false;

	for(int i = 0; i < 8; ++i) levels[i] = 0.75;
}

ofxFmodSoundPlayer2::~ofxFmodSoundPlayer2(){
	unload();
}



//---------------------------------------
// this should only be called once
void ofxFmodSoundPlayer2::initializeFmod(){
	if(!bFmodInitialized_){
		
		FMOD_System_Create(&sys);
		
		// **************************** SELECT DEVICE **************************
		
		static bool listCreated = false;
		
		if(listCreated == false) {
            FMOD_System_GetNumDrivers(sys, &ofxFmodNumDevices);
			
            for(int i = 0; i < ofxFmodNumDevices; i++)
            {
                char name[256];
                FMOD_System_GetDriverInfo(sys, i, name, 256, 0);				
                ofxFmodDeviceNames.push_back(name);
            }
			
            listCreated = true;
			
			cout << "--------------------------------\n";
			cout << "FMOD: Available Sound Outputs:\n";
			for(int i = 0; i < ofxFmodNumDevices; i++) {cout << i << " " <<  ofxFmodDeviceNames[i] << endl;}
			cout << "--------------------------------\n";
        }
		        
		if (ofxFmodPreferedDevice < ofxFmodNumDevices) {
			ofxFmodDevice = ofxFmodPreferedDevice;
            cout << "Selected device = " << ofxFmodDevice << endl << endl;
		}
		else {
            cout << "Warning: selected device not available, use default" << endl << endl;
            ofxFmodDevice = 0;
        }
		
		// *********************************************************************

		// set buffersize, keep number of buffers
		unsigned int bsTmp;
		int nbTmp;
		FMOD_System_GetDSPBufferSize(sys, &bsTmp, &nbTmp);
		FMOD_System_SetDSPBufferSize(sys, buffersize, nbTmp);

		#ifdef TARGET_LINUX
			FMOD_System_SetOutput(sys,FMOD_OUTPUTTYPE_ALSA);
		#endif
		
        FMOD_System_SetDriver(sys, ofxFmodDevice);
		FMOD_System_SetSpeakerMode(sys, FMOD_SPEAKERMODE_7POINT1); // NEW

		FMOD_System_Init(sys, 32, FMOD_INIT_NORMAL, 0);  //do we want just 32 channels?
		FMOD_System_GetMasterChannelGroup(sys, &channelgroup);
		bFmodInitialized_ = true;
	}
}




//---------------------------------------
void ofxFmodSoundPlayer2::closeFmod(){
	if(bFmodInitialized_){
		FMOD_System_Close(sys);
		bFmodInitialized_ = false;
	}
}

//------------------------------------------------------------
bool ofxFmodSoundPlayer2::load(string fileName, bool stream){

	this->fileName = fileName;
	
	fileName = ofToDataPath(fileName);

	// fmod uses IO posix internally, might have trouble
	// with unicode paths...
	// says this code:
	// http://66.102.9.104/search?q=cache:LM47mq8hytwJ:www.cleeker.com/doxygen/audioengine__fmod_8cpp-source.html+FSOUND_Sample_Load+cpp&hl=en&ct=clnk&cd=18&client=firefox-a
	// for now we use FMODs way, but we could switch if
	// there are problems:

	bMultiPlay = false;

	// [1] init fmod, if necessary

	initializeFmod();

	// [2] try to unload any previously loaded sounds
	// & prevent user-created memory leaks
	// if they call "loadSound" repeatedly, for example

	unload();

	// [3] load sound

	//choose if we want streaming
	int fmodFlags =  FMOD_SOFTWARE;
	if(stream)fmodFlags =  FMOD_SOFTWARE | FMOD_CREATESTREAM;

	result = FMOD_System_CreateSound(sys, fileName.c_str(),  fmodFlags, 0, &sound);

	if (result != FMOD_OK){
		bLoadedOk = false;
		ofLogError("ofxFmodSoundPlayer2") << "loadSound(): could not load \"" << fileName << "\"";
	} else {
		bLoadedOk = true;
		FMOD_Sound_GetLength(sound, &length, FMOD_TIMEUNIT_PCM);
		isStreaming = stream;
	}

	return bLoadedOk;
}

//------------------------------------------------------------
void ofxFmodSoundPlayer2::unload(){
	if (bLoadedOk){
		stop();						// try to stop the sound
		FMOD_Sound_Release(sound);
		bLoadedOk = false;
	}
}

//------------------------------------------------------------
bool ofxFmodSoundPlayer2::isPlaying() const{

	if (!bLoadedOk) return false;

	int playing = 0;
	FMOD_Channel_IsPlaying(channel, &playing);
	return (playing != 0 ? true : false);
}

//------------------------------------------------------------
float ofxFmodSoundPlayer2::getSpeed() const{
	return speed;
}

//------------------------------------------------------------
float ofxFmodSoundPlayer2::getPan() const{
	return pan;
}

//------------------------------------------------------------
float ofxFmodSoundPlayer2::getVolume() const{
	return volume;
}

//------------------------------------------------------------
bool ofxFmodSoundPlayer2::isLoaded() const{
	return bLoadedOk;
}

//------------------------------------------------------------
void ofxFmodSoundPlayer2::setVolume(float vol){
	if (isPlaying()){
		FMOD_Channel_SetVolume(channel, vol);
	}
	volume = vol;
}

//------------------------------------------------------------
void ofxFmodSoundPlayer2::setPosition(float pct){
	if (isPlaying()){
		int sampleToBeAt = (int)(length * pct);
		FMOD_Channel_SetPosition(channel, sampleToBeAt, FMOD_TIMEUNIT_PCM);
	}
}

void ofxFmodSoundPlayer2::setPositionMS(int ms) {
	if (isPlaying()){
		FMOD_Channel_SetPosition(channel, ms, FMOD_TIMEUNIT_MS);
	}
}

//------------------------------------------------------------
float ofxFmodSoundPlayer2::getPosition() const{
	if (isPlaying()){
		unsigned int sampleImAt;

		FMOD_Channel_GetPosition(channel, &sampleImAt, FMOD_TIMEUNIT_PCM);

		float pct = 0.0f;
		if (length > 0){
			pct = sampleImAt / (float)length;
		}
		return pct;
	} else {
		return 0;
	}
}

//------------------------------------------------------------
int ofxFmodSoundPlayer2::getPositionMS() const{
	if (isPlaying()){
		unsigned int sampleImAt;

		FMOD_Channel_GetPosition(channel, &sampleImAt, FMOD_TIMEUNIT_MS);

		return sampleImAt;
	} else {
		return 0;
	}
}

//------------------------------------------------------------
void ofxFmodSoundPlayer2::setPan(float p){
	pan = p;
	p = ofClamp(p, -1, 1);
	if (isPlaying()){
		FMOD_Channel_SetPan(channel,p);
	}
}


//------------------------------------------------------------
void ofxFmodSoundPlayer2::setPaused(bool bP){
	if (isPlaying()){
		FMOD_Channel_SetPaused(channel,bP);
		bPaused = bP;        
		bIsPlaying = ! bP; // NEW!
	}
}


//------------------------------------------------------------
void ofxFmodSoundPlayer2::setSpeed(float spd){
	if (isPlaying()){
			FMOD_Channel_SetFrequency(channel, internalFreq * spd);
	}
	speed = spd;
}


//------------------------------------------------------------
void ofxFmodSoundPlayer2::setLoop(bool bLp){
	if (isPlaying()){
		FMOD_Channel_SetMode(channel,  (bLp == true) ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);
	}
	bLoop = bLp;
}

// ----------------------------------------------------------------------------
void ofxFmodSoundPlayer2::setMultiPlay(bool bMp){
	bMultiPlay = bMp;		// be careful with this...
}

// ----------------------------------------------------------------------------
void ofxFmodSoundPlayer2::play(){

	// if it's a looping sound, we should try to kill it, no?
	// or else people will have orphan channels that are looping
	if (bLoop == true){
		FMOD_Channel_Stop(channel);
	}

	// if the sound is not set to multiplay, then stop the current,
	// before we start another
	if (!bMultiPlay){
		FMOD_Channel_Stop(channel);
	}

	FMOD_System_PlaySound(sys, FMOD_CHANNEL_FREE, sound, bPaused, &channel);

	FMOD_Channel_GetFrequency(channel, &internalFreq);
	FMOD_Channel_SetVolume(channel,volume);
	FMOD_Channel_SetSpeakerMix(channel, levels[0], levels[1], levels[2], levels[3], levels[4], levels[5], levels[6], levels[7]);
	FMOD_Channel_SetFrequency(channel, internalFreq * speed);
	FMOD_Channel_SetMode(channel,  (bLoop == true) ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);

	//fmod update() should be called every frame - according to the docs.
	//we have been using fmod without calling it at all which resulted in channels not being able
	//to be reused.  we should have some sort of global update function but putting it here
	//solves the channel bug
	FMOD_System_Update(sys);
	FMOD_Channel_SetPaused(channel, false);

    bIsPlaying = true; // NEW!
}

// ----------------------------------------------------------------------------
void ofxFmodSoundPlayer2::stop(){
	FMOD_Channel_Stop(channel);
	
    bIsPlaying = false; // NEW!
}

// ----------------------------------------------------------------------------

std::string ofxFmodSoundPlayer2::getFileName() { return fileName; }

void ofxFmodSoundPlayer2::setLevel(int index, float level)
// Change one level (to set more than one, use setMix4 or setMix8)
// index [1..8], value [0..1]
{
	if(index < 1) return; else if(index > 8) return; 
	if(level < 0.0) level = 0.0; else if(level > 1.0) level = 1.0; 
	
	levels[index-1] = level;

	if (isPlaying() == true){
		FMOD_Channel_SetSpeakerMix(channel, levels[0], levels[1], levels[2], levels[3], levels[4], levels[5], levels[6], levels[7]);
	}
}

float ofxFmodSoundPlayer2::getLevel(int index)
// Return one level, index [1..8]
{
	if(index < 1) return 0; else if(index > 8) return 0; 
	
	return levels[index-1];
}

void ofxFmodSoundPlayer2::setMix4(float l1, float l2, float l3, float l4)
{
	levels[0] = l1; levels[1] = l2; levels[2] = l3; levels[3] = l4;
	
	if (isPlaying() == true){
		FMOD_Channel_SetSpeakerMix(channel, levels[0], levels[1], levels[2], levels[3], levels[4], levels[5], levels[6], levels[7]);
	}
}

void ofxFmodSoundPlayer2::setMix8(float l1, float l2, float l3, float l4, float l5, float l6, float l7, float l8)
{
	levels[0] = l1; levels[1] = l2; levels[2] = l3; levels[3] = l4;
	levels[4] = l5; levels[5] = l6; levels[6] = l7; levels[7] = l8;
	
	if (isPlaying() == true){
		FMOD_Channel_SetSpeakerMix(channel, levels[0], levels[1], levels[2], levels[3], levels[4], levels[5], levels[6], levels[7]);
	}
}


