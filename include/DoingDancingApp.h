#pragma once

#ifndef DoingDanceingApp_h
#define DoingDanceingApp_h

#include "cinder/app/App.h"

#include "CaptureLooper.hpp"

using namespace ci;
using namespace ci::app;
using namespace std;

class DoingDancingApp : public App {
    
    CaptureLooper*          capture;
    
public:
    
    static void prepareSettings( Settings *settings );
    
    void setup() override;
    void keyDown( KeyEvent event ) override;
    void update() override;
    void draw() override;
    void cleanup() override;
    
};




#endif /* DoingDanceingApp_h */
