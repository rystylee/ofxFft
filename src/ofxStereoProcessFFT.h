#pragma once
#include "ofMain.h"
#include "ofxStereoEasyFft.h"

//This does a simple breakout of Kyle McDonald's EasyFFT class from his ofxFFT implementation

enum fftRangeType {SUPERLOW, LOW, MID, HIGH, MAXSOUND};

class StereoProcessFFT {
public:

        
    ofxStereoEasyFft fft;
    
    void setup(); //whether you want your sounds in a 0-1 range or a 0-volumeRange - setting volume range doesn't matter if you're normalizing
    void update();
    //For Debugging audio trends
    void drawHistoryGraph(const int index, ofPoint pt, fftRangeType drawType);
    
    //To feed to other components
    float getLoudBand(const int index);
    float getSuperLowVal(const int index);
    float getLowVal(const int index);
    float getMidVal(const int index);
    float getHighVal(const int index);
    float getNoisiness(const int index); //not currently implemented
    bool  getNormalized();
    float getSpectralCentroid(const int index); //this is not currently implemented
    float getDelta(const int index);
    float getUnScaledLoudestValue(const int index);
    float getSmoothedUnScaledLoudestValue(const int index);
    
    float getIntensityAtFrequency(const int index, float _freq);
    vector<float> getSpectrum(const int index);

    const vector<float>& getGraphLow(const int index) const { return graphLow[index]; }
    const vector<float>& getGraphMid(const int index) const { return graphMid[index]; }
    const vector<float>& getGraphHigh(const int index) const { return graphHigh[index]; }
    
    int getNumFFTbins();
    float getFFTpercentage();
    float getExponent();
    
    void setNumFFTBins(int _numFFTBins);
    void setFFTpercentage(float _FFTpercentage);
    void setExponent(float _Exponent);
    void setHistorySize(int _framesOfHistory);
    void setVolumeRange(int _volumeRange);
    void setNormalize(bool _normalize);
    void setSaveHistory(bool _saveHistory) { saveHistory = _saveHistory; }
    
    void drawBars();
    void drawDebug();
    

    
private:
    
    bool    normalize; //decide if you want the values between 0 and 1 or between 0 - 1000
    int     volumeRange; //use if you're not normalizing so you can give things a proper range for visualization
    
    int     scaleFactor; //this is arbitrary - it raises the FFT numbers so they aren't 0.0000054
    int     numBins;
    array<float, 2>   noisiness;
    float   spectralCentroid;
    
    array<float, 2>   delta;

    void    calculateFFT(const int index, vector<float>& buffer, float FFTpercentage, int numFFTbins);
    
    bool    saveHistory;
    int     graphMaxSize; //number of frames that the values average over if you want a longer tail
    
    int     numFFTbins; //how many Columns are we analyzing - 16-32, etc
    float   FFTpercentage; //how much of the FFT are we parsing out into bins? usually around 20% of the whole 0-5khz
    float   exponent; //this is a factor for making the high end be valued slightly more than the low end since the bass tends to be higher energy than the high end in FFT - in a range from 1.0-1.6
    
    array<int, 2>     loudestBand; //for each frame - this is the loudest frequency band at that time
    
    array<float, 2>   maxSound; //this is the loudest sound for each frame - unnormalized
    
    array<float, 2>   avgMaxSoundOverTime; //average max sound in relation to the graph max sound
    
    array<vector <float>, 2> fftSpectrum; //this holds the amplitudes of multiple columns of the FFT
    array<vector <float>, 2> graphLow;
    array<vector <float>, 2> graphMid;
    array<vector <float>, 2> graphHigh;
    array<vector <float>, 2> graphSuperLow;
    array<vector <float>, 2> graphMaxSound;
    
    array<float, 2> superLowEqAvg, lowEqAvg, midEqAvg, highEqAvg;
    
    void drawAvgGraph(ofPoint pt, vector<float> values, ofColor _color);
    void drawAvgGraphUnScaled(ofPoint pt, vector<float> values, ofColor _color);
    
};
