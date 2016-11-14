
#ifndef CaptureLooper_hpp
#define CaptureLooper_hpp

#include <stdio.h>

#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/Capture.h"
#include "cinder/qtime/AvfWriter.h"
#include "cinder/qtime/QuickTimeGl.h"

#include "EDSDK.h"
#include "EDSDKTypes.h"
#include "EDSDKErrors.h"

#define CL_NO_CAPTURE 0
#define CL_EDS_CAPTURE 1
#define CL_DEFAULT_CAPTURE 2

using namespace ci;

class CaptureLooper {
    
    const fs::path          saveFolder;
    const u_int32_t         width = 1056;
    const u_int32_t         height = 704;
    const u_int32_t         duration;
    u_int8_t                capture_state = 0;
    
    bool                    recording = false;
    u_int8_t                recording_count = 0;
    u_int32_t               timer = 0;
    
    EdsError                err;
    EdsCameraRef            camera;
    bool                    isSDKLoaded;
    
    CaptureRef              mCapture;
    
    qtime::MovieWriterRef   mMovieExporter;
    qtime::MovieGlRef		mMovie;
    
    gl::TextureRef			mFrameTexture;
    gl::TextureRef          mTexture;
    
public:
    CaptureLooper(fs::path path, const u_int32_t duration = 90);
    ~CaptureLooper();
    
    inline bool isOK() const { return (capture_state != CL_NO_CAPTURE); }
    inline bool isRecording() const { return recording; }
    
    void update(const Surface&);
    void draw (const Area&) const;
    void start();
    
private:
    
    void stop();
    
    fs::path getVideoRecordingPath (int);
    
    inline void loadMovie(const fs::path &moviePath);
    
    void setupDefaultCapture();
    
    EdsError setupEdsCamera();
    EdsError getFirstCamera(EdsCameraRef *camera);
    
    static EdsError EDSCALLBACK handlePropertyEvent( EdsPropertyEvent, EdsPropertyID, EdsUInt32, EdsVoid *);
    static EdsError EDSCALLBACK handleObjectEvent( EdsObjectEvent event, EdsBaseRef object,EdsVoid * context);
    
    EdsError keepAlive();
    EdsError startLiveview();
    EdsError endLiveview();
    EdsError downloadEvfData();
    
};

#endif /* CaptureLooper_hpp */
