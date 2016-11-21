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
    settings->setFullScreen( false );
    settings->setResizable( false );
    
}

void DoingDancingApp::setup() {
    
    int seconds = 2;
    fs::path path = getFolderPath(); //getSaveFilePath();
    if( path.empty() ) quit();
    
    capture = new CaptureLooper(path, 30*seconds);
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
    
    //pass across reference to window surface add to recording
    if(capture->isRecording()) capture->update(copyWindowSurface());
    voice->update();
    
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
    
}

void DoingDancingApp::draw() {
    gl::clear( Color( 0, 0, 0 ) );
    if(capture->isRecording()) capture->draw( getWindowBounds() );
}

CINDER_APP( DoingDancingApp, RendererGl, DoingDancingApp::prepareSettings )
