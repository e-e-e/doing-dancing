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
    //settings->setFullScreen( true );
    settings->setResizable( false );
    
}

void DoingDancingApp::setup() {
    int seconds = 180;
    fs::path path = getFolderPath(); //getSaveFilePath();
    if( path.empty() ) quit();
    
    capture = new CaptureLooper(getWindowBounds(), path, seconds);
    if(!capture->isOK()) {
        console() << "THERE WAS A PROBLEM STARTING VIDEO CAPTURE!" << endl;
        quit();
        return;
    }
    
    voice = new VoiceLooper(seconds);
//    state = DOING_DANCING_VOICE;
//    voice->start();
    change();
}

void DoingDancingApp::cleanup () {
    if( capture )   delete capture;
    if( voice )   delete voice;
}

void DoingDancingApp::keyDown( KeyEvent event ) {
    const char key = event.getChar();
    switch (key) {
        case ' ':
            change();
            break;
        default:
            break;
    }
}

void DoingDancingApp::update() {
    
    if( state == DOING_DANCING_VOICE && voice->hasBeenAMoment() ) {
        capture->preload();
    }
    
    change();
    
    capture->update();
    voice->update();
}

void DoingDancingApp::draw() {
    gl::clear( Color( 0, 0, 0 ) );
    capture->draw();
}

void DoingDancingApp::change() {
    if( state == DOING_DANCING_INIT ) {
        voice->start();
        state = DOING_DANCING_VOICE;
        timer.start();
    } else if ( voice->isStopped() && !capture->isRecording()) {
        cout << "Framerate = " << getAverageFps() << endl;
        cout << "TimeElapsed = " << timer.getSeconds() << endl;
        timer.start();
        if(state == DOING_DANCING_VOICE) {
            capture->start();
            state = DOING_DANCING_VIDEO;
        } else {
            voice->start();
            state = DOING_DANCING_VOICE;
        }
    }

}

RendererGl::Options options() {
    RendererGl::Options opts;
    opts.msaa(0);
    return opts;
}

CINDER_APP( DoingDancingApp, RendererGl(options()) , DoingDancingApp::prepareSettings )
