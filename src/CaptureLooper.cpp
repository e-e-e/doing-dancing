
#include "CaptureLooper.hpp"

#include "cinder/Log.h"

using namespace std;

void printChannelOrder (SurfaceRef ref) {
    switch ((ref->getChannelOrder()).getCode()) {
        case SurfaceChannelOrder::RGBA:
            cout<< "RGBA";
            break;
        case SurfaceChannelOrder::BGRA:
            cout<< "BGRA";
            break;
        case SurfaceChannelOrder::ARGB:
            cout<< "ARGB";
            break;
        case SurfaceChannelOrder::ABGR:
            cout<< "ABGR";
            break;
        case SurfaceChannelOrder::RGBX:
            cout<< "RGBX";
            break;
        case SurfaceChannelOrder::BGRX:
            cout<< "BGRX";
            break;
        case SurfaceChannelOrder::XRGB:
            cout<< "XRGB";
            break;
        case SurfaceChannelOrder::XBGR:
            cout<< "XBGR";
            break;
        case SurfaceChannelOrder::RGB:
            cout<< "RGB";
            break;
        case SurfaceChannelOrder::BGR:
            cout<< "BGR";
            break;
        case SurfaceChannelOrder::UNSPECIFIED:
            cout<< "UNSPECIFIED";
            break;
        default:
            cout<<'!';
            break;
    }
    cout << endl;
}

CaptureLooper::CaptureLooper(const Area& windowBounds, fs::path path, const u_int32_t duration, u_int32_t framerate)
    : framerate(framerate), windowBounds(windowBounds),
      saveFolder(path), duration(duration*framerate),
      framerateInSeconds(1.0 / (double) framerate )
{
    EdsError err = setupEdsCamera();
    if( err != EDS_ERR_OK ) {
        setupDefaultCapture();
    }
    cout << framerateInSeconds << ' ' << framerate << endl;
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
    if(mMovieExporter) {
        mMovieExporter->finish();
        mMovieExporter.reset();
    }
    //
}

void CaptureLooper::update() {
    if(!recording && captureReady()) {
        if( capture_state == CL_DEFAULT_CAPTURE ) {
            if( ! mTexture )
                mTexture = gl::Texture::create( *mCapture->getSurface(), gl::Texture::Format().loadTopDown() );
            else
                mTexture->update( *mCapture->getSurface() );
            
        } else {
            downloadEvfData();
            if( newFrame && mLivePixels ) {
                if (!mTexture)
                    mTexture = gl::Texture::create( *mLivePixels, gl::Texture::Format().loadTopDown() );
                else
                    mTexture->update( *mLivePixels );
            }
        }
    } else if ( recording && recordingReady() && captureReady() ) {
        cv::Mat capture, movie, output;
        SurfaceRef toTexture;
        
        if( capture_state == CL_DEFAULT_CAPTURE ) {
            capture = toOcvRef( *mCapture->getSurface() );
        } else {
            downloadEvfData();
            if(newFrame) capture = toOcvRef( *mLivePixels );
        }
        
        if( !capture.empty() ) {
        
            if( mMovie ) {
                
                movie = toOcvRef(*mMovie->getSurface());

                float alpha = (recording_count > 0) ? 1.0 / float(recording_count) : 1.0;

                //cv::cvtColor(movie, movie, cv::COLOR_BGRA2BGR);
    //                        cout << "Width : " << movie.cols << endl;
    //                        cout << "Height: " << movie.rows << endl;
    //                        cout << "channels: " << movie.channels() << endl;
    //                        cout << "Width : " << capture.cols << endl;
    //                        cout << "Height: " << capture.rows << endl;
    //                        cout << "channels: " << capture.channels() << endl;
                //            printChannelOrder(mMovie->getSurface());
                cv::addWeighted(capture,alpha,movie,1.0-alpha, 0, output);
                mMovie->stepForward();
            }
            
            toTexture = Surface::create( fromOcv((output.empty())? capture : output ));
            
            if( mMovieExporter && recording) {
                mMovieExporter->addFrame(*toTexture);
            }
            
            if( capture_state == CL_EDS_CAPTURE )
                toTexture->setChannelOrder(SurfaceChannelOrder(SurfaceChannelOrder::BGRA));
            if (!mTexture)
                mTexture = gl::Texture::create( *toTexture, gl::Texture::Format().loadTopDown() );
            else
                mTexture->update( *toTexture );
            
            
            if(recording) {
                frameCount++;
                if( frameCount >= duration ) stop();
            }
        }
    
    }
}

void CaptureLooper::draw() const {
    if( mTexture && recording) {
        Rectf centeredRect = Rectf( mTexture->getBounds() ).getCenteredFit( windowBounds, true );
        gl::color(1.0, 1.0, 1.0);
        gl::draw( mTexture, centeredRect );
    }
}

void CaptureLooper::preload() {
    if(recording_count>0 && !preloaded) {
        fs::path movie_path = getVideoRecordingPath(recording_count);
        loadMovie(movie_path);
        preloaded = true;
    }
}

void CaptureLooper::start() {
   
    if( capture_state == CL_EDS_CAPTURE )
        keepAlive();
    
    timer.start();
    recording = true;
    frameCount = 0;
    recording_count++;
    
    fs::path path = getVideoRecordingPath(recording_count);
    cout << "Starting Video Recording: " << path << endl;
    
    if(mCapture) {
        ivec2 size = mCapture->getSurface()->getSize();
        width = size.x;
        height = size.y;
    }
    
    auto format = qtime::MovieWriter::Format()
        .codec( qtime::MovieWriter::H264 )
        .fileType( qtime::MovieWriter::QUICK_TIME_MOVIE )
        .jpegQuality( 1.0f )
        .averageBitsPerSecond(framerateInSeconds);
        //.averageBitsPerSecond( 1000000 );
    
    mMovieExporter = qtime::MovieWriter::create( path, width, height, format );

}

void CaptureLooper::stop() {
    cout << "Stopped video recording!" << endl;
    if(mMovie && mMovie != nullptr) {
        mMovie->stop();
        mMovie.reset();
    }
    timer.stop();
    mMovieExporter->finish();
    mMovieExporter.reset();
    frameCount = 0;
    recording = false;
    preloaded = false;
    
}

bool CaptureLooper::nextFrame() {
    double seconds = timer.getSeconds();
    if( seconds >= framerateInSeconds ) {
        timer.start(seconds - framerateInSeconds);
        return true;
    }
    return false;
}

fs::path CaptureLooper::getVideoRecordingPath (int i) const {
    fs::path path = fs::path(saveFolder);
    path += "/doing_dancing_" + to_string(i) + ".mov";
    return path;
}



void CaptureLooper::loadMovie( const fs::path &moviePath ) {
    try {
        cout << "loading movie " << moviePath << endl;
        mMovie = qtime::MovieSurface::create( moviePath );
        mMovie->stop();
        mMovie->seekToStart();
    } catch( Exception &exc ) {
        cout << "Exception caught trying to load the movie from path: "
            << moviePath << ", what: " << exc.what()
            << endl;
        mMovie.reset();
    }
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
    if( nextFrame() ) {
        newFrame = true;
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
                mLivePixels = Surface::create(loadImage( DataSourceBuffer::create(buffer), ImageSource::Options(), "jpg" ), SurfaceConstraints(), true);
    //            mLivePixels->setChannelOrder(SurfaceChannelOrder(SurfaceChannelOrder::BGRA));
    //            mTexture = gl::Texture::create( loadImage( DataSourceBuffer::create(buffer), ImageSource::Options(), "jpg" ));
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
    } else {
        newFrame = false;
    }
    return err;
}
