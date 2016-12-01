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
    settings->setResizable( true );
    settings->setTitle("Doing Dancing");
    
}

void DoingDancingApp::setup() {
    int seconds = 8*60;
    fs::path path = getFolderPath(); //getSaveFilePath();
    if( path.empty() ) quit();
    
    capture = new CaptureLooper(path, seconds);
    if(!capture->isOK()) {
        console() << "THERE WAS A PROBLEM STARTING VIDEO CAPTURE!" << endl;
        quit();
        return;
    }
    
    voice = new VoiceLooper(path, seconds);
#ifdef DOING_DANCING_AUTOMATED
    change();
#endif
}

void DoingDancingApp::cleanup () {
    if( capture != nullptr )   delete capture;
    if( voice != nullptr )   delete voice;
}

void DoingDancingApp::keyDown( KeyEvent event ) {
    const char key = event.getChar();
    const bool fs = isFullScreen();
    switch (key) {
        case 'f':
        case 'F':
            setFullScreen(!fs);
            if(!fs) hideCursor();
            else showCursor();
            break;
        case ' ':
#ifndef DOING_DANCING_AUTOMATED
            change();
#endif
            break;
        default:
            break;
    }
}

void DoingDancingApp::update() {
    
    if( ((int)floor(timer.getSeconds()) % 60) == 0 ) capture->keepAlive();
    
    if( state == DOING_DANCING_VOICE && voice->hasBeenAMoment() ) {
        capture->preload();
    }
    
#ifdef DOING_DANCING_AUTOMATED
    change();
#endif
    
    capture->update();
    voice->update();
}

void DoingDancingApp::draw() {
    gl::clear( Color( 0, 0, 0 ) );
    gl::color(1,1,1);
    capture->draw(getWindowBounds());
    if(!voice->isStopped()) {
        gl::color(1, 0, 0);
        gl::drawSolidCircle(vec2(20,20), 10.0);
    }
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
//
//RendererGl::Options optionsGL() {
//    RendererGl::Options opts;
//    opts.msaa(0);
//    return opts;
//}

CINDER_APP( DoingDancingApp, RendererGl , DoingDancingApp::prepareSettings )
