#pragma once

#ifndef DoingDanceingApp_h
#define DoingDanceingApp_h

#include "cinder/app/App.h"

#include "CaptureLooper.hpp"
#include "VoiceLooper.hpp"

//#define DOING_DANCING_AUTOMATED
#define DOING_DANCING_INIT  0
#define DOING_DANCING_VOICE 1
#define DOING_DANCING_VIDEO 2

using namespace ci;
using namespace ci::app;
using namespace std;

class DoingDancingApp : public App {
    
    u_int8_t        state = DOING_DANCING_INIT;
    CaptureLooper*  capture;
    VoiceLooper*    voice;
    
    Timer           timer;
    
public:
    
    static void prepareSettings( Settings *settings );
    
    void setup() override;
    void keyDown( KeyEvent event ) override;
    void update() override;
    void draw() override;
    void cleanup() override;
    
private:
    
    inline void change();
    
};




#endif /* DoingDanceingApp_h */
