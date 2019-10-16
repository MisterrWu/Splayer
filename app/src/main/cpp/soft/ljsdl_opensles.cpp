//
// Created by wh on 2018/1/16.
//
#include <unistd.h>
#include "ljsdl_opensles.h"

AndroidOpenSLES::AndroidOpenSLES(const int id) :
        _callback(NULL),
        _critSect( SDL_CreateMutex() ),
        _id(id),
        _slEngineObject(NULL),
        _slPlayer(NULL),
        _slEngine(NULL),
        _slPlayerPlay(NULL),
        _slOutputMixObject(NULL),
        _slSpeakerVolume(NULL),
        _playQueueSeq(0),
        _initialized(false),
        _playing(false),
        _playIsInitialized(false),
        _speakerIsInitialized(false),
        _playWarning(0),
        _playError(0),
        _playoutDelay(0),
        _adbSampleRate(0),
        _samplingRateIn(SL_SAMPLINGRATE_16),
        _samplingRateOut(SL_SAMPLINGRATE_16),
        _maxSpeakerVolume(0),
        _minSpeakerVolume(0),
        _loudSpeakerOn(false),
        _playOutBufferIndex(0)
{
    memset(_playQueueBuffer, 0, sizeof(_playQueueBuffer));
    memset(_playOutBuffer,0,160*2*N_PALYOUT_BUFFER_LEN);
}

AndroidOpenSLES::~AndroidOpenSLES() {
    Terminate();
    SDL_DestroyMutex( _critSect );///delete &_critSect;
}


void AndroidOpenSLES::AttachAudioBuffer(
        void (*callback)(void *opaque, char *stream, int len), void* data ) {

    SDL_LockMutex(_critSect);///CriticalSectionScoped lock(&_critSect);

    _callback = callback;
    _opaque = data;

    SDL_UnlockMutex(_critSect);
}

int AndroidOpenSLES::Init() {
    int res = 0;
    SLEngineOption EngineOption[] = {
            { (SLuint32) SL_ENGINEOPTION_THREADSAFE, (SLuint32) SL_BOOLEAN_TRUE },
    };
    SDL_LockMutex(_critSect);//CriticalSectionScoped lock(&_critSect);

    if (_initialized) {
        res = 0;
        goto ERRORHAPPEN; //return 0;
    }
    _playWarning = 0;
    _playError = 0;

    res = slCreateEngine(&_slEngineObject, 1, EngineOption, 0,
                         NULL, NULL);
    if (res != SL_RESULT_SUCCESS) {
        res = -1;
        goto ERRORHAPPEN;
    }
    /* Realizing the SL Engine in synchronous mode. */
    if ((*_slEngineObject)->Realize(_slEngineObject, SL_BOOLEAN_FALSE)
        != SL_RESULT_SUCCESS) {
        res = -1;
        goto ERRORHAPPEN;
    }

    if ((*_slEngineObject)->GetInterface(_slEngineObject, SL_IID_ENGINE,
                                         (void*) &_slEngine)
        != SL_RESULT_SUCCESS) {
        res = -1;
        goto ERRORHAPPEN;
    }

    // Check the sample rate to be used for playback and recording
    if (InitSampleRate() != 0) {
        res = -1;
        goto ERRORHAPPEN;
    }
    _initialized = true;
    res = 0;
    ERRORHAPPEN:
    SDL_UnlockMutex(_critSect);

    return res;
}

int AndroidOpenSLES::Terminate() {

    int res = 0;

    SDL_LockMutex(_critSect); ///CriticalSectionScoped lock(&_critSect);

    if (!_initialized) {
        res = 0;
        goto OUT;
    }
    SDL_UnlockMutex(_critSect); ///播放时候死锁BUG

    // PLAYOUT
    StopPlayout();

    SDL_LockMutex(_critSect); ///播放时候死锁BUG

    if (_slEngineObject != NULL) {
        (*_slEngineObject)->Destroy(_slEngineObject);
        _slEngineObject = NULL;
        _slEngine = NULL;
    }
    _initialized = false;
    OUT:
    SDL_UnlockMutex(_critSect);
    return res;
}

bool AndroidOpenSLES::Initialized() const {
    return (_initialized);
}

int AndroidOpenSLES::SpeakerIsAvailable(bool& available) {

    // We always assume it's available
    available = true;
    return 0;
}

int AndroidOpenSLES::InitSpeaker() {

    int res = 0;

    SDL_LockMutex(_critSect);  //CriticalSectionScoped lock(&_critSect);

    if (_playing) {
        res = -1; //return -1;
        goto OUT;
    }

    // Nothing needs to be done here, we use a flag to have consistent
    // behavior with other platforms
    _speakerIsInitialized = true;

    OUT:
    SDL_UnlockMutex(_critSect);
    return 0;
}

bool AndroidOpenSLES::SpeakerIsInitialized() const {
    return _speakerIsInitialized;
}

int AndroidOpenSLES::SetSpeakerVolume(int volume)
{
    int res = 0;

    SDL_LockMutex(_critSect);

    if (!_playIsInitialized) {
        res = -1;
        goto OUT;
    }

    if( _slSpeakerVolume != NULL){
        res = (*_slSpeakerVolume)->SetVolumeLevel(_slSpeakerVolume, volume);
        if(SL_RESULT_SUCCESS == res){
            res = 0;
        }else{
            res = -1;
        }
    }else{
        res = -1;
    }
    OUT:
    SDL_UnlockMutex(_critSect);
    return res;

}

int AndroidOpenSLES::PlayoutIsAvailable(bool& available) {

    available = false;
    // Try to initialize the playout side
    int res = InitPlayout();

    // Cancel effect of initialization
    StopPlayout();

    if (res != -1) {
        available = true;
    }
    return res;
}

int AndroidOpenSLES::InitPlayout() {

    int res = 0;
    //int res = -1;
    SLDataFormat_PCM pcm;
    SLDataSource audioSource;
    SLDataLocator_AndroidSimpleBufferQueue simpleBufferQueue;
    SLDataSink audioSink;
    SLDataLocator_OutputMix locator_outputmix;

    // Create Output Mix object to be used by player
    SLInterfaceID ids[N_MAX_INTERFACES];
    SLboolean req[N_MAX_INTERFACES];

    SDL_LockMutex(_critSect);  //CriticalSectionScoped lock(&_critSect);

    if (!_initialized) {
        res = -1;
        goto OUT; //return -1;
    }

    if (_playing) {
        res = -1;
        goto OUT; //return -1;
    }

    if (_playIsInitialized) {
        res = 0;
        goto OUT; //return 0;
    }

    // Initialize the speaker
    if (InitSpeaker() == -1) {
    }

    if (_slEngineObject == NULL || _slEngine == NULL) {
        res = -1;
        goto OUT; //return -1;
    }

    for (unsigned int i = 0; i < N_MAX_INTERFACES; i++) {
        ids[i] = SL_IID_NULL;
        req[i] = SL_BOOLEAN_FALSE;
    }
    ids[0] = SL_IID_ENVIRONMENTALREVERB;
    res = (*_slEngine)->CreateOutputMix(_slEngine, &_slOutputMixObject, 1, ids,
                                        req);
    if (res != SL_RESULT_SUCCESS) {
        res = -1;
        goto OUT; //return -1;
    }
    // Realizing the Output Mix object in synchronous mode.
    res = (*_slOutputMixObject)->Realize(_slOutputMixObject, SL_BOOLEAN_FALSE);
    if (res != SL_RESULT_SUCCESS) {
        res = -1;
        goto OUT; //return -1;
    }

    // The code below can be moved to startplayout instead
    /* Setup the data source structure for the buffer queue */
    simpleBufferQueue.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
    /* Two buffers in our buffer queue, to have low latency*/
    simpleBufferQueue.numBuffers = N_PLAY_QUEUE_BUFFERS;
    // TODO(xians), figure out if we should support stereo playout for android
    /* Setup the format of the content in the buffer queue */
    pcm.formatType = SL_DATAFORMAT_PCM;
    pcm.numChannels = 1;
    // _samplingRateOut is initilized in InitSampleRate()
    pcm.samplesPerSec = SL_SAMPLINGRATE_16;
    pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
    pcm.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
    pcm.channelMask = SL_SPEAKER_FRONT_CENTER;
    pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;
    audioSource.pFormat = (void *) &pcm;
    audioSource.pLocator = (void *) &simpleBufferQueue;
    /* Setup the data sink structure */
    locator_outputmix.locatorType = SL_DATALOCATOR_OUTPUTMIX;
    locator_outputmix.outputMix = _slOutputMixObject;
    audioSink.pLocator = (void *) &locator_outputmix;
    audioSink.pFormat = NULL;

    // Set arrays required[] and iidArray[] for SEEK interface
    // (PlayItf is implicit)
    ids[0] = SL_IID_BUFFERQUEUE;
    ids[1] = SL_IID_EFFECTSEND;
    ids[2] = SL_IID_VOLUME;
    req[0] = SL_BOOLEAN_TRUE;
    req[1] = SL_BOOLEAN_TRUE;
    req[2] = SL_BOOLEAN_TRUE;
    ///ids[0] = SL_IID_BUFFERQUEUE;
    ///ids[1] = SL_IID_VOLUME;
    ///ids[2] = SL_IID_ANDROIDCONFIGURATION;
    ///req[0] = SL_BOOLEAN_TRUE;
    ///req[1] = SL_BOOLEAN_TRUE;
    ///req[2] = SL_BOOLEAN_TRUE;
    // Create the music player
    res = (*_slEngine)->CreateAudioPlayer(_slEngine, &_slPlayer, &audioSource,
                                          &audioSink, 3, ids, req);
    if (res != SL_RESULT_SUCCESS) {
        res = -1;
        goto OUT; //return -1;
    }

    // Realizing the player in synchronous mode.
    res = (*_slPlayer)->Realize(_slPlayer, SL_BOOLEAN_FALSE);
    if (res != SL_RESULT_SUCCESS) {
        res = -1;
        goto OUT; //return -1;
    }
    // Get seek and play interfaces
    res = (*_slPlayer)->GetInterface(_slPlayer, SL_IID_PLAY,
                                     (void*) &_slPlayerPlay);
    if (res != SL_RESULT_SUCCESS) {
        res = -1;
        goto OUT; //return -1;
    }
    //buffer queue
    res = (*_slPlayer)->GetInterface(_slPlayer, SL_IID_BUFFERQUEUE,
                                     (void*) &_slPlayerSimpleBufferQueue);
    if (res != SL_RESULT_SUCCESS) {
        res = -1;
        goto OUT; //return -1;
    }

    // Setup to receive buffer queue event callbacks
    res = (*_slPlayerSimpleBufferQueue)->RegisterCallback(
            _slPlayerSimpleBufferQueue,
            PlayerSimpleBufferQueueCallback,
            this);
    if (res != SL_RESULT_SUCCESS) {
        res = -1;
        goto OUT; //return -1;
    }


    res = (*_slPlayer)->GetInterface(_slPlayer, SL_IID_VOLUME, &_slSpeakerVolume);
    if (res != SL_RESULT_SUCCESS) {
        //res = -1;
        //goto OUT; //return -1;
    }

    _playIsInitialized = true;
    res = 0;

    OUT:
    SDL_UnlockMutex(_critSect);
    return res;
}


bool AndroidOpenSLES::PlayoutIsInitialized() const {

    return _playIsInitialized;
}

int AndroidOpenSLES::StartPlayout() {

    int res = 0;
    int nSample10ms = _adbSampleRate / 100;
    char playBuffer[2 * nSample10ms];
    int noSamplesOut(0);

    SDL_LockMutex(_critSect);  ///CriticalSectionScoped lock(&_critSect);

    if (true!=_playIsInitialized) {
        res = -1;
        goto OUT; //return -1;
    }

    if (_playing) {
        res = 0;
        goto OUT; //return 0;
    }

    if (_slPlayerPlay == NULL) {
        res = -1;
        goto OUT; //return -1;
    }
    if (_slPlayerSimpleBufferQueue == NULL) {
        res = -1;
        goto OUT; //return -1;
    }

    ///_recQueueSeq = 0;

    //int res(-1);
    /* Enqueue a set of zero buffers to get the ball rolling */

    {
        // Get data from Audio Device Buffer
        noSamplesOut = RequestPlayoutData(nSample10ms,playBuffer);//noSamplesOut = _ptrAudioBuffer->GetPlayoutData(playBuffer);
        // Insert what we have in data buffer
        memcpy(_playQueueBuffer[_playQueueSeq], playBuffer, 2 * noSamplesOut);

        // write the buffer data we got from VoE into the device
        res = (*_slPlayerSimpleBufferQueue)->Enqueue(
                _slPlayerSimpleBufferQueue,
                (void*) _playQueueBuffer[_playQueueSeq],
                2 * noSamplesOut);
        if (res != SL_RESULT_SUCCESS) {
            //return ; dong return
        }
        _playQueueSeq = (_playQueueSeq + 1) % N_PLAY_QUEUE_BUFFERS;
    }

    // Play the PCM samples using a buffer queue
    res = (*_slPlayerPlay)->SetPlayState(_slPlayerPlay, SL_PLAYSTATE_PLAYING);
    if (res != SL_RESULT_SUCCESS) {
        res = -1;
        goto OUT; //return -1;
    }

    _playWarning = 0;
    _playError = 0;
    _playing = true;
    res = 0;

    OUT:
    SDL_UnlockMutex(_critSect);
    return res;
}

int AndroidOpenSLES::StopPlayout() {

    int res = 0;
    if(!_playing){
        return res ;
    }
    SDL_LockMutex(_critSect); //CriticalSectionScoped lock(&_critSect);

    if (!_playIsInitialized) {
        res = 0;
        goto OUT; //return -1;
    }

    ////modify by SDL
    if (_slPlayerPlay != NULL) {
        // Make sure player is stopped
        res = (*_slPlayerPlay)->SetPlayState(_slPlayerPlay,
                                             SL_PLAYSTATE_STOPPED);
        if (res != SL_RESULT_SUCCESS) {
            res = -1;
            goto OUT; //return -1;
        }
        _slPlayerPlay = NULL;
        _playing = false;
    }

    SDL_UnlockMutex(_critSect);
    usleep(10000); ///sleep 10 ms

    SDL_LockMutex(_critSect);

    if ( _slPlayerSimpleBufferQueue != NULL ) {
        res = (*_slPlayerSimpleBufferQueue)->Clear(_slPlayerSimpleBufferQueue);
        if (res != SL_RESULT_SUCCESS) {
            res = -1;
            goto OUT; //return -1;
        }
        _slPlayerSimpleBufferQueue = NULL;
    }

    if ( _slPlayer != NULL ) {
        // Destroy the player
        (*_slPlayer)->Destroy(_slPlayer);
        _slPlayer = NULL;
    }

    if ( _slOutputMixObject != NULL ) {
        // Destroy Output Mix object
        (*_slOutputMixObject)->Destroy(_slOutputMixObject);
        _slOutputMixObject = NULL;
    }
    ////end

    if( _slSpeakerVolume != NULL){
        _slSpeakerVolume = NULL;
    }

    _playIsInitialized = false;
    _playing = false;
    _playWarning = 0;
    _playError = 0;
    _playQueueSeq = 0;
    res = 0;

    OUT:
    SDL_UnlockMutex(_critSect);

    return res;
}

int AndroidOpenSLES::PlayoutDelay(unsigned short& delayMS) const {
    delayMS = _playoutDelay;
    return 0;
}


bool AndroidOpenSLES::Playing() const {
    return _playing;
}


int AndroidOpenSLES::SetLoudspeakerStatus(bool enable) {
    _loudSpeakerOn = enable;
    return 0;
}

int AndroidOpenSLES::GetLoudspeakerStatus(
        bool& enabled) const {

    enabled = _loudSpeakerOn;
    return 0;
}


void AndroidOpenSLES::PlayerSimpleBufferQueueCallback(
        SLAndroidSimpleBufferQueueItf queueItf,
        void *pContext) {
    AndroidOpenSLES* ptrThis =
            static_cast<AndroidOpenSLES*> (pContext);
    ptrThis->PlayerSimpleBufferQueueCallbackHandler(queueItf);
}

void AndroidOpenSLES::PlayerSimpleBufferQueueCallbackHandler(
        SLAndroidSimpleBufferQueueItf queueItf) {

    int res;
    SDL_LockMutex( _critSect );//Lock();
    if (_playing && (_playQueueSeq < N_PLAY_QUEUE_BUFFERS)) {
        unsigned int noSamp10ms = _adbSampleRate / 100;
        // Max 10 ms @ samplerate kHz / 16 bit
        char playBuffer[2 * noSamp10ms];
        int noSamplesOut = 0;

        // Assumption for implementation
        // assert(PLAYBUFSIZESAMPLES == noSamp10ms);

        // TODO(xians), update the playout delay
        ///拿出10ms,16位16000HZ的声音数据
        //noSamplesOut = _ptrAudioBuffer->RequestPlayoutData(noSamp10ms);
        // Get data from Audio Device Buffer
        noSamplesOut = RequestPlayoutData(noSamp10ms,playBuffer);//noSamplesOut = _ptrAudioBuffer->GetPlayoutData(playBuffer);
        // Cast OK since only equality comparison
        if (noSamp10ms != (unsigned int) noSamplesOut) {
            if (_playWarning > 0) {
            }
            _playWarning = 1;
        }
        // Insert what we have in data buffer
        ///将数据复制到播放缓冲区队列去
        memcpy(_playQueueBuffer[_playQueueSeq], playBuffer, 2 * noSamplesOut);

        // write the buffer data we got from VoE into the device
        ///进行播放
        res = (*_slPlayerSimpleBufferQueue)->Enqueue(
                _slPlayerSimpleBufferQueue,
                _playQueueBuffer[_playQueueSeq],
                2 * noSamplesOut);
        if (res != SL_RESULT_SUCCESS) {
            goto OUT;
            //return;
        }
        // update the playout delay
        //UpdatePlayoutDelay(noSamplesOut);
        // update the play buffer sequency
        _playQueueSeq = (_playQueueSeq + 1) % N_PLAY_QUEUE_BUFFERS;
    }else{
        ///modify by SDL
        // Max 10 ms @ samplerate kHz / 16 bit
        unsigned int noSamp10ms = _adbSampleRate / 100;
        ///unsigned int noSamp10ms = _adbSampleRate / 1000; //1ms @ samplerate kHz / 16 bit
        char playBuffer[2 * noSamp10ms];
        memset(playBuffer,0,2 * noSamp10ms);
        if(!_playing){
        }
        if( _playQueueSeq >= N_PLAY_QUEUE_BUFFERS){
        }
        res = (*_slPlayerSimpleBufferQueue)->Enqueue(
                _slPlayerSimpleBufferQueue,
                playBuffer,
                2 * noSamp10ms);
        if (res != SL_RESULT_SUCCESS) {
            goto OUT;//UnLock();
            //return;
        }
    }
    OUT:
    SDL_UnlockMutex( _critSect );//UnLock();
}

void AndroidOpenSLES::UpdatePlayoutDelay(
        int nSamplePlayed) {
    // currently just do some simple calculation, should we setup a timer for
    // the callback to have a more accurate delay
    // Android CCD asks for 10ms as the maximum warm output latency, so we
    // simply add (nPlayQueueBuffer -1 + 0.5)*10ms
    // This playout delay should be seldom changed
    _playoutDelay = (N_PLAY_QUEUE_BUFFERS - 0.5) * 10 + N_PLAY_QUEUE_BUFFERS
                                                        * nSamplePlayed / (_adbSampleRate / 1000);
}


int AndroidOpenSLES::InitSampleRate() {
    if (_slEngineObject == NULL) {
        return -1;
    }

    _samplingRateIn = SL_SAMPLINGRATE_16;
    _samplingRateOut = SL_SAMPLINGRATE_16;
    _adbSampleRate = 16000;

    return 0;

}


int AndroidOpenSLES::RequestPlayoutData(int sample,  char* playBuffer)
{
    if(_playOutBufferIndex == N_PALYOUT_BUFFER_LEN ){
        _callback(_opaque, _playOutBuffer, (N_PLAY_SAMPLES_PER_SEC/100)*2*N_PALYOUT_BUFFER_LEN);
        _playOutBufferIndex = 0;
    }
    memcpy(playBuffer ,_playOutBuffer + sample*2*_playOutBufferIndex, sample*2);
    _playOutBufferIndex++;
    return sample;
}
