#pragma once

#ifndef DoingDanceingApp_h
#define DoingDanceingApp_h

#include "cinder/app/App.h"

#include "CaptureLooper.hpp"
#include "VoiceLooper.hpp"

using namespace ci;
using namespace ci::app;
using namespace std;

#define DOING_DANCING_VOICE 0
#define DOING_DANCING_VIDEO 1

class DoingDancingApp : public App {
    
    u_int8_t        state;
    CaptureLooper*  capture;
    VoiceLooper*    voice;
    
public:
    
    static void prepareSettings( Settings *settings );
    
    void setup() override;
    void keyDown( KeyEvent event ) override;
    void update() override;
    void draw() override;
    void cleanup() override;
    
};




#endif /* DoingDanceingApp_h */
