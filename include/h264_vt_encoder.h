// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAST_SENDER_H264_VT_ENCODER_H_
#define MEDIA_CAST_SENDER_H264_VT_ENCODER_H_

#include "base/mac/scoped_cftyperef.h"
//#include "base/threading/thread_checker.h"
#include "videotoolbox_glue.h"
//#include "media/cast/sender/size_adaptable_video_encoder_base.h"
//#include "media/cast/sender/video_encoder.h"
#include "video_frame.h"




#include "h264_interface.h"


namespace webrtc{
struct VideoSenderConfig {
  VideoSenderConfig(){};
  ~VideoSenderConfig(){};

  // Identifier referring to the sender, used by the receiver.
  uint32 ssrc;

  // The receiver's SSRC identifier.
  uint32 receiver_ssrc;

  int rtcp_interval;

  // The total amount of time between a frame's capture/recording on the sender
  // and its playback on the receiver (i.e., shown to a user).  This should be
  // set to a value large enough to give the system sufficient time to encode,
  // transmit/retransmit, receive, decode, and render; given its run-time
  // environment (sender/receiver hardware performance, network conditions,
  // etc.).
  //base::TimeDelta min_playout_delay;
  //base::TimeDelta max_playout_delay;

  // RTP payload type enum: Specifies the type/encoding of frame data.
  int rtp_payload_type;

  bool use_external_encoder;

  float congestion_control_back_off;
  int max_bitrate;
  int min_bitrate;
  int start_bitrate;
  int max_qp;
  int min_qp;
  int max_frame_rate;  // TODO(miu): Should be double, not int.

  // This field is used differently by various encoders. It defaults to 1.
  //
  // For VP8, it should be 1 to operate in single-buffer mode, or 3 to operate
  // in multi-buffer mode. See
  // http://www.webmproject.org/docs/encoder-parameters/ for details.
  //
  // For H.264 on Mac or iOS, it controls the max number of frames the encoder
  // may hold before emitting a frame. A larger window may allow higher encoding
  // efficiency at the cost of latency and memory. Set to 0 to let the encoder
  // choose a suitable value for the platform and other encoding settings.
  int max_number_of_video_buffers_used;

  //Codec codec;
  int codec;

  int number_of_encode_threads;
 // The AES crypto key and initialization vector.  Each of these strings
  // contains the data in binary form, of size kAesKeySize.  If they are empty
  // strings, crypto is not being used.
  //std::string aes_key;
  //std::string aes_iv_mask;
};






// VideoToolbox implementation of the media::cast::VideoEncoder interface.
// VideoToolbox makes no guarantees that it is thread safe, so this object is
// pinned to the thread on which it is constructed.
class H264VideoToolboxEncoder {
  typedef CoreMediaGlue::CMSampleBufferRef CMSampleBufferRef;
  typedef VideoToolboxGlue::VTCompressionSessionRef VTCompressionSessionRef;
  typedef VideoToolboxGlue::VTEncodeInfoFlags VTEncodeInfoFlags;

 public:

  H264VideoToolboxEncoder();
      //const scoped_refptr<CastEnvironment>& cast_environment,
  ~H264VideoToolboxEncoder() ;

#if 0
  // Returns true if the current platform and system configuration supports
  // using H264VideoToolboxEncoder with the given |video_config|.
  //static bool IsSupported(const VideoSenderConfig& video_config);
  static bool IsSupported(int codec);

  void SetBitRate(int new_bit_rate) override;
  void LatestFrameIdToReference(uint32 frame_id) override;
  scoped_ptr<VideoFrameFactory> CreateVideoFrameFactory() override;
  void EmitFrames() override;
#endif

  EncodedImageCallback* encoded_complete_callback_;
  CVPixelBufferPoolRef pool;
  void GenerateKeyFrame() ;
  // media::cast::VideoEncoder implementation
  bool EncodeVideoFrame(
      const I420VideoFrame& video_frame
      //const base::TimeTicks& reference_time,
//      const FrameEncodedCallback& frame_encoded_callback
  ) ;



  CVPixelBufferRef WrapVideoFrameInCVPixelBuffer(const I420VideoFrame& frame) ;
//base::ScopedCFTypeRef<CVPixelBufferRef> WrapVideoFrameInCVPixelBuffer(const I420VideoFrame& frame) ;
 



  // Initialize the compression session.
  bool Initialize(const VideoSenderConfig& video_config,int width,int height);

  // Configure the compression session.
  void ConfigureSession(const VideoSenderConfig& video_config);

  // Teardown the encoder.
  void Teardown();

  void SetEncodedCompleteCallback(EncodedImageCallback* cb);

  // Set a compression session property.
  bool SetSessionProperty(CFStringRef key, int32_t value);
  bool SetSessionProperty(CFStringRef key, bool value);
  bool SetSessionProperty(CFStringRef key, CFStringRef value);


 private:
  // Compression session callback function to handle compressed frames.
  static void CompressionCallback(void* encoder_opaque,
                                  void* request_opaque,
                                  OSStatus status,
                                  VTEncodeInfoFlags info,
                                  CMSampleBufferRef sbuf);

  // The cast environment (contains worker threads & more).
  //const scoped_refptr<CastEnvironment> cast_environment_;

  // VideoToolboxGlue provides access to VideoToolbox at runtime.
  const VideoToolboxGlue* const videotoolbox_glue_;

  // The size of the visible region of the video frames to be encoded.
  //const gfx::Size frame_size_;

  // Callback used to report initialization status and runtime errors.
  //const StatusChangeCallback status_change_cb_;

  // Thread checker to enforce that this object is used on a specific thread.
  //base::ThreadChecker thread_checker_;

  // The compression session.
  base::ScopedCFTypeRef<VTCompressionSessionRef> compression_session_;

  // Frame identifier counter.
  uint32 next_frame_id_;

  // Force next frame to be a keyframe.
  bool encode_next_frame_as_keyframe_;


  DISALLOW_COPY_AND_ASSIGN(H264VideoToolboxEncoder);
};

#if 0
// An implementation of SizeAdaptableVideoEncoderBase to proxy for
// H264VideoToolboxEncoder instances.
class SizeAdaptableH264VideoToolboxVideoEncoder
 : public SizeAdaptableVideoEncoderBase {
 public:
  SizeAdaptableH264VideoToolboxVideoEncoder(
      const scoped_refptr<CastEnvironment>& cast_environment,
      const VideoSenderConfig& video_config,
      const StatusChangeCallback& status_change_cb);

  ~SizeAdaptableH264VideoToolboxVideoEncoder() override;

  scoped_ptr<VideoFrameFactory> CreateVideoFrameFactory() override;

 protected:
  scoped_ptr<VideoEncoder> CreateEncoder() override;
  void OnEncoderReplaced(VideoEncoder* replacement_encoder) override;
  void DestroyEncoder() override;

 private:
  struct FactoryHolder;
  class VideoFrameFactoryProxy;

  const scoped_refptr<FactoryHolder> holder_;

  DISALLOW_COPY_AND_ASSIGN(SizeAdaptableH264VideoToolboxVideoEncoder);
};
#endif

}  // namespace webrtc

#endif  // MEDIA_CAST_SENDER_H264_VT_ENCODER_H_
