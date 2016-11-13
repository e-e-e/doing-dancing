#pragma once

#ifndef DoingDanceingApp_h
#define DoingDanceingApp_h

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "cinder/Capture.h"
#include "cinder/qtime/AvfWriter.h"
#include "cinder/qtime/QuickTimeGl.h"

#include "EDSDK.h"
#include "EDSDKTypes.h"
#include "EDSDKErrors.h"

#define EOS_USE true
#define RECORDING_TIME 90
#define OFF_TIME 30


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
    void cleanup() override;
    
private:
    
    Boolean                 recording = false;
    u_int8_t                recording_count = 0;
    u_int32_t               recording_timer = 0;
    fs::path                saveFolder;
    
#if EOS_USE == true
    EdsError err;
    EdsCameraRef camera;
    bool isSDKLoaded;
#else
    CaptureRef              mCapture;
#endif
    
    qtime::MovieWriterRef   mMovieExporter;
    qtime::MovieGlRef		mMovie;
    
    gl::TextureRef			mFrameTexture;
    gl::TextureRef          mTexture;

    void printDevices();
    
    void startVideoRecording();
    void updateVideoRecording();
    void stopVideoRecording();
    void loadMovie(const fs::path &moviePath);
    
    void startSoundRecording();
    
    fs::path getVideoRecordingPath (int);
    
#if EOS_USE == true
    
    EdsError getFirstCamera(EdsCameraRef *camera);
    
    static EdsError EDSCALLBACK handlePropertyEvent( EdsPropertyEvent, EdsPropertyID, EdsUInt32, EdsVoid *);
    static EdsError EDSCALLBACK handleObjectEvent( EdsObjectEvent event, EdsBaseRef object,EdsVoid * context);
    
    EdsError startLiveview();
    EdsError endLiveview();
    EdsError downloadEvfData();
    
#endif
    
};




#endif /* DoingDanceingApp_h */
