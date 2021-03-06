//
//  VoiceLooper.cpp
//  AudioExperiment
//
//  Created by Benjamin Forster on 11/11/2016.
//
//

#include "VoiceLooper.hpp"

VoiceLooper::VoiceLooper ()
    : saveAudio(false), duration(71)
{
    setup();
}

VoiceLooper::VoiceLooper (fs::path path)
    : saveFolder(path), saveAudio(true), duration(71)
{
    setup();
}

VoiceLooper::VoiceLooper (fs::path path, int d)
    : saveFolder(path), saveAudio(true), duration(d)
{
    setup();
}

VoiceLooper::~VoiceLooper() {
    stop();
}

void VoiceLooper::setup() {
    auto ctx = audio::Context::master();
    
    //    auto anout = ci::audio::Device::findDeviceByName( "Internal Speakers" );
    //    console() << anout->getName() << endl;
    //    ci::audio::OutputDeviceNodeRef output = ctx->createOutputDeviceNode( anout );
    //    ctx->setOutput( output );
    
    //    audio::DeviceRef dev = dynamic_pointer_cast<audio::OutputDeviceNode>( ctx->getOutput() )->getDevice();
    //
    //    console() << "input: " << audio::Device::getDefaultInput()->getName() << ' ' << audio::Device::getDefaultInput()->getSampleRate() << endl
    //    << "output: " << dev->getName() << ' ' << dev->getNumOutputChannels() << ' ' << dev->getSampleRate() << endl;
    
    for( const auto &device : ci::audio::Device::getOutputDevices() ) {
        console() << "Device: " << device->getName() << " " << device->getSampleRate()
        << " " << device->getKey() << endl;
        if( device->getSampleRate() == 44100 ) {
            ci::audio::OutputDeviceNodeRef output = ctx->createOutputDeviceNode( device );
            ctx->setOutput( output );
            break;
        }
    }
    
    // set up input source
    auto format = audio::Node::Format().channels( 1 );
    input = ctx->createInputDeviceNode(audio::Device::getDefaultInput(),format);
    
    audioRecorder   = ctx->makeNode(new audio::BufferRecorderNode() );
    bufferPlayer    = ctx->makeNode(new audio::BufferPlayerNode(audioRecorder->getRecordedCopy()) );
    previousNoise   = ctx->makeNode(new audio::GainNode(0.0));
    currentNoise    = ctx->makeNode(new audio::GainNode(1.0));
    
    audioRecorder->setNumSeconds(duration);
    
    input >> currentNoise >> audioRecorder;
    bufferPlayer >> previousNoise >> audioRecorder;
    bufferPlayer >> ctx->getOutput();
    
    
    
    ctx->enable();
    
}

void VoiceLooper::update() {
    if( !isStopped() && recordingTimer.getSeconds() > duration ) stop();
}

void VoiceLooper::start() {
    recordingCount++;
    console() << "Starting voice recording: " << recordingCount << endl;
    float ratio = 1.0 / float(recordingCount);
    previousNoise->setValue(1.0 - ratio);
    currentNoise->setValue(ratio);
    bufferPlayer->enable();
    audioRecorder->enable();
    input->enable();
    bufferPlayer->setBuffer( audioRecorder->getRecordedCopy());
    bufferPlayer->enable();
    bufferPlayer->start();
    audioRecorder->start();
    recordingTimer.start();
    
}

void VoiceLooper::stop() {
    cout << "Stopped recording" << endl;
    recordingTimer.stop();
    if(bufferPlayer) bufferPlayer->stop();
    if( audioRecorder && audioRecorder->isEnabled() ) {
        audioRecorder->stop();
        if(saveAudio && recordingCount > 0) {
            fs::path path = getRecordingPath(recordingCount);
            audioRecorder->writeToFile(path);
        }
        audioRecorder->disable();
    }
}

fs::path VoiceLooper::getRecordingPath (int i) const {
    fs::path path = fs::path(saveFolder);
    path += "/doing_dancing_" + to_string(i) + ".wav";
    return path;
}
