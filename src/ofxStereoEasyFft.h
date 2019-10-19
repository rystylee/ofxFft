#pragma once

#include "ofMain.h"
#include "ofxFft.h"

class ofxStereoEasyFft : public ofBaseSoundInput{
public:
	ofxStereoEasyFft();
    ~ofxStereoEasyFft();
	void setup(int bufferSize = 512,
						 fftWindowType windowType = OF_FFT_WINDOW_HAMMING,
						 fftImplementation implementation = OF_FFT_BASIC,
						 int audioBufferSize = 256,
						 int audioSampleRate = 44100);
	void setUseNormalization(bool useNormalization);
	void update();
	vector<float>& getAudio(const int index);
	vector<float>& getBins(const int index);
	
	void audioReceived(float* input, int bufferSize, int nChannels);
	
	array<ofxFft*, 2> fft;
	ofSoundStream stream;
private:
	bool useNormalization;
	ofMutex soundMutex;
    vector<float> audioRaw;
    array<vector<float>, 2> audioFront, audioMiddle, audioBack;
	array<vector<float>, 2> bins;
	
	void normalize(vector<float>& data);

};
