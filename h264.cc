/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 *
 * This file contains the x264 wrapper implementation
 *
 */
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <stdint.h>

#include "trace.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "module_common_types.h"
#include "reference_picture_selection.h"
//#include "temporal_layers.h"
#include "tick_util.h"
#include "vpx/vpx_encoder.h"
#include "vpx/vpx_decoder.h"
#include "vpx/vp8cx.h"
#include "vpx/vp8dx.h"
#include "h264_encoder_decoder.h"

//#define CHECK_H264
#ifndef MAC_IPHONE
/*xiongfei 2014 0212*/
//#define CHECK_BITRATE
#endif

#ifdef CHECK_BITRATE
FILE *cbitratefile;
#endif

#ifdef ANDROID 
#include <android/log.h>
#endif



enum { kH264ErrorPropagationTh = 30 };

namespace webrtc
{

  H264Encoder* H264Encoder::Create() {
  return new H264Encoder();
}

H264Encoder::H264Encoder()
    : encoded_image_(),
      encoded_complete_callback_(NULL),
      inited_(false),
      timestamp_(0),
      first_frame_encoded_(false),
      encoder_(NULL),
      nal_info_(NULL),
      config_(NULL),
      frame_num_(0)
{
  memset(&codec_, 0, sizeof(codec_));
  uint32_t seed = static_cast<uint32_t>(TickTime::MillisecondTimestamp());
  srand(seed);
}

H264Encoder::~H264Encoder() {
  Release();
}

int H264Encoder::Release() {
    if (encoded_image_._buffer != NULL) {
        delete [] encoded_image_._buffer;
        encoded_image_._buffer = NULL;
    }
    if (encoder_ != NULL) {
        encoder_->UnInit();
        delete encoder_;
        encoder_ = NULL;
    }
    
    if(config_ != NULL){
        delete config_;
        config_ = NULL;
    }

	if(nal_info_ != NULL){
		free(nal_info_);
        nal_info_ = NULL;
	}
    
    if(raw_ != NULL){
        raw_ = NULL;
    }
    
    inited_ = false;
    return WEBRTC_VIDEO_CODEC_OK;
}

int H264Encoder::SetRates(uint32_t new_bitrate_kbit, uint32_t new_framerate) {
    if (!inited_) {
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
    
    encoder_->SetParam(new_bitrate_kbit, new_framerate);
  
    return WEBRTC_VIDEO_CODEC_OK;
}

int H264Encoder::InitEncode(const VideoCodec* inst,
                           int number_of_cores,
                           uint32_t /*max_payload_size*/) {
    if (inst == NULL) {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }
    if (inst->maxFramerate < 1) {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }
    // allow zero to represent an unspecified maxBitRate
    if (inst->maxBitrate > 0 && inst->startBitrate > inst->maxBitrate) {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }
    if (inst->width < 1 || inst->height < 1) {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }
    if (number_of_cores < 1) {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }

    int retVal = Release();
    if (retVal < 0) {
        return retVal;
    }
    if (encoder_ == NULL) {
        encoder_ = new CX264Encoder;
    }
    if(config_ == NULL) {
        config_ = new X264ENCPARAM;
    }        
	if(nal_info_ == NULL) {
		nal_info_ = (X264NalInfo *)malloc(sizeof(X264NalInfo) * MAX_NAL);
	}

    timestamp_ = 0;

    codec_ = *inst;

	config_->iBitrate = 2000;//gCmd.kbps;
	config_->iFPS = 15;//gCmd.fps;
	config_->iHeight = 240;//gCmd.height;
	config_->iWidth  = 320;//gCmd.width;
	config_->iGOP = 15;//gCmd.gop;
    config_->igfGOP = 0;//gCmd.gfGOP;
    config_->ispGOP = 0;//gCmd.spGOP;
    config_->iMaxQP = 45;//gCmd.rc_max_q;
    config_->iMinQP = 16;//gCmd.rc_min_q;
    config_->cp = X264ENCPARAM::cp_fast;
    config_->eProfileLevel = X264ENCPARAM::emProfileLevel_Base;
//	if(0==strcmp((gCmd.profile),"BP"))
//	{
//		Info.eProfileLevel = X264ENCPARAM::emProfileLevel_Base;
//	}
//	else if(0==strcmp((gCmd.profile),"MP"))
//	{
//		Info.eProfileLevel = X264ENCPARAM::emProfileLevel_Main;
//	}
//	else if(0==strcmp((gCmd.profile),"HP"))
//	{
//		Info.eProfileLevel = X264ENCPARAM::emProfileLevel_High;
//	}
    
    retVal = encoder_->Init(*config_);
    if (retVal != true) {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCoding, -1,
                     "H264Encoder::InitEncode() fails to initialize encoder ret_val %d",
                     retVal);
        encoder_->UnInit();
        delete encoder_;
        delete config_;
        config_ = NULL;
        encoder_ = NULL;
        return WEBRTC_VIDEO_CODEC_ERROR;
    }
    
    if (encoded_image_._buffer != NULL) {
        delete [] encoded_image_._buffer;
    }
    encoded_image_._size = CalcBufferSize(kI420, codec_.width, codec_.height);
    encoded_image_._buffer = new uint8_t[encoded_image_._size];
    encoded_image_._completeFrame = true;
    
    inited_ = true;
    WEBRTC_TRACE(webrtc::kTraceApiCall, webrtc::kTraceVideoCoding, -1,
                 "H264Encoder::InitEncode(width:%d, height:%d, framerate:%d, start_bitrate:%d, max_bitrate:%d)",
                 inst->width, inst->height, inst->maxFramerate, inst->startBitrate, inst->maxBitrate);
    
    return WEBRTC_VIDEO_CODEC_OK;
}
    
EmFrameType H264Encoder::getFrameType(int frameNum,int gop,int gfgop,int spgop)
{
    int inner;
	EmFrameType frame_type;
    
	inner = frameNum%gop;
	if(0==inner)
	{
		frame_type = emType_IDR;
	}
	else
	{
		frame_type = emType_PnoSP;
        
		if(spgop > 0)
		{
			frame_type = emType_PwithSP;
			if(0==inner%spgop)
			{
				frame_type = emType_SP;
			}
		}
		
		if(gfgop > 0)
		{
			if(0==inner%gfgop)
			{
				frame_type = emType_GF;
			}
		}
	}
	return frame_type;
}

int H264Encoder::Encode(const VideoFrame& input_image,
                       const CodecSpecificInfo* codec_specific_info,
                       const VideoFrameType frame_type) {
    long        frame_size;   
    
    if (!inited_) {
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
    if (input_image.Buffer() == NULL) {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }
    if (encoded_complete_callback_ == NULL) {
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
    
    frame_type_ = getFrameType(frame_num_, config_->iGOP, config_->igfGOP, config_->ispGOP);
    
    if (frame_type == kKeyFrame) {
        frame_type_ = emType_IDR;
    }
    
    // Check for change in frame size.
    if (input_image.Width() != codec_.width ||
        input_image.Height() != codec_.height) {
        int ret = UpdateCodecFrameSize(input_image.Width(), input_image.Height());
        if (ret < 0) {
            return ret;
        }
    }
    
    frame_size = (long)config_->iWidth * config_->iHeight * 3 / 2;
    raw_ = input_image.Buffer();
    
    /* Encode */
    if ( true != encoder_->Encode(raw_, frame_size, &buf_enc_, nal_info_, frame_type_) )
    {
        WEBRTC_TRACE(kTraceError, kTraceVideoCoding, 0, "Encode error\n");
        return WEBRTC_VIDEO_CODEC_ERROR;
    }
    
    frame_num_++;
    return GetEncodedFrame(input_image);
}

int H264Encoder::UpdateCodecFrameSize(WebRtc_UWord32 input_image_width,
                                     WebRtc_UWord32 input_image_height) {
  codec_.width = input_image_width;
  codec_.height = input_image_height;

  return WEBRTC_VIDEO_CODEC_OK;
}

int H264Encoder::GetEncodedFrame(const VideoFrame& input_image) {
    int             nalNum = 0;
    uint8_t         *pBuf;
    uint8_t         lenNalHeader = 0;
    int             lenNal = nal_info_[0].i_nal;
	X264NalInfo		*pNalInfo = nal_info_;
    
    VideoFrameType  frame_type_com;
    
    pBuf = buf_enc_;
    while (nalNum < lenNal) {
        if((*pBuf == 0x00) && (*(pBuf+1) == 0x00) && (*(pBuf+2) == 0x00) && (*(pBuf+3) == 0x01))
        {
            lenNalHeader = 4;
        }
        else if((*(pBuf+1) == 0x00) && (*(pBuf+2) == 0x00) && (*(pBuf+3) == 0x01))
        {
            lenNalHeader = 3;
        }
        
        if(frame_type_ == emType_IDR)
            frame_type_com = kKeyFrame;
        else
            frame_type_com = kDeltaFrame;

        encoded_image_._length          = pNalInfo->i_payload - lenNalHeader;
        encoded_image_._frameType       = frame_type_com;
        encoded_image_._timeStamp       = input_image.TimeStamp();
        encoded_image_.capture_time_ms_ = input_image.RenderTimeMs();
        encoded_image_._encodedHeight   = config_->iHeight;
        encoded_image_._encodedWidth    = config_->iWidth;
        
        memcpy(encoded_image_._buffer, (pBuf+lenNalHeader), encoded_image_._length);

        WEBRTC_TRACE(webrtc::kTraceApiCall, webrtc::kTraceVideoCoding, -1,
                     "H264EncoderImpl::GetEncodedFrame() frame_type: %d, length:%d",
                     frame_type_com, encoded_image_._length);
        // call back
        encoded_complete_callback_->Encoded(encoded_image_, NULL, NULL);

		pBuf = pBuf + pNalInfo->i_payload;
        pNalInfo++;
		nalNum++;
    }

    return WEBRTC_VIDEO_CODEC_OK;
}

int H264Encoder::SetChannelParameters(uint32_t /*packet_loss*/, int rtt) {
    
    return WEBRTC_VIDEO_CODEC_OK;
}

int H264Encoder::RegisterEncodeCompleteCallback(
    EncodedImageCallback* callback) {
    encoded_complete_callback_ = callback;
    return WEBRTC_VIDEO_CODEC_OK;
}

H264Decoder* H264Decoder::Create() {
  return new H264Decoder();
}

H264Decoder::H264Decoder()
    : decode_complete_callback_(NULL),
      inited_(false),
      feedback_mode_(false),
      decoder_(NULL),
      last_keyframe_(),
      image_format_(VPX_IMG_FMT_NONE),
      ref_frame_(NULL),
      propagation_cnt_(-1),
      latest_keyframe_complete_(false),
#ifdef CHECK_H264
      fileoutputyuv(NULL),
#endif
      mfqe_enabled_(false) {
  memset(&codec_, 0, sizeof(codec_));
}

H264Decoder::~H264Decoder() {
  inited_ = true; // in order to do the actual release
  Release();
}

int H264Decoder::Reset() {
  if (!inited_) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }
  InitDecode(&codec_, 1);
  propagation_cnt_ = -1;
  latest_keyframe_complete_ = false;
  mfqe_enabled_ = false;
  return WEBRTC_VIDEO_CODEC_OK;
}

int H264Decoder::InitDecode(const VideoCodec* inst, int number_of_cores) {

  if (inst == NULL) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }

  int ret_val = Release();

  if (ret_val < 0 ) {
    return ret_val;
  }

  if (inst->codecType == kVideoCodecH264) {
    feedback_mode_ = inst->codecSpecific.H264.feedbackModeOn;
  }

 

  int ret = ffh264_create_decoder(&decoder_);
  propagation_cnt_ = -1;
  latest_keyframe_complete_ = false;

  inited_ = true;
  width = 640;
  height = 480;

  if(ret == 0)
  return WEBRTC_VIDEO_CODEC_OK;
  return WEBRTC_VIDEO_CODEC_ERROR;
}

int H264Decoder::Decode(const EncodedImage& input_image,
                       bool missing_frames,
                       const RTPFragmentationHeader* fragmentation,
                       const CodecSpecificInfo* codec_specific_info,
                       int64_t /*render_time_ms*/) {       
                    
  if (!inited_) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }
  if (decode_complete_callback_ == NULL) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }
  if (input_image._buffer == NULL && input_image._length > 0) {
    // Reset to avoid requesting key frames too often.
    if (propagation_cnt_ > 0)
      propagation_cnt_ = 0;
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }

  // Restrict error propagation using key frame requests. Disabled when
  // the feedback mode is enabled (RPS).
  // Reset on a key frame refresh.
  if (!feedback_mode_) {
    if (input_image._frameType == kKeyFrame && input_image._completeFrame)
      propagation_cnt_ = -1;
    // Start count on first loss.
    else if ((!input_image._completeFrame || missing_frames) &&
        propagation_cnt_ == -1)
      propagation_cnt_ = 0;
    if (propagation_cnt_ >= 0)
      propagation_cnt_++;
  }

  /*xiongfei modified 2013 0628*/
  if (true == missing_frames)
  {
//	  return WEBRTC_VIDEO_CODEC_ERROR;
  }

  vpx_codec_iter_t iter = NULL;
//  vpx_image_t* img;
  int ret;

  uint8_t* buffer = input_image._buffer;
  if (input_image._length == 0) {
    buffer = NULL; // Triggers full frame concealment.
  }

  

   uint32_t required_size = CalcBufferSize(kI420, width, height);
  decoded_image_.VerifyAndAllocate(required_size);

  uint32_t outlength = decoded_image_.Size();
  uint32_t frame_count;
  ret = ffh264_decode( decoder_, input_image._buffer, input_image._length, 
	  decoded_image_.Buffer(), &outlength,&width, &height, &frame_count);

   switch(ret)
        {
        case dec_success:
            {
                break;
            }
        case dec_padding:
            {
		return WEBRTC_VIDEO_CODEC_OK;
            }
		case dec_mem_shortage:
		{
			width = width*2;
			height = height*2;
			 return WEBRTC_VIDEO_CODEC_OK;
		}

        default:// dec_failed
            {
                return WEBRTC_VIDEO_CODEC_ERROR;
            }
        }
 

  decoded_image_.SetWidth(width);
  decoded_image_.SetHeight(height);
  decoded_image_.SetTimeStamp(input_image._timeStamp);
  decoded_image_.SetLength(outlength);
  ret = decode_complete_callback_->Decoded(decoded_image_);
  if (ret != 0)
    return ret;


  return WEBRTC_VIDEO_CODEC_OK; 
}



int H264Decoder::RegisterDecodeCompleteCallback(
    DecodedImageCallback* callback) {
  decode_complete_callback_ = callback;
  return WEBRTC_VIDEO_CODEC_OK;
}

int H264Decoder::Release() {
  decoded_image_.Free();
  if (last_keyframe_._buffer != NULL) {
    delete [] last_keyframe_._buffer;
    last_keyframe_._buffer = NULL;
  }
  if (decoder_ != NULL) {
	 ffh264_release_decoder(&decoder_);
    decoder_ = NULL;
  }
  if (ref_frame_ != NULL) {
    vpx_img_free(&ref_frame_->img);
    delete ref_frame_;
    ref_frame_ = NULL;
  }
  inited_ = false;

#ifdef CHECK_H264
  if (NULL != fileoutputyuv)
  {
	  fclose(fileoutputyuv);
  }
#endif
  return WEBRTC_VIDEO_CODEC_OK;
}
} // namespace webrtc
