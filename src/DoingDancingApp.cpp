#include "DoingDancingApp.h"

#include "cinder/app/RendererGl.h"
#include "cinder/Log.h"

using namespace ci;
using namespace ci::app;
using namespace std;

void DoingDancingApp::prepareSettings( App::Settings *settings )
{
    settings->setWindowSize( 1056,704 );
    settings->setFrameRate( 60 );
    settings->setHighDensityDisplayEnabled( false );
    settings->setFullScreen( false );
    settings->setResizable( false );
    
}

void DoingDancingApp::setup() {
    int seconds = 5;
    fs::path path = getFolderPath(); //getSaveFilePath();
    if( path.empty() ) quit();
    
    capture = new CaptureLooper(getWindowBounds(), path, 25*seconds);
    if(!capture->isOK()) {
        console() << "THERE WAS A PROBLEM STARTING VIDEO CAPTURE!" << endl;
        quit();
        return;
    }
    
    voice = new VoiceLooper(seconds);
//    state = DOING_DANCING_VOICE;
//    voice->start();
    
}

void DoingDancingApp::cleanup () {
    if( capture )   delete capture;
    if( voice )   delete voice;
}

void DoingDancingApp::keyDown( KeyEvent event ) {
    const char key = event.getChar();
    switch (key) {
        case ' ':
            if( state == DOING_DANCING_INIT ) {
                voice->start();
                state = DOING_DANCING_VOICE;
            } else if ( voice->isStopped() && !capture->isRecording()) {
                cout << getAverageFps() << endl;
                if(state == DOING_DANCING_VOICE) {
                    capture->start();
                    state = DOING_DANCING_VIDEO;
                } else {
                    voice->start();
                    state = DOING_DANCING_VOICE;
                }
            }
            break;
        default:
            break;
    }
}

void DoingDancingApp::update() {
    
    if( state == DOING_DANCING_VOICE && voice->hasBeenAMoment() ) {
        capture->preload();
    }
    
    capture->update();
    voice->update();
}

void DoingDancingApp::draw() {
    gl::clear( Color( 0, 0, 0 ) );
    if(capture->isRecording()) capture->draw();
}

RendererGl::Options options() {
    RendererGl::Options opts;
    opts.msaa(0);
    return opts;
}

CINDER_APP( DoingDancingApp, RendererGl(options()) , DoingDancingApp::prepareSettings )
