#include "ofxStereoProcessFFT.h"

void StereoProcessFFT::setup(){
    
    scaleFactor = 10000;
    numBins = 16384;
    
    fft.setup(numBins); //default
    fft.setUseNormalization(false);
    
    graphMaxSize = 200; //approx 10sec of history at 60fps
    
    for (int i = 0; i < 2; i++)
    {
        graphLow[i].assign(graphMaxSize, 0.0);
        graphMid[i].assign(graphMaxSize, 0.0);
        graphHigh[i].assign(graphMaxSize, 0.0);
        graphSuperLow[i].assign(graphMaxSize, 0.0);
        graphMaxSound[i].assign(graphMaxSize, 200.0);
    }
    
    saveHistory = false;
    
    exponent = 1.0;
    
    numFFTbins = 32;
    FFTpercentage = 0.14;
    
    for (int i = 0; i < 2; i++)
    {
        delta[i] = loudestBand[i] = noisiness[i] = maxSound[i] = avgMaxSoundOverTime[i] = 0;
    }
    
    normalize = false;
    volumeRange = 400; //only used if normalize is false

}

//---------------------------------------------
void StereoProcessFFT::update() {
    fft.update();
    if (saveHistory) {
        for (int i = 0; i < 2; i++)
        {
            if (graphHigh[i].size() > graphMaxSize) {
                graphHigh[i].erase(graphHigh[i].begin(), graphHigh[i].begin() + 1);
                graphMid[i].erase(graphMid[i].begin(), graphMid[i].begin() + 1);
                graphLow[i].erase(graphLow[i].begin(), graphLow[i].begin() + 1);
                graphSuperLow[i].erase(graphSuperLow[i].begin(), graphSuperLow[i].begin() + 1);
            }
        }
    }

    for (int i = 0; i < 2; i++)
    {
        if (graphMaxSound[i].size() > graphMaxSize) { //make sure this is always running!
            graphMaxSound[i].erase(graphMaxSound[i].begin(), graphMaxSound[i].begin() + 1);
        }
        calculateFFT(i, fft.getBins(i), FFTpercentage, numFFTbins);
    }

}

//---------------------------------------------
void StereoProcessFFT::calculateFFT(const int index, vector<float>&buffer, float _FFTpercentage, int _numFFTbins){
    
    this->numFFTbins = _numFFTbins;
    this->FFTpercentage = _FFTpercentage;
    
    fftSpectrum[index].clear(); //empty it all
    
    float loudBand = 0;
    maxSound[index] = 0;
    float freqDelta = 0;
    
    for(int i = 0; i<numFFTbins; i++){
        fftSpectrum[index].push_back(0); //init the vector for each pass
    }
    
    //sort through and find the loudest sound
    //use the loudest sound to normalize it to the proper range of 0-1
    //drop all those values into the fftSpectrum
    
    //average first
    for(int i=0; i<fftSpectrum[index].size(); i++){ //for the number of columns
        float bin_size = buffer.size()*FFTpercentage;
        
        for (int j=(bin_size*((float)i/numFFTbins)); j<bin_size*((float)1/numFFTbins)+(bin_size*((float)i/numFFTbins)) ; j++) { //for each i position, average the values in i's+offset
            fftSpectrum[index][i] = fftSpectrum[index][i] + buffer[j]*10000; //sum values in each section of buffers. Multiply by 10000 so you're not dealing with tiny numbers.
        }
        
        fftSpectrum[index][i] = abs((fftSpectrum[index][i]/(bin_size*(float)1/numFFTbins))*(1+pow(i, exponent)/numFFTbins));//Then make low frequency values weighted lower than high frequency with pow
        
        //find maximum band
        if (maxSound[index]<fftSpectrum[index][i]) {
            maxSound[index] = fftSpectrum[index][i];
            loudestBand[index] = i;
        }
    }
    
    graphMaxSound[index].push_back(maxSound[index]); //accumulate loudest sounds
    
    float accumMaxSounds;
    for (int i =0; i<graphMaxSound[index].size(); i++) {
        accumMaxSounds = accumMaxSounds+graphMaxSound[index][i]; //add up all loudest sounds
    }
    
    avgMaxSoundOverTime[index] = accumMaxSounds/graphMaxSound[index].size(); //take average over a certain number of frames
    
    float meanSum=0;
    float mean,stdDev,stdDevAccum, variance,deviation;
    
    float spectralCentroidAccum, spectralWeightsAccum, spectralWeights;
    
    
    for(int i=0; i<fftSpectrum[index].size(); i++){ //for the number of columns
        
        //NORMALIZE
        if(normalize){
            fftSpectrum[index][i] = ofMap(fftSpectrum[index][i], 0, avgMaxSoundOverTime[index], 0, 1, true); //normalize each frame to 0-1
        }
        
        //COMPUTE NOISINESS
        //compute standard deviation - this works OK, but isn't iron clad - can detect single notes versus multiple notes
        //REMOVING FOR FULL RUN - NOT BEING USED
        /*
        meanSum = meanSum + fftSpectrum[i]; //add up all values
        
        mean=meanSum/fftSpectrum.size(); //compute the mean
        
        deviation = (fftSpectrum[i]-mean)*(fftSpectrum[i]-mean);
        
        stdDevAccum = deviation + stdDevAccum;
        
        variance = stdDevAccum/(fftSpectrum.size()-1);
        
        noisiness = stdDev = sqrt(variance);
        
        //AVERAGE PITCH/SPECTRAL CENTROID
        //compute spectral centroid/average pitch - this is not quite right yet
        spectralCentroidAccum += i*fftSpectrum[i];
        
        spectralWeightsAccum += fftSpectrum[i];
        
        spectralCentroid = spectralCentroidAccum/spectralWeightsAccum; //gives the average band that is the loudest
        */
        
        //EQ BANDS
        if (i==1 ) {
            superLowEqAvg[index] = (fftSpectrum[index][0]); //just compute the lowest bass bin - not an average
        }
        
        //find bands for each 3rd of the entire thing...this is not musically accurate, just a rough estimate
        if (i>0 && i<numFFTbins*.333) {
            lowEqAvg[index]= lowEqAvg[index]+fftSpectrum[index][i];
        }
        if (i>numFFTbins*.33 && i<numFFTbins*.666) {
            midEqAvg[index]= midEqAvg[index]+fftSpectrum[index][i];
        }
        if (i>numFFTbins*.666 && i<numFFTbins) {
            highEqAvg[index] = highEqAvg[index]+fftSpectrum[index][i];
        }
    }
    
    freqDelta = freqDelta/numFFTbins;
    
    // superLowEqAvg = (float)superLowEqAvg/2; //only doing it off the lowest ones
    lowEqAvg[index] = lowEqAvg[index]/(numFFTbins*.333); //take a third of the entire bin collection to decide about low/mid/high
    midEqAvg[index] = midEqAvg[index]/(numFFTbins*.333);
    highEqAvg[index] = highEqAvg[index]/(numFFTbins*.333);
    
    if(saveHistory){ //only save these if drawing
        graphSuperLow[index].push_back(superLowEqAvg[index]);
        graphLow[index].push_back(lowEqAvg[index]);
        graphMid[index].push_back(midEqAvg[index]);
        graphHigh[index].push_back(highEqAvg[index]);
    }
    
}

//---------------------------------------------
void StereoProcessFFT::drawHistoryGraph(const int index, ofPoint pt, fftRangeType drawType){
    
    saveHistory=true; //only do this if drawing
    
    switch (drawType) {
        case SUPERLOW:
            drawAvgGraph(pt, graphSuperLow[index], ofColor(0, 100, 255,200));
            break;
        case LOW:
            drawAvgGraph(pt, graphLow[index], ofColor(0, 100, 255,200));
            break;
        case MID:
            drawAvgGraph(pt, graphMid[index], ofColor(0, 255, 100,200));
            break;
        case HIGH:
            drawAvgGraph(pt, graphHigh[index],ofColor(255, 0, 100,200));
            break;
        case MAXSOUND:
            drawAvgGraphUnScaled(pt, graphMaxSound[index],ofColor(255, 100, 255,200));
            break;
        default:
            drawAvgGraphUnScaled(pt, graphMaxSound[index],ofColor(255, 100, 255,200));
            break;
    }
    

    
}

//---------------------------------------------
void StereoProcessFFT::drawAvgGraph(ofPoint pt, vector<float> values, ofColor _color){

    if (normalize) {
        ofEnableAlphaBlending();
        ofPushMatrix();
        ofFill();
        ofSetColor(_color);
        ofTranslate(pt.x, pt.y);
        ofBeginShape();
    
        float avgVal;
        for (int i = 0; i < (int)ofMap(values.size(), 0 , values.size(), 0,200); i++){ //scale it to be 200px wide
            if( i == 0 ) ofVertex(i, 200);
            
            ofVertex(i,ofMap(values[(int)ofMap(i, 0 , 200, 0,values.size())], 0, 1, 200, 0,true));
            
            avgVal = avgVal+values[(int)ofMap(i, 0 , 200, 0,values.size())];
            if( i == 200 -1 ) ofVertex(i, 200);
        }
    
        avgVal = avgVal/values.size();
        
        ofEndShape(false);
        ofSetColor(255);
        ofDrawLine(0,ofMap(avgVal, 0, 1, 200, 0,true) , 200, ofMap(avgVal, 0, 1, 200, 0,true));
        ofPopMatrix();
        ofDisableAlphaBlending();
    }else{
        //not normalized
        ofEnableAlphaBlending();
        ofPushMatrix();
        ofFill();
        ofSetColor(_color);
        ofTranslate(pt.x, pt.y);
        ofBeginShape();
        
        float avgVal;
        for (int i = 0; i < (int)ofMap(values.size(), 0 , values.size(), 0,200); i++){ //scale it to be 200px wide
            if( i == 0 ) ofVertex(i, 200);
            
            ofVertex(i,ofMap(values[(int)ofMap(i, 0 , 200, 0,values.size())], 0, volumeRange, 200, 0,true));
            
            avgVal = avgVal+values[(int)ofMap(i, 0 , 200, 0,values.size())];
            if( i == 200 -1 ) ofVertex(i, 200);
        }
        
        avgVal = avgVal/values.size();
        
        ofEndShape(false);
        ofSetColor(255);
        ofDrawLine(0,ofMap(avgVal, 0, volumeRange, 200, 0,true) , 200, ofMap(avgVal, 0, volumeRange, 200, 0,true));
        ofPopMatrix();
        ofDisableAlphaBlending();
    }
    
}
//---------------------------------------------
void StereoProcessFFT::drawAvgGraphUnScaled(ofPoint pt, vector<float> values, ofColor _color){
        ofEnableAlphaBlending();
        ofPushMatrix();
        ofFill();
        ofSetColor(_color);
        ofTranslate(pt.x, pt.y);
        ofBeginShape(); //then do the average again
    
    float prevAvgMaximum = 0;
        float avgMaximum =0;
    
        for (int i = 0; i < (int)ofMap(values.size(), 0 , values.size(), 0,200); i++){
            if( i == 0 ) ofVertex(i, 200);
            
            ofVertex(i,ofMap(values[(int)ofMap(i, 0 , 200, 0,values.size())], 0, volumeRange, 200, 0,true));
            
            avgMaximum = avgMaximum+values[(int)ofMap(i, 0 , 200, 0,values.size())];
            
            if (i<((int)ofMap(i, 0 , 200, 0,values.size()))/2) {
                prevAvgMaximum = prevAvgMaximum + values[(int)ofMap(i, 0 , 200, 0,values.size())]; //take half of the bin and get the average and compare that to the whole thing
            }
            
            if( i == 200 -1 ) ofVertex(i, 200);
        }
        
        ofEndShape(false);
    
    avgMaximum = avgMaximum/values.size();
    prevAvgMaximum = prevAvgMaximum/(values.size()/2);
    
   // cout<< "Delta: " << avgMaximum - prevAvgMaximum <<endl;
    
    //make trigger a percentage of the current max volume
    
    
    
    ofSetColor(255);
    ofDrawLine(0,ofMap(avgMaximum, 0, volumeRange, 200, 0,true) , 200, ofMap(avgMaximum, 0,volumeRange, 200, 0,true));

    
        ofPopMatrix();
        ofDisableAlphaBlending();
}

//---------------------------------------------
void StereoProcessFFT::drawBars(){
    //if(normalize){
    //ofPushStyle();
    //ofSetRectMode(OF_RECTMODE_CORNER);
    //ofSetLineWidth(2);
    //for(int i=0; i<fftSpectrum.size(); i++){ //for the number of columns
    //    if (i==loudestBand) {
    //        ofSetColor(255,0,0);
    //    }
    //    else{
    //        ofSetColor(100,100,200);
    //    }
    //    ofNoFill();
    //    ofDrawRectangle(ofGetWidth()*((float)i/numFFTbins), ofGetHeight()-20, ofGetWidth()/numFFTbins, -ofMap(fftSpectrum[i], 0, 1, 0, ofGetHeight() -50));
    //}
    //ofPopStyle();
    //}else{
    //    //not normalized
    //    ofPushStyle();
    //    ofSetRectMode(OF_RECTMODE_CORNER);
    //    ofSetLineWidth(2);
    //    for(int i=0; i<fftSpectrum.size(); i++){ //for the number of columns
    //        if (i==loudestBand) {
    //            ofSetColor(255,0,0);
    //        }
    //        else{
    //            ofSetColor(100,100,200);
    //        }
    //        ofNoFill();
    //        ofDrawRectangle(ofGetWidth()*((float)i/numFFTbins), ofGetHeight()-20, ofGetWidth()/numFFTbins, -ofMap(fftSpectrum[i], 0, volumeRange, 0, ofGetHeight() -50));
    //    }
    //    ofPopStyle();
    //}
}

//---------------------------------------------
void StereoProcessFFT::drawDebug(){
    //ofPushMatrix();
    //ofDrawBitmapStringHighlight("Loudest Band: " + ofToString(loudestBand), 250,20);
    //ofDrawBitmapStringHighlight("Curr. Max Sound Val: "+ ofToString(maxSound), 250,40);
    //ofDrawBitmapStringHighlight("Super Low Avg: " + ofToString(superLowEqAvg), 250,60);
    //ofDrawBitmapStringHighlight("Low Avg: " + ofToString(lowEqAvg), 250,80);
    //ofDrawBitmapStringHighlight("Mid Avg: " + ofToString(midEqAvg), 250,100);
    //ofDrawBitmapStringHighlight("High Avg: " + ofToString(highEqAvg), 250,120);
    //ofDrawBitmapStringHighlight("Noisiness: " + ofToString(noisiness), 250,140);
    //ofDrawBitmapStringHighlight("SpectralCentroid: " + ofToString(spectralCentroid), 250,160);
    //ofDrawBitmapStringHighlight("Avg Max Sound: " + ofToString(avgMaxSoundOverTime), 250,180);
    //ofDrawBitmapStringHighlight("Delta: " + ofToString(getDelta()), 250,200);
    //ofDrawBitmapStringHighlight("Delta Shift Detected: " + ofToString(abs(getDelta())>(avgMaxSoundOverTime*.20)), 250,220);
    //
    //
    //ofDrawBitmapStringHighlight("Freq Range up to: " +ofToString(ofMap(FFTpercentage, 0, 0.23, 0, 5000)) + "hz", 450,60);
    //float freqPerBin = ofMap(FFTpercentage, 0, 0.23, 0, 5000)/numFFTbins;
    //ofDrawBitmapStringHighlight("Freq range per bin: " +ofToString(freqPerBin) + "hz", 450,80);
    //ofDrawBitmapStringHighlight("Approx Number of octaves from C0: " +ofToString(ofMap(ofMap(FFTpercentage, 0, 0.23, 0, 5000),0,5000,0,8)), 450,100); //wrong wrong wrong - octave frequency doubles as it goes up - octave is from n to 2n hz, so do more math for this
    //ofDrawBitmapStringHighlight("Approx Freq of Loudest Band: " +ofToString(freqPerBin*loudestBand)+"hz", 450,120);
    //ofPopMatrix();
}


//GETTERS

float StereoProcessFFT::getIntensityAtFrequency(const int index, float _freq){
    //Todo: figure out this calculation from the raw bins
    //8193 bins
    
   // fft.getBins()[fft.getBins().size()];
    
    int whichBin;
    whichBin = ofMap(_freq, 0, 22100, 0, fft.getBins(index).size()); //An approximation...
    
    float normalizedFreq;
    normalizedFreq = ofMap(fft.getBins(index)[whichBin]*scaleFactor, 0, avgMaxSoundOverTime[index], 0, 1,true); //the scalefactor is just a scaling factor to have easier numbers to look at
    
    return normalizedFreq;
    
}

float StereoProcessFFT::getDelta(const int index){
    float prevAvgMaximum = 0;
    float avgMaximum =0;
    
    for (int i = 0; i < graphMaxSound.size(); i++){
              
        avgMaximum = avgMaximum + graphMaxSound[index][i];
        
        if (i<graphMaxSound.size()/2) {
            prevAvgMaximum = prevAvgMaximum + graphMaxSound[index][i]; //take half of the bin and get the average and compare that to the whole thing
        }
        
    }
    
    avgMaximum = avgMaximum/graphMaxSound.size();
    prevAvgMaximum = prevAvgMaximum/(graphMaxSound.size()/2);
    
    delta[index] = avgMaximum - prevAvgMaximum;
    return delta[index]; //if the delta is greater than a percentage of the maximum volume, then trigger an event (delta scales to maximum volume)
}


float StereoProcessFFT::getUnScaledLoudestValue(const int index){
    //This returns the unnormalized value of the current loudest sound - useful for detecting whether the volume input is low or really high
    return maxSound[index];
}

float StereoProcessFFT::getSmoothedUnScaledLoudestValue(const int index){
    return avgMaxSoundOverTime[index];
}

vector<float> StereoProcessFFT::getSpectrum(const int index){
    return fftSpectrum[index];
}

float StereoProcessFFT::getNoisiness(const int index){
    return noisiness[index];
}

bool StereoProcessFFT::getNormalized(){
    return normalize;
}

float StereoProcessFFT::getLoudBand(const int index){
    return loudestBand[index]; //Todo: this needs to be an average
}

float StereoProcessFFT::getSuperLowVal(const int index){
    return superLowEqAvg[index]; //this is NOT smoothed, but outputs
}

float StereoProcessFFT::getLowVal(const int index){
    return lowEqAvg[index];
}

float StereoProcessFFT::getMidVal(const int index){
    return midEqAvg[index];
}

float StereoProcessFFT::getHighVal(const int index){
    return highEqAvg[index];
}

float StereoProcessFFT::getFFTpercentage(){
    return FFTpercentage;
}

float StereoProcessFFT::getExponent(){
    return FFTpercentage;
}

int StereoProcessFFT::getNumFFTbins(){
    return numFFTbins;
}

//SETTERS
void StereoProcessFFT::setFFTpercentage(float _FFTpercentage){
    this->FFTpercentage=_FFTpercentage;
}

void StereoProcessFFT::setExponent(float _exponent){
    this->exponent=_exponent;
}

void StereoProcessFFT::setNumFFTBins(int _numFFTBins){
    this->numFFTbins = _numFFTBins;
}

void StereoProcessFFT::setHistorySize(int _framesOfHistory){
    this->graphMaxSize = _framesOfHistory;
}

void StereoProcessFFT::setNormalize(bool _normalize){
    this->normalize = _normalize;
}

void StereoProcessFFT::setVolumeRange(int _volumeRange){
    this->volumeRange = _volumeRange;
}
