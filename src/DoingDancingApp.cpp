#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Capture.h"
#include "cinder/qtime/AvfWriter.h"
#include "cinder/qtime/QuickTimeGl.h"
#include "cinder/Log.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class DoingDancingApp : public App {

  public:
	
    static void prepareSettings( Settings *settings );
    
    void setup() override;
	void keyDown( KeyEvent event ) override;
	void update() override;
	void draw() override;
    void cleanup() override { mMovieExporter.reset(); }
    
  private:
    
    Boolean                 recording = false;
    u_int8_t                recording_count = 0;
    fs::path                saveFolder;
    
    CaptureRef              mCapture;
    
    qtime::MovieWriterRef   mMovieExporter;
    qtime::MovieGlRef		mMovie;
    gl::TextureRef			mFrameTexture;
    gl::TextureRef          mTexture;
    
    void printDevices();
    void startRecording();
    void stopRecording();
    void loadMovie(const fs::path &moviePath);
    
};

void DoingDancingApp::prepareSettings( App::Settings *settings )
{
    settings->setWindowSize( 640, 480 );
    settings->setHighDensityDisplayEnabled( false );
    settings->setFullScreen( false );
    settings->setResizable( false );
    
}

void DoingDancingApp::setup() {
    setFrameRate( 30 );
    printDevices();
    
    try {
        mCapture = Capture::create( getWindowWidth(), getWindowHeight() );
        mCapture->start();
    } catch( ci::Exception &exc ) {
        CI_LOG_EXCEPTION( "Failed to init capture ", exc );
        quit();
    }
    
    saveFolder = getFolderPath(); //getSaveFilePath();
    
    if( saveFolder.empty() ) quit();
}

void DoingDancingApp::keyDown( KeyEvent event ) {
    const char key = event.getChar();
    switch (key) {
        case ' ':
            recording = !recording;
            console() << "recording: " << (recording ? "true" : "false") << endl;
            //start recording
            if( recording ) startRecording();
            else {
                stopRecording();
                fs::path path = fs::path(saveFolder);
                path += "/doing_dancing_" + to_string(recording_count) + ".mov";
                loadMovie(path);
            }
            break;
        default:
            break;
    }
}

void DoingDancingApp::update() {
    //console() << "fs: " << getAverageFps() << " / " << getFrameRate() << " " << isFrameRateEnabled() << endl;
    if( mCapture && mCapture->checkNewFrame() ) {
        if( ! mTexture ) {
            // Capture images come back as top-down, and it's more efficient to keep them that way
            mTexture = gl::Texture::create( *mCapture->getSurface(), gl::Texture::Format().loadTopDown() );
        } else {
            mTexture->update( *mCapture->getSurface() );
        }
        
        if( mMovie ) {
            //        if(mMovie->getNumFrames())
            mMovie->stepForward();
            if(mMovie->isDone()) {
                mMovie->seekToStart();
            }
            mFrameTexture = mMovie->getTexture();
        }
        
        if( mMovieExporter && recording ) {
            mMovieExporter->addFrame(  copyWindowSurface()  );
        }
    }
}

void DoingDancingApp::draw() {
    gl::clear( Color( 0, 0, 0 ) );
    Rectf centeredRect;
    if(mFrameTexture) {
        centeredRect = Rectf( mFrameTexture->getBounds() ).getCenteredFit( getWindowBounds(), true );
        gl::color(1.0, 1.0, 1.0, 1.0);
        gl::draw( mFrameTexture, centeredRect );
    }
    if( mTexture ) {
        float alpha = (recording_count > 0) ? 1.0 / float(recording_count) : 1.0;
        centeredRect = Rectf( mTexture->getBounds() ).getCenteredFit( getWindowBounds(), true );
        gl::color(1.0, 1.0, 1.0, alpha);
        gl::draw( mTexture );
    }
    
}

void DoingDancingApp::startRecording() {
    auto format = qtime::MovieWriter::Format()
    .codec( qtime::MovieWriter::H264 )
    .fileType( qtime::MovieWriter::QUICK_TIME_MOVIE )
    .jpegQuality( 0.09f )
    .averageBitsPerSecond( 10000000 );
    //.defaultFrameDuration( 1.0/30.0 );
    recording_count++;
    fs::path path = fs::path(saveFolder);
    path += "/doing_dancing_" + to_string(recording_count) + ".mov";
    console() << "saving to " << path << endl;
    mMovieExporter = qtime::MovieWriter::create( path, getWindowWidth(), getWindowHeight(), format );
}

void DoingDancingApp::stopRecording() {
    mMovieExporter->finish();
    mMovieExporter.reset();
}

void DoingDancingApp::loadMovie( const fs::path &moviePath ) {
    try {
        if(mMovie && mMovie != nullptr) {
            console() << "stop movie" << endl;
            mMovie->stop();
            mMovie.reset();
        }
        console() << "loading movie " << moviePath << endl;
        mMovie = qtime::MovieGl::create( moviePath );
        console() << "framerate: " << mMovie->getFramerate() << endl;
    }
    catch( Exception &exc ) {
        console() << "Exception caught trying to load the movie from path: " << moviePath << ", what: " << exc.what() << endl;
        mMovie.reset();
        
    }
    
    mFrameTexture.reset();
}

void DoingDancingApp::printDevices()
{
    for( const auto &device : Capture::getDevices() ) {
        console() << "Device: " << device->getName() << " " << endl;
    }
}

CINDER_APP( DoingDancingApp, RendererGl )
