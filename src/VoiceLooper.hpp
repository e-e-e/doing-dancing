//
//  VoiceLooper.hpp
//  AudioExperiment
//
//  Created by Benjamin Forster on 11/11/2016.
//
//

#ifndef VoiceLooper_hpp
#define VoiceLooper_hpp

#include <stdio.h>

#include "cinder/audio/Context.h"
#include "cinder/audio/SampleRecorderNode.h"
#include "cinder/audio/SamplePlayerNode.h"
#include "cinder/audio/GainNode.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class VoiceLooper {
    
    const int duration;
    int recordingCount = 0;
    Timer recordingTimer;
    
    audio::InputDeviceNodeRef input;
    audio::BufferRecorderNodeRef audioRecorder;
    audio::BufferPlayerNodeRef bufferPlayer;
    audio::GainNodeRef previousNoise;
    audio::GainNodeRef currentNoise;
    
public:
    
    VoiceLooper();
    VoiceLooper(int);
    ~VoiceLooper();
    
    inline bool isStopped() const { return recordingTimer.isStopped(); }
    inline bool hasBeenAMoment() const {
        return ( !recordingTimer.isStopped()
                && recordingTimer.getSeconds() > (duration/2.0f) ); }
    
    void start();
    void setup();
    void update();


private:
    
    void stop();
    
};


#endif /* VoiceLooper_hpp */
