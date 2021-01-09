// Joan Andrés (@Andres6936) Github.

#if defined(__WINDOWS_ASIO__) // ASIO API on Windows

// The ASIO API is designed around a callback scheme, so this
// implementation is similar to that used for OS-X CoreAudio and Linux
// Jack.  The primary constraint with ASIO is that it only allows
// access to a single driver at a time.  Thus, it is not possible to
// have more than one simultaneous RtAudio stream.
//
// This implementation also requires a number of external ASIO files
// and a few global variables.  The ASIO callback scheme does not
// allow for the passing of user data, so we must create a global
// pointer to our callbackInfo structure.
//
// On unix systems, we make use of a pthread condition variable.
// Since there is no equivalent in Windows, I hacked something based
// on information found in
// http://www.cs.wustl.edu/~schmidt/win32-cv-1.html.

#include "asiosys.h"
#include "asio.h"
#include "iasiothiscallresolver.h"
#include "asiodrivers.h"
#include <cmath>

AsioDrivers drivers;
ASIOCallbacks asioCallbacks;
ASIODriverInfo driverInfo;
CallbackInfo *asioCallbackInfo;
bool asioXRun;

struct AsioHandle {
  int drainCounter;       // Tracks callback counts when draining
  bool internalDrain;     // Indicates if stop is initiated from callback or not.
  ASIOBufferInfo *bufferInfos;
  HANDLE condition;

  AsioHandle()
    :drainCounter(0), internalDrain(false), bufferInfos(0) {}
};

// Function declarations (definitions at end of section)
static const char* getAsioErrorString( ASIOError result );
void sampleRateChanged( ASIOSampleRate sRate );
long asioMessages( long selector, long value, void* message, double* opt );

RtApiAsio :: RtApiAsio()
{
  // ASIO cannot run on a multi-threaded appartment. You can call
  // CoInitialize beforehand, but it must be for appartment threading
  // (in which case, CoInitilialize will return S_FALSE here).
  coInitialized_ = false;
  HRESULT hr = CoInitialize( NULL );
  if ( FAILED(hr) ) {
    errorText_ = "RtApiAsio::ASIO requires a single-threaded appartment. Call CoInitializeEx(0,COINIT_APARTMENTTHREADED)";
    error( RtError::WARNING );
  }
  coInitialized_ = true;

  drivers.removeCurrentDriver();
  driverInfo.asioVersion = 2;

  // See note in DirectSound implementation about GetDesktopWindow().
  driverInfo.sysRef = GetForegroundWindow();
}

RtApiAsio :: ~RtApiAsio()
{
  if ( stream_.state != STREAM_CLOSED ) closeStream();
  if ( coInitialized_ ) CoUninitialize();
}

unsigned int RtApiAsio :: getDeviceCount( void )
{
  return (unsigned int) drivers.asioGetNumDev();
}

RtDeviceInfo RtApiAsio :: getDeviceInfo( unsigned int device )
{
  RtDeviceInfo info;
  info.probed = false;

  // Get device ID
  unsigned int nDevices = getDeviceCount();
  if ( nDevices == 0 ) {
    errorText_ = "RtApiAsio::getDeviceInfo: no devices found!";
    error( RtError::INVALID_USE );
  }

  if ( device >= nDevices ) {
    errorText_ = "RtApiAsio::getDeviceInfo: device ID is invalid!";
    error( RtError::INVALID_USE );
  }

  // If a stream is already open, we cannot probe other devices.  Thus, use the saved results.
  if ( stream_.state != STREAM_CLOSED ) {
    if ( device >= devices_.size() ) {
      errorText_ = "RtApiAsio::getDeviceInfo: device ID was not present before stream was opened.";
      error( RtError::WARNING );
      return info;
    }
    return devices_[ device ];
  }

  char driverName[32];
  ASIOError result = drivers.asioGetDriverName( (int) device, driverName, 32 );
  if ( result != ASE_OK ) {
    errorStream_ << "RtApiAsio::getDeviceInfo: unable to get driver name (" << getAsioErrorString( result ) << ").";
    errorText_ = errorStream_.str();
    error( RtError::WARNING );
    return info;
  }

  info.name = driverName;

  if ( !drivers.loadDriver( driverName ) ) {
    errorStream_ << "RtApiAsio::getDeviceInfo: unable to load driver (" << driverName << ").";
    errorText_ = errorStream_.str();
    error( RtError::WARNING );
    return info;
  }

  result = ASIOInit( &driverInfo );
  if ( result != ASE_OK ) {
    errorStream_ << "RtApiAsio::getDeviceInfo: error (" << getAsioErrorString( result ) << ") initializing driver (" << driverName << ").";
    errorText_ = errorStream_.str();
    error( RtError::WARNING );
    return info;
  }

  // Determine the device channel information.
  long inputChannels, outputChannels;
  result = ASIOGetChannels( &inputChannels, &outputChannels );
  if ( result != ASE_OK ) {
    drivers.removeCurrentDriver();
    errorStream_ << "RtApiAsio::getDeviceInfo: error (" << getAsioErrorString( result ) << ") getting channel count (" << driverName << ").";
    errorText_ = errorStream_.str();
    error( RtError::WARNING );
    return info;
  }

  info.outputChannels = outputChannels;
  info.inputChannels = inputChannels;
  if ( info.outputChannels > 0 && info.inputChannels > 0 )
    info.duplexChannels = (info.outputChannels > info.inputChannels) ? info.inputChannels : info.outputChannels;

  // Determine the supported sample rates.
  info.sampleRates.clear();
  for ( unsigned int i=0; i<MAX_SAMPLE_RATES; i++ ) {
    result = ASIOCanSampleRate( (ASIOSampleRate) SAMPLE_RATES[i] );
    if ( result == ASE_OK )
      info.sampleRates.push_back( SAMPLE_RATES[i] );
  }

  // Determine supported data types ... just check first channel and assume rest are the same.
  ASIOChannelInfo channelInfo;
  channelInfo.channel = 0;
  channelInfo.isInput = true;
  if ( info.inputChannels <= 0 ) channelInfo.isInput = false;
  result = ASIOGetChannelInfo( &channelInfo );
  if ( result != ASE_OK ) {
    drivers.removeCurrentDriver();
    errorStream_ << "RtApiAsio::getDeviceInfo: error (" << getAsioErrorString( result ) << ") getting driver channel info (" << driverName << ").";
    errorText_ = errorStream_.str();
    error( RtError::WARNING );
    return info;
  }

  info.nativeFormats = 0;
  if ( channelInfo.type == ASIOSTInt16MSB || channelInfo.type == ASIOSTInt16LSB )
    info.nativeFormats |= RTAUDIO_SINT16;
  else if ( channelInfo.type == ASIOSTInt32MSB || channelInfo.type == ASIOSTInt32LSB )
    info.nativeFormats |= RTAUDIO_SINT32;
  else if ( channelInfo.type == ASIOSTFloat32MSB || channelInfo.type == ASIOSTFloat32LSB )
    info.nativeFormats |= RTAUDIO_FLOAT32;
  else if ( channelInfo.type == ASIOSTFloat64MSB || channelInfo.type == ASIOSTFloat64LSB )
    info.nativeFormats |= RTAUDIO_FLOAT64;

  if ( info.outputChannels > 0 )
    if ( getDefaultOutputDevice() == device ) info.isDefaultOutput = true;
  if ( info.inputChannels > 0 )
    if ( getDefaultInputDevice() == device ) info.isDefaultInput = true;

  info.probed = true;
  drivers.removeCurrentDriver();
  return info;
}

void bufferSwitch( long index, ASIOBool processNow )
{
  RtApiAsio *object = (RtApiAsio *) asioCallbackInfo->object;
  object->callbackEvent( index );
}

void RtApiAsio :: saveDeviceInfo( void )
{
  devices_.clear();

  unsigned int nDevices = getDeviceCount();
  devices_.resize( nDevices );
  for ( unsigned int i=0; i<nDevices; i++ )
    devices_[i] = getDeviceInfo( i );
}

bool RtApiAsio :: probeDeviceOpen( unsigned int device, StreamMode mode, unsigned int channels,
                                   unsigned int firstChannel, unsigned int sampleRate,
                                   RtAudioFormat format, unsigned int *bufferSize,
                                   RtAudio::StreamOptions *options )
{
  // For ASIO, a duplex stream MUST use the same driver.
  if ( mode == INPUT && stream_.mode == OUTPUT && stream_.device[0] != device ) {
    errorText_ = "RtApiAsio::probeDeviceOpen: an ASIO duplex stream must use the same device for input and output!";
    return FAILURE;
  }

  char driverName[32];
  ASIOError result = drivers.asioGetDriverName( (int) device, driverName, 32 );
  if ( result != ASE_OK ) {
    errorStream_ << "RtApiAsio::probeDeviceOpen: unable to get driver name (" << getAsioErrorString( result ) << ").";
    errorText_ = errorStream_.str();
    return FAILURE;
  }

  // Only load the driver once for duplex stream.
  if ( mode != INPUT || stream_.mode != OUTPUT ) {
    // The getDeviceInfo() function will not work when a stream is open
    // because ASIO does not allow multiple devices to run at the same
    // time.  Thus, we'll probe the system before opening a stream and
    // save the results for use by getDeviceInfo().
    this->saveDeviceInfo();

    if ( !drivers.loadDriver( driverName ) ) {
      errorStream_ << "RtApiAsio::probeDeviceOpen: unable to load driver (" << driverName << ").";
      errorText_ = errorStream_.str();
      return FAILURE;
    }

    result = ASIOInit( &driverInfo );
    if ( result != ASE_OK ) {
      errorStream_ << "RtApiAsio::probeDeviceOpen: error (" << getAsioErrorString( result ) << ") initializing driver (" << driverName << ").";
      errorText_ = errorStream_.str();
      return FAILURE;
    }
  }

  // Check the device channel count.
  long inputChannels, outputChannels;
  result = ASIOGetChannels( &inputChannels, &outputChannels );
  if ( result != ASE_OK ) {
    drivers.removeCurrentDriver();
    errorStream_ << "RtApiAsio::probeDeviceOpen: error (" << getAsioErrorString( result ) << ") getting channel count (" << driverName << ").";
    errorText_ = errorStream_.str();
    return FAILURE;
  }

  if ( ( mode == OUTPUT && (channels+firstChannel) > (unsigned int) outputChannels) ||
       ( mode == INPUT && (channels+firstChannel) > (unsigned int) inputChannels) ) {
    drivers.removeCurrentDriver();
    errorStream_ << "RtApiAsio::probeDeviceOpen: driver (" << driverName << ") does not support requested channel count (" << channels << ") + offset (" << firstChannel << ").";
    errorText_ = errorStream_.str();
    return FAILURE;
  }
  stream_.nDeviceChannels[mode] = channels;
  stream_.nUserChannels[mode] = channels;
  stream_.channelOffset[mode] = firstChannel;

  // Verify the sample rate is supported.
  result = ASIOCanSampleRate( (ASIOSampleRate) sampleRate );
  if ( result != ASE_OK ) {
    drivers.removeCurrentDriver();
    errorStream_ << "RtApiAsio::probeDeviceOpen: driver (" << driverName << ") does not support requested sample rate (" << sampleRate << ").";
    errorText_ = errorStream_.str();
    return FAILURE;
  }

  // Get the current sample rate
  ASIOSampleRate currentRate;
  result = ASIOGetSampleRate( &currentRate );
  if ( result != ASE_OK ) {
    drivers.removeCurrentDriver();
    errorStream_ << "RtApiAsio::probeDeviceOpen: driver (" << driverName << ") error getting sample rate.";
    errorText_ = errorStream_.str();
    return FAILURE;
  }

  // Set the sample rate only if necessary
  if ( currentRate != sampleRate ) {
    result = ASIOSetSampleRate( (ASIOSampleRate) sampleRate );
    if ( result != ASE_OK ) {
      drivers.removeCurrentDriver();
      errorStream_ << "RtApiAsio::probeDeviceOpen: driver (" << driverName << ") error setting sample rate (" << sampleRate << ").";
      errorText_ = errorStream_.str();
      return FAILURE;
    }
  }

  // Determine the driver data type.
  ASIOChannelInfo channelInfo;
  channelInfo.channel = 0;
  if ( mode == OUTPUT ) channelInfo.isInput = false;
  else channelInfo.isInput = true;
  result = ASIOGetChannelInfo( &channelInfo );
  if ( result != ASE_OK ) {
    drivers.removeCurrentDriver();
    errorStream_ << "RtApiAsio::probeDeviceOpen: driver (" << driverName << ") error (" << getAsioErrorString( result ) << ") getting data format.";
    errorText_ = errorStream_.str();
    return FAILURE;
  }

  // Assuming WINDOWS host is always little-endian.
  stream_.doByteSwap[mode] = false;
  stream_.userFormat = format;
  stream_.deviceFormat[mode] = 0;
  if ( channelInfo.type == ASIOSTInt16MSB || channelInfo.type == ASIOSTInt16LSB ) {
    stream_.deviceFormat[mode] = RTAUDIO_SINT16;
    if ( channelInfo.type == ASIOSTInt16MSB ) stream_.doByteSwap[mode] = true;
  }
  else if ( channelInfo.type == ASIOSTInt32MSB || channelInfo.type == ASIOSTInt32LSB ) {
    stream_.deviceFormat[mode] = RTAUDIO_SINT32;
    if ( channelInfo.type == ASIOSTInt32MSB ) stream_.doByteSwap[mode] = true;
  }
  else if ( channelInfo.type == ASIOSTFloat32MSB || channelInfo.type == ASIOSTFloat32LSB ) {
    stream_.deviceFormat[mode] = RTAUDIO_FLOAT32;
    if ( channelInfo.type == ASIOSTFloat32MSB ) stream_.doByteSwap[mode] = true;
  }
  else if ( channelInfo.type == ASIOSTFloat64MSB || channelInfo.type == ASIOSTFloat64LSB ) {
    stream_.deviceFormat[mode] = RTAUDIO_FLOAT64;
    if ( channelInfo.type == ASIOSTFloat64MSB ) stream_.doByteSwap[mode] = true;
  }

  if ( stream_.deviceFormat[mode] == 0 ) {
    drivers.removeCurrentDriver();
    errorStream_ << "RtApiAsio::probeDeviceOpen: driver (" << driverName << ") data format not supported by RtAudio.";
    errorText_ = errorStream_.str();
    return FAILURE;
  }

  // Set the buffer size.  For a duplex stream, this will end up
  // setting the buffer size based on the input constraints, which
  // should be ok.
  long minSize, maxSize, preferSize, granularity;
  result = ASIOGetBufferSize( &minSize, &maxSize, &preferSize, &granularity );
  if ( result != ASE_OK ) {
    drivers.removeCurrentDriver();
    errorStream_ << "RtApiAsio::probeDeviceOpen: driver (" << driverName << ") error (" << getAsioErrorString( result ) << ") getting buffer size.";
    errorText_ = errorStream_.str();
    return FAILURE;
  }

  if ( *bufferSize < (unsigned int) minSize ) *bufferSize = (unsigned int) minSize;
  else if ( *bufferSize > (unsigned int) maxSize ) *bufferSize = (unsigned int) maxSize;
  else if ( granularity == -1 ) {
    // Make sure bufferSize is a power of two.
    int log2_of_min_size = 0;
    int log2_of_max_size = 0;

    for ( unsigned int i = 0; i < sizeof(long) * 8; i++ ) {
      if ( minSize & ((long)1 << i) ) log2_of_min_size = i;
      if ( maxSize & ((long)1 << i) ) log2_of_max_size = i;
    }

    long min_delta = std::abs( (long)*bufferSize - ((long)1 << log2_of_min_size) );
    int min_delta_num = log2_of_min_size;

    for (int i = log2_of_min_size + 1; i <= log2_of_max_size; i++) {
      long current_delta = std::abs( (long)*bufferSize - ((long)1 << i) );
      if (current_delta < min_delta) {
        min_delta = current_delta;
        min_delta_num = i;
      }
    }

    *bufferSize = ( (unsigned int)1 << min_delta_num );
    if ( *bufferSize < (unsigned int) minSize ) *bufferSize = (unsigned int) minSize;
    else if ( *bufferSize > (unsigned int) maxSize ) *bufferSize = (unsigned int) maxSize;
  }
  else if ( granularity != 0 ) {
    // Set to an even multiple of granularity, rounding up.
    *bufferSize = (*bufferSize + granularity-1) / granularity * granularity;
  }

  if ( mode == INPUT && stream_.mode == OUTPUT && stream_.bufferSize != *bufferSize ) {
    drivers.removeCurrentDriver();
    errorText_ = "RtApiAsio::probeDeviceOpen: input/output buffersize discrepancy!";
    return FAILURE;
  }

  stream_.bufferSize = *bufferSize;
  stream_.nBuffers = 2;

  if ( options && options->flags & RTAUDIO_NONINTERLEAVED ) stream_.userInterleaved = false;
  else stream_.userInterleaved = true;

  // ASIO always uses non-interleaved buffers.
  stream_.deviceInterleaved[mode] = false;

  // Allocate, if necessary, our AsioHandle structure for the stream.
  AsioHandle *handle = (AsioHandle *) stream_.apiHandle;
  if ( handle == 0 ) {
    try {
      handle = new AsioHandle;
    }
    catch ( std::bad_alloc& ) {
      //if ( handle == NULL ) {
      drivers.removeCurrentDriver();
      errorText_ = "RtApiAsio::probeDeviceOpen: error allocating AsioHandle memory.";
      return FAILURE;
    }
    handle->bufferInfos = 0;

    // Create a manual-reset event.
    handle->condition = CreateEvent( NULL,   // no security
                                     TRUE,   // manual-reset
                                     FALSE,  // non-signaled initially
                                     NULL ); // unnamed
    stream_.apiHandle = (void *) handle;
  }

  // Create the ASIO internal buffers.  Since RtAudio sets up input
  // and output separately, we'll have to dispose of previously
  // created output buffers for a duplex stream.
  long inputLatency, outputLatency;
  if ( mode == INPUT && stream_.mode == OUTPUT ) {
    ASIODisposeBuffers();
    if ( handle->bufferInfos ) free( handle->bufferInfos );
  }

  // Allocate, initialize, and save the bufferInfos in our stream callbackInfo structure.
  bool buffersAllocated = false;
  unsigned int i, nChannels = stream_.nDeviceChannels[0] + stream_.nDeviceChannels[1];
  handle->bufferInfos = (ASIOBufferInfo *) malloc( nChannels * sizeof(ASIOBufferInfo) );
  if ( handle->bufferInfos == NULL ) {
    errorStream_ << "RtApiAsio::probeDeviceOpen: error allocating bufferInfo memory for driver (" << driverName << ").";
    errorText_ = errorStream_.str();
    goto error;
  }

  ASIOBufferInfo *infos;
  infos = handle->bufferInfos;
  for ( i=0; i<stream_.nDeviceChannels[0]; i++, infos++ ) {
    infos->isInput = ASIOFalse;
    infos->channelNum = i + stream_.channelOffset[0];
    infos->buffers[0] = infos->buffers[1] = 0;
  }
  for ( i=0; i<stream_.nDeviceChannels[1]; i++, infos++ ) {
    infos->isInput = ASIOTrue;
    infos->channelNum = i + stream_.channelOffset[1];
    infos->buffers[0] = infos->buffers[1] = 0;
  }

  // Set up the ASIO callback structure and create the ASIO data buffers.
  asioCallbacks.bufferSwitch = &bufferSwitch;
  asioCallbacks.sampleRateDidChange = &sampleRateChanged;
  asioCallbacks.asioMessage = &asioMessages;
  asioCallbacks.bufferSwitchTimeInfo = NULL;
  result = ASIOCreateBuffers( handle->bufferInfos, nChannels, stream_.bufferSize, &asioCallbacks );
  if ( result != ASE_OK ) {
    errorStream_ << "RtApiAsio::probeDeviceOpen: driver (" << driverName << ") error (" << getAsioErrorString( result ) << ") creating buffers.";
    errorText_ = errorStream_.str();
    goto error;
  }
  buffersAllocated = true;

  // Set flags for buffer conversion.
  stream_.doConvertBuffer[mode] = false;
  if ( stream_.userFormat != stream_.deviceFormat[mode] )
    stream_.doConvertBuffer[mode] = true;
  if ( stream_.userInterleaved != stream_.deviceInterleaved[mode] &&
       stream_.nUserChannels[mode] > 1 )
    stream_.doConvertBuffer[mode] = true;

  // Allocate necessary internal buffers
  unsigned long bufferBytes;
  bufferBytes = stream_.nUserChannels[mode] * *bufferSize * formatBytes( stream_.userFormat );
  stream_.userBuffer[mode] = (char *) calloc( bufferBytes, 1 );
  if ( stream_.userBuffer[mode] == NULL ) {
    errorText_ = "RtApiAsio::probeDeviceOpen: error allocating user buffer memory.";
    goto error;
  }

  if ( stream_.doConvertBuffer[mode] ) {

    bool makeBuffer = true;
    bufferBytes = stream_.nDeviceChannels[mode] * formatBytes( stream_.deviceFormat[mode] );
    if ( mode == INPUT ) {
      if ( stream_.mode == OUTPUT && stream_.deviceBuffer ) {
        unsigned long bytesOut = stream_.nDeviceChannels[0] * formatBytes( stream_.deviceFormat[0] );
        if ( bufferBytes <= bytesOut ) makeBuffer = false;
      }
    }

    if ( makeBuffer ) {
      bufferBytes *= *bufferSize;
      if ( stream_.deviceBuffer ) free( stream_.deviceBuffer );
      stream_.deviceBuffer = (char *) calloc( bufferBytes, 1 );
      if ( stream_.deviceBuffer == NULL ) {
        errorText_ = "RtApiAsio::probeDeviceOpen: error allocating device buffer memory.";
        goto error;
      }
    }
  }

  stream_.sampleRate = sampleRate;
  stream_.device[mode] = device;
  stream_.state = STREAM_STOPPED;
  asioCallbackInfo = &stream_.callbackInfo;
  stream_.callbackInfo.object = (void *) this;
  if ( stream_.mode == OUTPUT && mode == INPUT )
    // We had already set up an output stream.
    stream_.mode = DUPLEX;
  else
    stream_.mode = mode;

  // Determine device latencies
  result = ASIOGetLatencies( &inputLatency, &outputLatency );
  if ( result != ASE_OK ) {
    errorStream_ << "RtApiAsio::probeDeviceOpen: driver (" << driverName << ") error (" << getAsioErrorString( result ) << ") getting latency.";
    errorText_ = errorStream_.str();
    error( RtError::WARNING); // warn but don't fail
  }
  else {
    stream_.latency[0] = outputLatency;
    stream_.latency[1] = inputLatency;
  }

  // Setup the buffer conversion information structure.  We don't use
  // buffers to do channel offsets, so we override that parameter
  // here.
  if ( stream_.doConvertBuffer[mode] ) setConvertInfo( mode, 0 );

  return SUCCESS;

 error:
  if ( buffersAllocated )
    ASIODisposeBuffers();
  drivers.removeCurrentDriver();

  if ( handle ) {
    CloseHandle( handle->condition );
    if ( handle->bufferInfos )
      free( handle->bufferInfos );
    delete handle;
    stream_.apiHandle = 0;
  }

  for ( int i=0; i<2; i++ ) {
    if ( stream_.userBuffer[i] ) {
      free( stream_.userBuffer[i] );
      stream_.userBuffer[i] = 0;
    }
  }

  if ( stream_.deviceBuffer ) {
    free( stream_.deviceBuffer );
    stream_.deviceBuffer = 0;
  }

  return FAILURE;
}

void RtApiAsio :: closeStream()
{
  if ( stream_.state == STREAM_CLOSED ) {
    errorText_ = "RtApiAsio::closeStream(): no open stream to close!";
    error( RtError::WARNING );
    return;
  }

  if ( stream_.state == STREAM_RUNNING ) {
    stream_.state = STREAM_STOPPED;
    ASIOStop();
  }
  ASIODisposeBuffers();
  drivers.removeCurrentDriver();

  AsioHandle *handle = (AsioHandle *) stream_.apiHandle;
  if ( handle ) {
    CloseHandle( handle->condition );
    if ( handle->bufferInfos )
      free( handle->bufferInfos );
    delete handle;
    stream_.apiHandle = 0;
  }

  for ( int i=0; i<2; i++ ) {
    if ( stream_.userBuffer[i] ) {
      free( stream_.userBuffer[i] );
      stream_.userBuffer[i] = 0;
    }
  }

  if ( stream_.deviceBuffer ) {
    free( stream_.deviceBuffer );
    stream_.deviceBuffer = 0;
  }

  stream_.mode = UNINITIALIZED;
  stream_.state = STREAM_CLOSED;
}

bool stopThreadCalled = false;

void RtApiAsio :: startStream()
{
  verifyStream();
  if ( stream_.state == STREAM_RUNNING ) {
    errorText_ = "RtApiAsio::startStream(): the stream is already running!";
    error( RtError::WARNING );
    return;
  }

  //MUTEX_LOCK( &stream_.mutex );

  AsioHandle *handle = (AsioHandle *) stream_.apiHandle;
  ASIOError result = ASIOStart();
  if ( result != ASE_OK ) {
    errorStream_ << "RtApiAsio::startStream: error (" << getAsioErrorString( result ) << ") starting device.";
    errorText_ = errorStream_.str();
    goto unlock;
  }

  handle->drainCounter = 0;
  handle->internalDrain = false;
  ResetEvent( handle->condition );
  stream_.state = STREAM_RUNNING;
  asioXRun = false;

 unlock:
  //MUTEX_UNLOCK( &stream_.mutex );

  stopThreadCalled = false;

  if ( result == ASE_OK ) return;
  error( RtError::SYSTEM_ERROR );
}

void RtApiAsio :: stopStream()
{
  verifyStream();
  if ( stream_.state == STREAM_STOPPED ) {
    errorText_ = "RtApiAsio::stopStream(): the stream is already stopped!";
    error( RtError::WARNING );
    return;
  }

  /*
  MUTEX_LOCK( &stream_.mutex );

  if ( stream_.state == STREAM_STOPPED ) {
    MUTEX_UNLOCK( &stream_.mutex );
    return;
  }
  */

  AsioHandle *handle = (AsioHandle *) stream_.apiHandle;
  if ( stream_.mode == OUTPUT || stream_.mode == DUPLEX ) {
    if ( handle->drainCounter == 0 ) {
      handle->drainCounter = 2;
      //      MUTEX_UNLOCK( &stream_.mutex );
      WaitForSingleObject( handle->condition, INFINITE );  // block until signaled
      //ResetEvent( handle->condition );
      //      MUTEX_LOCK( &stream_.mutex );
    }
  }

  stream_.state = STREAM_STOPPED;

  ASIOError result = ASIOStop();
  if ( result != ASE_OK ) {
    errorStream_ << "RtApiAsio::stopStream: error (" << getAsioErrorString( result ) << ") stopping device.";
    errorText_ = errorStream_.str();
  }

  //  MUTEX_UNLOCK( &stream_.mutex );

  if ( result == ASE_OK ) return;
  error( RtError::SYSTEM_ERROR );
}

void RtApiAsio :: abortStream()
{
  verifyStream();
  if ( stream_.state == STREAM_STOPPED ) {
    errorText_ = "RtApiAsio::abortStream(): the stream is already stopped!";
    error( RtError::WARNING );
    return;
  }

  // The following lines were commented-out because some behavior was
  // noted where the device buffers need to be zeroed to avoid
  // continuing sound, even when the device buffers are completely
  // disposed.  So now, calling abort is the same as calling stop.
  // AsioHandle *handle = (AsioHandle *) stream_.apiHandle;
  // handle->drainCounter = 2;
  stopStream();
}

// This function will be called by a spawned thread when the user
// callback function signals that the stream should be stopped or
// aborted.  It is necessary to handle it this way because the
// callbackEvent() function must return before the ASIOStop()
// function will return.
extern "C" unsigned __stdcall asioStopStream( void *ptr )
{
  CallbackInfo *info = (CallbackInfo *) ptr;
  RtApiAsio *object = (RtApiAsio *) info->object;

  object->stopStream();

  _endthreadex( 0 );
  return 0;
}

bool RtApiAsio :: callbackEvent( long bufferIndex )
{
  if ( stream_.state == STREAM_STOPPED ) return SUCCESS;
  if ( stopThreadCalled ) return SUCCESS;
  if ( stream_.state == STREAM_CLOSED ) {
    errorText_ = "RtApiAsio::callbackEvent(): the stream is closed ... this shouldn't happen!";
    error( RtError::WARNING );
    return FAILURE;
  }

  CallbackInfo *info = (CallbackInfo *) &stream_.callbackInfo;
  AsioHandle *handle = (AsioHandle *) stream_.apiHandle;

  // Check if we were draining the stream and signal if finished.
  if ( handle->drainCounter > 3 ) {
    if ( handle->internalDrain == false )
      SetEvent( handle->condition );
    else { // spawn a thread to stop the stream
      unsigned threadId;
      stopThreadCalled = true;
      stream_.callbackInfo.thread = _beginthreadex( NULL, 0, &asioStopStream,
                                                    &stream_.callbackInfo, 0, &threadId );
    }
    return SUCCESS;
  }

  /*MUTEX_LOCK( &stream_.mutex );

  // The state might change while waiting on a mutex.
  if ( stream_.state == STREAM_STOPPED ) goto unlock; */

  // Invoke user callback to get fresh output data UNLESS we are
  // draining stream.
  if ( handle->drainCounter == 0 ) {
    RtAudioCallback callback = (RtAudioCallback) info->callback;
    double streamTime = getStreamTime();
    RtAudioStreamStatus status = 0;
    if ( stream_.mode != INPUT && asioXRun == true ) {
      status |= RTAUDIO_OUTPUT_UNDERFLOW;
      asioXRun = false;
    }
    if ( stream_.mode != OUTPUT && asioXRun == true ) {
      status |= RTAUDIO_INPUT_OVERFLOW;
      asioXRun = false;
    }
    handle->drainCounter = callback( stream_.userBuffer[0], stream_.userBuffer[1],
                                     stream_.bufferSize, streamTime, status, info->userData );
    if ( handle->drainCounter == 2 ) {
      //      MUTEX_UNLOCK( &stream_.mutex );
      //      abortStream();
      unsigned threadId;
      stopThreadCalled = true;
      stream_.callbackInfo.thread = _beginthreadex( NULL, 0, &asioStopStream,
                                                    &stream_.callbackInfo, 0, &threadId );
      return SUCCESS;
    }
    else if ( handle->drainCounter == 1 )
      handle->internalDrain = true;
  }

  unsigned int nChannels, bufferBytes, i, j;
  nChannels = stream_.nDeviceChannels[0] + stream_.nDeviceChannels[1];
  if ( stream_.mode == OUTPUT || stream_.mode == DUPLEX ) {

    bufferBytes = stream_.bufferSize * formatBytes( stream_.deviceFormat[0] );

    if ( handle->drainCounter > 1 ) { // write zeros to the output stream

      for ( i=0, j=0; i<nChannels; i++ ) {
        if ( handle->bufferInfos[i].isInput != ASIOTrue )
          memset( handle->bufferInfos[i].buffers[bufferIndex], 0, bufferBytes );
      }

    }
    else if ( stream_.doConvertBuffer[0] ) {

      convertBuffer( stream_.deviceBuffer, stream_.userBuffer[0], stream_.convertInfo[0] );
      if ( stream_.doByteSwap[0] )
        byteSwapBuffer( stream_.deviceBuffer,
                        stream_.bufferSize * stream_.nDeviceChannels[0],
                        stream_.deviceFormat[0] );

      for ( i=0, j=0; i<nChannels; i++ ) {
        if ( handle->bufferInfos[i].isInput != ASIOTrue )
          memcpy( handle->bufferInfos[i].buffers[bufferIndex],
                  &stream_.deviceBuffer[j++*bufferBytes], bufferBytes );
      }

    }
    else {

      if ( stream_.doByteSwap[0] )
        byteSwapBuffer( stream_.userBuffer[0],
                        stream_.bufferSize * stream_.nUserChannels[0],
                        stream_.userFormat );

      for ( i=0, j=0; i<nChannels; i++ ) {
        if ( handle->bufferInfos[i].isInput != ASIOTrue )
          memcpy( handle->bufferInfos[i].buffers[bufferIndex],
                  &stream_.userBuffer[0][bufferBytes*j++], bufferBytes );
      }

    }

    if ( handle->drainCounter ) {
      handle->drainCounter++;
      goto unlock;
    }
  }

  if ( stream_.mode == INPUT || stream_.mode == DUPLEX ) {

    bufferBytes = stream_.bufferSize * formatBytes(stream_.deviceFormat[1]);

    if (stream_.doConvertBuffer[1]) {

      // Always interleave ASIO input data.
      for ( i=0, j=0; i<nChannels; i++ ) {
        if ( handle->bufferInfos[i].isInput == ASIOTrue )
          memcpy( &stream_.deviceBuffer[j++*bufferBytes],
                  handle->bufferInfos[i].buffers[bufferIndex],
                  bufferBytes );
      }

      if ( stream_.doByteSwap[1] )
        byteSwapBuffer( stream_.deviceBuffer,
                        stream_.bufferSize * stream_.nDeviceChannels[1],
                        stream_.deviceFormat[1] );
      convertBuffer( stream_.userBuffer[1], stream_.deviceBuffer, stream_.convertInfo[1] );

    }
    else {
      for ( i=0, j=0; i<nChannels; i++ ) {
        if ( handle->bufferInfos[i].isInput == ASIOTrue ) {
          memcpy( &stream_.userBuffer[1][bufferBytes*j++],
                  handle->bufferInfos[i].buffers[bufferIndex],
                  bufferBytes );
        }
      }

      if ( stream_.doByteSwap[1] )
        byteSwapBuffer( stream_.userBuffer[1],
                        stream_.bufferSize * stream_.nUserChannels[1],
                        stream_.userFormat );
    }
  }

 unlock:
  // The following call was suggested by Malte Clasen.  While the API
  // documentation indicates it should not be required, some device
  // drivers apparently do not function correctly without it.
  ASIOOutputReady();

  //  MUTEX_UNLOCK( &stream_.mutex );

  RtApi::tickStreamTime();
  return SUCCESS;
}

void sampleRateChanged( ASIOSampleRate sRate )
{
  // The ASIO documentation says that this usually only happens during
  // external sync.  Audio processing is not stopped by the driver,
  // actual sample rate might not have even changed, maybe only the
  // sample rate status of an AES/EBU or S/PDIF digital input at the
  // audio device.

  RtApi *object = (RtApi *) asioCallbackInfo->object;
  try {
    object->stopStream();
  }
  catch ( RtError &exception ) {
    std::cerr << "\nRtApiAsio: sampleRateChanged() error (" << exception.getMessage() << ")!\n" << std::endl;
    return;
  }

  std::cerr << "\nRtApiAsio: driver reports sample rate changed to " << sRate << " ... stream stopped!!!\n" << std::endl;
}

long asioMessages( long selector, long value, void* message, double* opt )
{
  long ret = 0;

  switch( selector ) {
  case kAsioSelectorSupported:
    if ( value == kAsioResetRequest
         || value == kAsioEngineVersion
         || value == kAsioResyncRequest
         || value == kAsioLatenciesChanged
         // The following three were added for ASIO 2.0, you don't
         // necessarily have to support them.
         || value == kAsioSupportsTimeInfo
         || value == kAsioSupportsTimeCode
         || value == kAsioSupportsInputMonitor)
      ret = 1L;
    break;
  case kAsioResetRequest:
    // Defer the task and perform the reset of the driver during the
    // next "safe" situation.  You cannot reset the driver right now,
    // as this code is called from the driver.  Reset the driver is
    // done by completely destruct is. I.e. ASIOStop(),
    // ASIODisposeBuffers(), Destruction Afterwards you initialize the
    // driver again.
    std::cerr << "\nRtApiAsio: driver reset requested!!!" << std::endl;
    ret = 1L;
    break;
  case kAsioResyncRequest:
    // This informs the application that the driver encountered some
    // non-fatal data loss.  It is used for synchronization purposes
    // of different media.  Added mainly to work around the Win16Mutex
    // problems in Windows 95/98 with the Windows Multimedia system,
    // which could lose data because the Mutex was held too long by
    // another thread.  However a driver can issue it in other
    // situations, too.
    // std::cerr << "\nRtApiAsio: driver resync requested!!!" << std::endl;
    asioXRun = true;
    ret = 1L;
    break;
  case kAsioLatenciesChanged:
    // This will inform the host application that the drivers were
    // latencies changed.  Beware, it this does not mean that the
    // buffer sizes have changed!  You might need to update internal
    // delay data.
    std::cerr << "\nRtApiAsio: driver latency may have changed!!!" << std::endl;
    ret = 1L;
    break;
  case kAsioEngineVersion:
    // Return the supported ASIO version of the host application.  If
    // a host application does not implement this selector, ASIO 1.0
    // is assumed by the driver.
    ret = 2L;
    break;
  case kAsioSupportsTimeInfo:
    // Informs the driver whether the
    // asioCallbacks.bufferSwitchTimeInfo() callback is supported.
    // For compatibility with ASIO 1.0 drivers the host application
    // should always support the "old" bufferSwitch method, too.
    ret = 0;
    break;
  case kAsioSupportsTimeCode:
    // Informs the driver whether application is interested in time
    // code info.  If an application does not need to know about time
    // code, the driver has less work to do.
    ret = 0;
    break;
  }
  return ret;
}

static const char* getAsioErrorString( ASIOError result )
{
  struct Messages
  {
    ASIOError value;
    const char*message;
  };

  static Messages m[] =
    {
      {   ASE_NotPresent,    "Hardware input or output is not present or available." },
      {   ASE_HWMalfunction,  "Hardware is malfunctioning." },
      {   ASE_InvalidParameter, "Invalid input parameter." },
      {   ASE_InvalidMode,      "Invalid mode." },
      {   ASE_SPNotAdvancing,     "Sample position not advancing." },
      {   ASE_NoClock,            "Sample clock or rate cannot be determined or is not present." },
      {   ASE_NoMemory,           "Not enough memory to complete the request." }
    };

  for ( unsigned int i = 0; i < sizeof(m)/sizeof(m[0]); ++i )
    if ( m[i].value == result ) return m[i].message;

  return "Unknown error.";
}
//******************** End of __WINDOWS_ASIO__ *********************//

#endif

