#include "ofxStereoEasyFft.h"

ofxStereoEasyFft::ofxStereoEasyFft()
:useNormalization(true) {
}
ofxStereoEasyFft::~ofxStereoEasyFft(){
    stream.close();
}

void ofxStereoEasyFft::setup(int bufferSize, fftWindowType windowType, fftImplementation implementation, int audioBufferSize, int audioSampleRate) {
	if(bufferSize < audioBufferSize) {
		ofLogWarning("ofxStereoEasyFft") << "bufferSize (" << bufferSize << ") less than audioBufferSize (" << audioBufferSize << "), using " << audioBufferSize;
		bufferSize = audioBufferSize;
	}
	
    for (int i = 0; i < 2; i++)
    {
	    fft[i] = ofxFft::create(bufferSize / 2, windowType, implementation);
	    bins[i].resize(fft[i]->getBinSize());
	    audioFront[i].resize(bufferSize / 2);
	    audioMiddle[i].resize(bufferSize / 2);
	    audioBack[i].resize(bufferSize / 2);
    }
	audioRaw.resize(bufferSize);
	
    stream.getDeviceList();
    cout << stream.getDeviceList() << endl;
    stream.setup(0, 2, audioSampleRate, audioBufferSize, 4);
    stream.setDevice(stream.getDeviceList()[4]);
    stream.setInput(this);
}

void ofxStereoEasyFft::setUseNormalization(bool useNormalization) {
	this->useNormalization = useNormalization;
}

void ofxStereoEasyFft::update() {
	soundMutex.lock();
	audioFront[0] = audioMiddle[0];
	audioFront[1] = audioMiddle[1];
	soundMutex.unlock();
	
    for (int i = 0; i < 2; i++)
    {
	    fft[i]->setSignal(&audioFront[i][0]);
	    float* curFft = fft[i]->getAmplitude();
	    copy(curFft, curFft + fft[i]->getBinSize(), bins[i].begin());
	    normalize(bins[i]);
    }
}

vector<float>& ofxStereoEasyFft::getAudio(const int index) {
	return audioFront[index];
}

vector<float>& ofxStereoEasyFft::getBins(const int index) {
	return bins[index];
}

void ofxStereoEasyFft::audioReceived(float* input, int bufferSize, int nChannels) {
	if(audioRaw.size() > bufferSize) {
		copy(audioRaw.begin() + bufferSize, audioRaw.end(), audioRaw.begin()); // shift old
	}
	copy(input, input + bufferSize, audioRaw.end() - bufferSize); // push new

    for (int i = 0; i < 2; i++)
    {
	    //copy(audioRaw.begin(), audioRaw.end(), audioBack.begin());
        for (int j = 0; j < bufferSize / 2; j++)
        {
            if (j % 2 == i) audioBack[i][j] = audioRaw[j];
        }
	    normalize(audioBack[i]);

	    soundMutex.lock();
	    audioMiddle[i] = audioBack[i];
	    soundMutex.unlock();
    }
}



void ofxStereoEasyFft::normalize(vector<float>& data) {
	if(useNormalization) {
		float maxValue = 0;
		for(int i = 0; i < data.size(); i++) {
			if(abs(data[i]) > maxValue) {
				maxValue = abs(data[i]);
			}
		}
		for(int i = 0; i < data.size(); i++) {
			data[i] /= maxValue;
		}
	}
}
