#include "DoingDancingApp.h"

#include "cinder/app/RendererGl.h"
#include "cinder/Log.h"

using namespace ci;
using namespace ci::app;
using namespace std;

void DoingDancingApp::prepareSettings( App::Settings *settings )
{
    settings->setWindowSize( 1056,704 );
    settings->setFrameRate( 30 );
    settings->setHighDensityDisplayEnabled( false );
    settings->setFullScreen( true );
    settings->setResizable( false );
    
}

void DoingDancingApp::setup() {
    int seconds = 5;
    fs::path path = getFolderPath(); //getSaveFilePath();
    if( path.empty() ) quit();
    
    capture = new CaptureLooper(getWindowBounds(), path, getFrameRate()*seconds);
    if(!capture->isOK()) {
        console() << "THERE WAS A PROBLEM STARTING VIDEO CAPTURE!" << endl;
        quit();
        return;
    }
    
    voice = new VoiceLooper(seconds);
    voice->start();
    state = DOING_DANCING_VOICE;
}

void DoingDancingApp::cleanup () {
    if( capture )   delete capture;
}

void DoingDancingApp::keyDown( KeyEvent event ) {
    const char key = event.getChar();
    switch (key) {
        case ' ':
//            if( !capture->isRecording() ) capture->start();
            break;
        default:
            break;
    }
}

void DoingDancingApp::update() {
    
    if( voice->isStopped() && !capture->isRecording()) {
        cout << getAverageFps() << endl;
        if(state == DOING_DANCING_VOICE) {
            capture->start();
            state = DOING_DANCING_VIDEO;
        } else {
            voice->start();
            state = DOING_DANCING_VOICE;
        }
    }
    
    if( state == DOING_DANCING_VOICE && voice->hasBeenAMoment() ) {
        capture->preload();
    }
    
    if( capture->isRecording() ) {
        //pass across reference to window surface add to recording
        Area copy = capture->drawingBounds();
        capture->update(copyWindowSurface(copy));
    } else {
        capture->update();
    }
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
