#include "DoingDancingApp.h"

#include "cinder/Log.h"


using namespace ci;
using namespace ci::app;
using namespace std;

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
    
#if EOS_USE == true
    err = EDS_ERR_OK;
    camera = NULL;
    isSDKLoaded = false;
    
    // Initialize SDK
    err = EdsInitializeSDK();
    if(err == EDS_ERR_OK) {
        isSDKLoaded = true;
    }
    // Get first camera
    if(err == EDS_ERR_OK) {
        err = getFirstCamera (&camera);
    }
    // Set event handler
    if(err == EDS_ERR_OK) {
        err = EdsSetObjectEventHandler(camera, kEdsObjectEvent_All, handleObjectEvent, NULL);
    }
    // Set event handler
    if(err == EDS_ERR_OK) {
        err = EdsSetPropertyEventHandler(camera, kEdsPropertyEvent_All, handlePropertyEvent, NULL);
    }
    // Open session with camera
    
    if(err == EDS_ERR_OK) {
        err = EdsOpenSession(camera);
    }
    
    if(err == EDS_ERR_OK)
        printf("EOS OPENED\n");
    else
        printf("EOS FAILED\n");

    startLiveview();
#else
    try {
        mCapture = Capture::create( getWindowWidth(), getWindowHeight() );
        mCapture->start();
    } catch( ci::Exception &exc ) {
        CI_LOG_EXCEPTION( "Failed to init capture ", exc );
        quit();
    }
#endif
    saveFolder = getFolderPath(); //getSaveFilePath();
    
    if( saveFolder.empty() ) quit();
}

void DoingDancingApp::cleanup () {
    
#if EOS_USE == true
    
    endLiveview();
    if(err == EDS_ERR_OK) {
        err = EdsCloseSession(camera);
    }
    // Release camera
    if(camera != NULL) {
        printf("Camera Released\n");
        EdsRelease(camera);
    }
    // Terminate SDK
    if(isSDKLoaded) {
        printf("EOS CLOSED\n");
        EdsTerminateSDK();
    }
    
#endif
    
    if(mMovieExporter) mMovieExporter->finish();
    mMovieExporter.reset();
    
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
#if EOS_USE == false
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
#else 
    downloadEvfData();
#endif
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

#if EOS_USE == true

EdsError DoingDancingApp::getFirstCamera(EdsCameraRef *camera) {
    
    EdsError err = EDS_ERR_OK;
    EdsCameraListRef cameraList = NULL;
    EdsUInt32 count = 0;
    // Get camera list
    err = EdsGetCameraList(&cameraList);
    // Get number of cameras
    if(err == EDS_ERR_OK) {
        err = EdsGetChildCount(cameraList, &count);
        if(count == 0) {
            err = EDS_ERR_DEVICE_NOT_FOUND;
        }
    }
    
    // Get first camera retrieved
    if(err == EDS_ERR_OK) {
        err = EdsGetChildAtIndex(cameraList,0,camera);
    }
    
    // Release camera list
    if(cameraList != NULL) {
        EdsRelease(cameraList);
        cameraList = NULL;
    }
    
    return err;
}


EdsError EDSCALLBACK DoingDancingApp::handleObjectEvent( EdsObjectEvent event, EdsBaseRef object,EdsVoid * context)
{
    // do something
    /* switch(event)
     EdsVoid * context)
     {
     case kEdsObjectEvent_DirItemRequestTransfer:
     downloadImage(object); break;
     default:
     break;
     } */
    // Object must be released
    printf("OBJECT EVENT\n");
    if(object) {
        EdsRelease(object);
    }
    return EDS_ERR_OK;
}


EdsError EDSCALLBACK DoingDancingApp::handlePropertyEvent( EdsPropertyEvent inEvent, EdsPropertyID inPropertyID,
                                                             EdsUInt32 inParam, EdsVoid * inContext)
{
    printf("PROPERTY EVENT\n");
    return EDS_ERR_OK;
}

EdsError DoingDancingApp::startLiveview() {
    EdsError err = EDS_ERR_OK;
    // Get the output device for the live view image
    EdsUInt32 device;
    err = EdsGetPropertyData(camera, kEdsPropID_Evf_OutputDevice, 0 , sizeof(device), &device );
    // PC live view starts by setting the PC as the output device for the live view image.
    if(err == EDS_ERR_OK) {
        device |= kEdsEvfOutputDevice_PC;
        err = EdsSetPropertyData(camera, kEdsPropID_Evf_OutputDevice, 0 , sizeof(device), &device);
    }
    // A property change event notification is issued from the camera if property settings are made successfully.
    // Start downloading of the live view image once the property change notification arrives.
    return err;
}

EdsError DoingDancingApp::endLiveview() {
    EdsError err = EDS_ERR_OK;
    // Get the output device for the live view image
    EdsUInt32 device;
    err = EdsGetPropertyData(camera, kEdsPropID_Evf_OutputDevice, 0, sizeof(device), &device );
    // PC live view ends if the PC is disconnected from the live view image output device.
    if(err == EDS_ERR_OK)
    {
        device &= ~kEdsEvfOutputDevice_PC;
        err = EdsSetPropertyData(camera, kEdsPropID_Evf_OutputDevice, 0 , sizeof(device), &device);
    }
    return err;
}

EdsError DoingDancingApp::downloadEvfData() {
    EdsError err = EDS_ERR_OK;
    EdsStreamRef stream = NULL;
    EdsEvfImageRef evfImage = NULL;
    // Create memory stream.
    err = EdsCreateMemoryStream( 0, &stream);
    // Create EvfImageRef.
    if(err == EDS_ERR_OK) {
        err = EdsCreateEvfImageRef(stream, &evfImage);
    }
    // Download live view image data.
    if(err == EDS_ERR_OK) {
        err = EdsDownloadEvfImage(camera, evfImage);
    }
    // Get the incidental data of the image.
    if(err == EDS_ERR_OK) {
        // Get the zoom ratio
        EdsUInt32 zoom;
        EdsGetPropertyData(evfImage, kEdsPropID_Evf_ZoomPosition, 0 , sizeof(zoom), &zoom);
        // Get the focus and zoom border position
        EdsPoint point;
        EdsGetPropertyData(evfImage, kEdsPropID_Evf_ZoomPosition, 0 , sizeof(point), &point);
        
        EdsUInt32 length;
        EdsGetLength(stream, &length);
        
        if( length > 0 ){
            
            unsigned char * ImageData;
            EdsUInt32 DataSize = length;
            
            EdsGetPointer(stream,(EdsVoid**)&ImageData);
            EdsGetLength(stream, &DataSize);
            
            BufferRef buffer = Buffer::create(ImageData,DataSize);
            mTexture = gl::Texture::create( loadImage( DataSourceBuffer::create(buffer), ImageSource::Options(), "jpg" ), gl::Texture::Format().loadTopDown() );
            printf("%i,%i\n",mTexture->getWidth(), mTexture->getHeight());
            
        }
    }
    
    // Release stream
    if(stream != NULL) {
        EdsRelease(stream);
        stream = NULL;
    }
    // Release evfImage
    if(evfImage != NULL) {
        EdsRelease(evfImage);
        evfImage = NULL;
    }
    return err;
}

#endif



CINDER_APP( DoingDancingApp, RendererGl )
