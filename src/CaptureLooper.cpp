
#include "CaptureLooper.hpp"

#include "cinder/Log.h"
#include "cinder/ip/Resize.h"

using namespace std;

CaptureLooper::CaptureLooper(fs::path path, const u_int32_t duration )
    : saveFolder(path), duration(duration)
{
    EdsError err = setupEdsCamera();
    if( err != EDS_ERR_OK ) {
        setupDefaultCapture();
    }
}

CaptureLooper::~CaptureLooper() {
    if( capture_state == CL_EDS_CAPTURE) {
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
        if( isSDKLoaded ) {
            printf("EOS CLOSED\n");
            EdsTerminateSDK();
        }
    }
    if(mMovieExporter) mMovieExporter->finish();
    mMovieExporter.reset();
}

void CaptureLooper::update(const Surface& surface) {
    
    if(!mMovie || (mMovie->isPlayable() || mMovie->checkPlaythroughOk())) {
    
        if( capture_state == CL_EDS_CAPTURE ||
           (capture_state == CL_DEFAULT_CAPTURE && mCapture && mCapture->checkNewFrame()))
        {
            // update capture texture
            if( capture_state == CL_DEFAULT_CAPTURE ) {
                
                mLivePixels = mCapture->getSurface();
                if( ! mTexture )
                    mTexture = gl::Texture::create( *mLivePixels, gl::Texture::Format().loadTopDown() );
                else
                    mTexture->update( *mLivePixels );

                // Capture images come back as top-down, and it's more efficient to keep them that way
                
            } else {
                downloadEvfData();
            }
            
            if( mMovie ) {
                
                mFrameTexture = mMovie->getTexture();

//                if( ! mFrameTexture )
//                    mFrameTexture = gl::Texture::create( *mMoviePixels, gl::Texture::Format().loadTopDown() );
//                else
//                    mFrameTexture->update( *mMoviePixels );
               
//                cout << "MOVIE-" << mMoviePixels->getSize() << endl;
//                cout << "LIVE-" << mLivePixels->getSize() << endl;
//                Surface::Iter inputIter( mMoviePixels->getIter() );
//                Surface::Iter outputIter( mLivePixels->getIter() );
//                double alpha = (recording_count > 0) ? 1.0 / double(recording_count) : 1.0;
//                double inverse_alpha = 1.0 - alpha;
//                while( inputIter.line() ) {
//                    outputIter.line();
//                    while( inputIter.pixel() ) {
//                        outputIter.pixel();
//                        outputIter.r() = constrain<int32_t>(  (double(inputIter.r()) * inverse_alpha) + (double(outputIter.r()) * (alpha)), 0, 255 );
//                        outputIter.b() = constrain<int32_t>( (double(inputIter.b()) * inverse_alpha) + (double(outputIter.b()) * (alpha)), 0, 255 );
//                        outputIter.g() = constrain<int32_t>( (double(inputIter.g()) * inverse_alpha) + (double(outputIter.g()) * (alpha)), 0, 255 );
//                    }
//                }
                mMovie->stepForward();
                if(mMovie->isDone()) {
                    mMovie->seekToStart();
                }
            }
            
//            if( ! mTexture )
//                mTexture = gl::Texture::create( *mLivePixels, gl::Texture::Format().loadTopDown() );
//            else
//                mTexture->update( *mLivePixels );
            
            if( mMovieExporter && recording ) {
                
                mMovieExporter->addFrame( ip::resizeCopy (surface, surface.getBounds(), mMovieExporter->getSize()) );
            }
        
            if(recording) {
                timer++;
                if( timer >= duration ) stop();
            }
        
        }
    } else {
        if(mMovie) {
            if(mMovie->isDone())
                cout << "MOVIE IS DONE" << timer << '/'<<duration << mMovie->getNumFrames() << endl;
            else {
                cout << "CT:" << mMovie->getCurrentTime()
                << " DUR: " << mMovie->getDuration() << endl;
            }
        }
    }
}

void CaptureLooper::draw(const Area& windowBounds) const {
    Rectf centeredRect;
    if(mFrameTexture) {
        centeredRect = Rectf( mFrameTexture->getBounds() ).getCenteredFit( windowBounds, true );
        gl::color(1.0, 1.0, 1.0, 1.0);
        gl::draw( mFrameTexture, centeredRect );
    }
    if( mTexture ) {
        float alpha = (recording_count > 0) ? 1.0 / float(recording_count) : 1.0;
        centeredRect = Rectf( mTexture->getBounds() ).getCenteredFit( windowBounds, true );
        gl::color(1.0, 1.0, 1.0, alpha);
        gl::draw( mTexture, centeredRect );
    }
}

void CaptureLooper::start() {
   
    if( capture_state == CL_EDS_CAPTURE )
        keepAlive();
    
    if(recording_count>0) {
        fs::path movie_path = getVideoRecordingPath(recording_count);
        loadMovie(movie_path);
    }
    
    recording = true;
    timer = 0;
    recording_count++;
    
    fs::path path = getVideoRecordingPath(recording_count);
    cout << "saving to " << path << endl;

    if(mCapture) {
        ivec2 size = mCapture->getSurface()->getSize();
        width = size.x;
        height = size.y;
    }
    
    auto format = qtime::MovieWriter::Format()
        .codec( qtime::MovieWriter::H264 )
        .fileType( qtime::MovieWriter::QUICK_TIME_MOVIE )
        .jpegQuality( 0.09f )
        .averageBitsPerSecond( 10000000 );
    
    
    mMovieExporter = qtime::MovieWriter::create( path, width, height, format );

}

void CaptureLooper::stop() {
    if(mMovie && mMovie != nullptr) {
        mMovie->stop();
        mMovie.reset();
    }
    mMovieExporter->finish();
    mMovieExporter.reset();
    timer = 0;
    recording = false;
}

fs::path CaptureLooper::getVideoRecordingPath (int i) {
    fs::path path = fs::path(saveFolder);
    path += "/doing_dancing_" + to_string(i) + ".mov";
    return path;
}



void CaptureLooper::loadMovie( const fs::path &moviePath ) {
    try {
        cout << "loading movie " << moviePath << endl;
        mMovie = qtime::MovieGl::create( moviePath );
        cout  << "framerate: " << mMovie->getFramerate() << endl;
    }
    catch( Exception &exc ) {
        cout << "Exception caught trying to load the movie from path: " << moviePath << ", what: " << exc.what() << endl;
        mMovie.reset();
    }
    mFrameTexture.reset();
}

void CaptureLooper::setupDefaultCapture() {
    try {
        mCapture = Capture::create( width, height );
        mCapture->start();
        capture_state = CL_DEFAULT_CAPTURE;
    } catch( ci::Exception &exc ) {
        CI_LOG_EXCEPTION( "Failed to init capture ", exc );
        cout << "PROBLEM " << exc.what() << endl;
        capture_state = CL_NO_CAPTURE;
    }
}

EdsError CaptureLooper::setupEdsCamera() {
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
    
    if(err == EDS_ERR_OK) {
        printf("EOS OPENED\n");
        startLiveview();
        capture_state = CL_EDS_CAPTURE;
    } else {
        printf("EOS FAILED\n");
    }
    
    return err;
}

EdsError CaptureLooper::getFirstCamera(EdsCameraRef *camera) {
    
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


EdsError EDSCALLBACK CaptureLooper::handleObjectEvent( EdsObjectEvent event, EdsBaseRef object,EdsVoid * context)
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


EdsError EDSCALLBACK CaptureLooper::handlePropertyEvent( EdsPropertyEvent inEvent, EdsPropertyID inPropertyID,
                                                          EdsUInt32 inParam, EdsVoid * inContext)
{
    printf("PROPERTY EVENT\n");
    return EDS_ERR_OK;
}

EdsError CaptureLooper::startLiveview() {
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

EdsError CaptureLooper::endLiveview() {
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

EdsError CaptureLooper::keepAlive() {
    EdsError err = EDS_ERR_OK;
    EdsUInt32 inParam = 0;
    err = EdsSendCommand(camera, kEdsCameraCommand_ExtendShutDownTimer, inParam);
    if( err != EDS_ERR_OK ) {
        cout << "THERE WAS A PROBLEM KEEPING ALIVE! " << err << endl;
    }
    return err;
}

EdsError CaptureLooper::downloadEvfData() {
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
        
        EdsUInt64 length;
        EdsGetLength(stream, &length);
        
        if( length > 0 ){
            
            unsigned char * ImageData;
            EdsUInt64 DataSize = length;
            
            EdsGetPointer(stream,(EdsVoid**)&ImageData);
            EdsGetLength(stream, &DataSize);
            
            BufferRef buffer = Buffer::create(ImageData,DataSize);
            mLivePixels = Surface::create( loadImage( DataSourceBuffer::create(buffer), ImageSource::Options(), "jpg" ));
//            printf("%i,%i\n",mTexture->getWidth(), mTexture->getHeight());
            
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
