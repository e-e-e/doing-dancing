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
    
    fs::path path = getFolderPath(); //getSaveFilePath();
    if( path.empty() ) quit();
    
    capture = new CaptureLooper(path);
    
}

void DoingDancingApp::cleanup () {
    if( capture )
        delete capture;
}

void DoingDancingApp::keyDown( KeyEvent event ) {
    const char key = event.getChar();
    switch (key) {
        case ' ':
            break;
        default:
            break;
    }
}

void DoingDancingApp::update() {
    //pass across reference to window surface add to recording
    capture->update(copyWindowSurface());
    
}

void DoingDancingApp::draw() {
    gl::clear( Color( 0, 0, 0 ) );
    capture->draw( getWindowBounds() );
}

CINDER_APP( DoingDancingApp, RendererGl, DoingDancingApp::prepareSettings )
