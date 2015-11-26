/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 *
 * This file contains the WEBRTC H264 wrapper implementation
 *
 */

#include "include/h264_impl.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
//#include <vector>

#include "common_video/libyuv/include/webrtc_libyuv.h"
//#include "module_common_types.h"
#include "trace.h"
#include "tick_util.h"
#define MAX_NAL 10

#include "webrtc/common.h"
#include "webrtc/common_video/libyuv/include/webrtc_libyuv.h"
#include "webrtc/modules/interface/module_common_types.h"
#include "webrtc/system_wrappers/interface/tick_util.h"
#include "webrtc/system_wrappers/interface/trace_event.h"

#include "libyuv.h"


namespace webrtc
{
	H264Encoder* H264Encoder::Create() {
		return new H264EncoderImpl();
	}

H264EncoderImpl::H264EncoderImpl() 
{
    encoder_ = NULL;
    config_ = NULL;
	nal_info_ = NULL;
	i_nal = 0;
    frame_type_ = emType_IDR;
    rawY_ = NULL;
	rawU_ = NULL;
	rawV_ = NULL;
    frame_num_ = 0;
	frame_size = 0;
	rawY_stride = 0;
	rawUV_stride = 0;
    buf_enc_ = NULL;
	buf_len_ = 0;  

    first_frame_encoded_ = false;
    inited_ = false;
    timestamp_ = 0;	
        
    encoded_complete_callback_ = NULL;
	memset(&codec_, 0, sizeof(codec_));
	uint32_t seed = static_cast<uint32_t>(TickTime::MillisecondTimestamp());
	srand(seed);

     vtEncoder=new H264VideoToolboxEncoder();
}

H264EncoderImpl::~H264EncoderImpl() {
  Release();
}

int H264EncoderImpl::Release() {
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
	/*
	if(nal_info_ != NULL){
		free(nal_info_);
        nal_info_ = NULL;
	}*/
    
    if(rawY_ != NULL){
        rawY_ = NULL;
    }

	if (rawU_ != NULL){
		rawU_ = NULL;
	} 
	
	if (rawV_ != NULL){
		rawV_ = NULL;
	}

    inited_ = false;
    return WEBRTC_VIDEO_CODEC_OK;
}

int H264EncoderImpl::SetRates(uint32_t new_bitrate_kbit, uint32_t new_framerate) {
    if (!inited_) {
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
    
    encoder_->SetParam(new_bitrate_kbit, new_framerate);
  
    return WEBRTC_VIDEO_CODEC_OK;
}

int H264EncoderImpl::InitEncode(const VideoCodec* inst,
                           int number_of_cores,
						   size_t max_payload_size) {
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

    VideoSenderConfig config;
    config.min_bitrate=inst->minBitrate;
    config.max_bitrate=inst->maxBitrate;
    config.max_frame_rate=25;

    vtEncoder->Initialize(config,inst->width,inst->height);


#if 0
    int retVal = Release();
    if (retVal < 0) {
        return retVal;
    }
    if (encoder_ == NULL) {
        encoder_ = new CX264Encoder();
    }
    if(config_ == NULL) {
        config_ = new X264ENCPARAM();
    }        

	/*
	if(nal_info_ == NULL) {
		nal_info_ = (x264_nal_t *)malloc(sizeof(x264_nal_t) * MAX_NAL);
	}*/

        timestamp_ = 0;
	frame_num_ = 0;

	codec_ = *inst;

	config_->iBitrate = inst->startBitrate;//gCmd.kbps;
	config_->iFPS = inst->maxFramerate;//gCmd.fps;
	config_->iHeight = inst->height;//gCmd.height;
	config_->iWidth  = inst->width;//gCmd.width;
	config_->iGOP = 30;//gCmd.gop;
	config_->igfGOP = 0;//gCmd.gfGOP;
	config_->ispGOP = 0;//gCmd.spGOP;
	config_->iMaxQP = 51;//gCmd.rc_max_q;
	config_->iMinQP = 0;//gCmd.rc_min_q;



	switch (inst->codecSpecific.H264.profile)
	{
#if 0
		case kProfileBase:
			config_->cp = X264ENCPARAM::cp_fast;
			config_->eProfileLevel = X264ENCPARAM::emProfileLevel_Base;
			break;

		case kProfileMain:
			config_->cp = X264ENCPARAM::cp_fast;
			config_->eProfileLevel = X264ENCPARAM::emProfileLevel_Main;
			break;
#endif	
#if 1
		case kComplexityNormal:
		config_->cp = X264ENCPARAM::cp_fast;
		config_->eProfileLevel = X264ENCPARAM::emProfileLevel_Base;
              
		break;

		case kComplexityHigh:
		config_->cp = X264ENCPARAM::cp_fast;
		config_->eProfileLevel = X264ENCPARAM::emProfileLevel_Main;
        
		break;
		case kComplexityHigher:
		config_->cp = X264ENCPARAM::cp_normal;
		config_->eProfileLevel = X264ENCPARAM::emProfileLevel_Main;
        
		break;
		default:
          
		config_->cp = X264ENCPARAM::cp_best;
		config_->eProfileLevel = X264ENCPARAM::emProfileLevel_Main;
		break;
#endif
	}

    
    retVal = encoder_->Init(*config_);
    if (retVal != true) {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCoding, -1,
                     "H264EncoderImpl::InitEncode() fails to initialize encoder ret_val %d",
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
                 "H264EncoderImpl::InitEncode(width:%d, height:%d, framerate:%d, start_bitrate:%d, max_bitrate:%d, cp:%d, profile:%d)",
                 inst->width, inst->height, inst->maxFramerate, inst->startBitrate, inst->maxBitrate, config_->cp, config_->eProfileLevel);
#endif
    
    return WEBRTC_VIDEO_CODEC_OK;
}
    
EmFrameType H264EncoderImpl::getFrameType(int frameNum,int gop,int gfgop,int spgop)
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

/*
enum PlaneType {
	kYPlane = 0,
	kUPlane = 1,
	kVPlane = 2,
	kNumOfPlanes = 3,
};*/




int H264EncoderImpl::Encode(const I420VideoFrame& input_image,
                       const CodecSpecificInfo* codec_specific_info,
					   const std::vector<VideoFrameType>* frame_types) {
  
	//x264_nal_t *nal;

    if (!inited_) {
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
	if (input_image.buffer(kYPlane) == NULL) {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }
    if (encoded_complete_callback_ == NULL) {
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
    

	VideoFrameType frame_type_ = kDeltaFrame;
	// We only support one stream at the moment.
	if (frame_types && frame_types->size() > 0) {
		frame_type_ = (*frame_types)[0];
	}

	if ((frame_num_ < 250) && (frame_num_ % 25 == 0))
	{
		frame_type_ = kKeyFrame;
	}


    // Check for change in frame size.
    if (input_image.width() != codec_.width ||
		input_image.height() != codec_.height) {
		int ret = UpdateCodecFrameSize(input_image.width(), input_image.height());
        if (ret < 0) {
            return ret;
        }
    }
    
    frame_num_++;
    vtEncoder->EncodeVideoFrame(input_image); 
    return 0;
#if 0
    //frame_size = (long)config_->iWidth * config_->iHeight * 3 / 2;
	rawY_ = (uint8_t*)input_image.buffer(kYPlane);
	rawU_ = (uint8_t*)input_image.buffer(kUPlane);
	rawV_ = (uint8_t*)input_image.buffer(kVPlane);

	rawY_stride = input_image.stride(kYPlane);
	rawUV_stride = input_image.stride(kUPlane);

    /* Encode */
	if ((frame_size = encoder_->Encode(input_image, &nal_info_, &i_nal, frame_type_))<0)
    {
        WEBRTC_TRACE(kTraceError, kTraceVideoCoding, 0, "Encode error\n");
        return WEBRTC_VIDEO_CODEC_ERROR;
    }		

	//nal_info_[0].i_payload = buf_len_;
    frame_num_++;
	if (frame_size > 0)
	{
		return GetEncodedPartitions(input_image);
	}
	else
	{
		return 0;
	}
#endif
}


int H264EncoderImpl::UpdateCodecFrameSize(uint32_t input_image_width,
	uint32_t input_image_height) {
	
	codec_.width = input_image_width;
	codec_.height = input_image_height;
	return WEBRTC_VIDEO_CODEC_OK;
}

int H264EncoderImpl::GetEncodedPartitions(const I420VideoFrame& input_image) {
    int             nalNum = 0;
    uint8_t         *pBuf;
    uint8_t         lenNalHeader = 0;

	//x264_nal_t		pNalInfo = nal_info_[0];
    VideoFrameType  frame_type_com;
    
    while (nalNum < i_nal) 
	{
		x264_nal_t  pNalInfo = nal_info_[nalNum];
		//int	 lenNal = nal_info.i_payload;
		pBuf = pNalInfo.p_payload;
        if((*pBuf == 0x00) && (*(pBuf+1) == 0x00) && (*(pBuf+2) == 0x00) && (*(pBuf+3) == 0x01))
        {
            lenNalHeader = 4;
        }
        else if((*pBuf == 0x00) && (*(pBuf+1) == 0x00) && (*(pBuf+2) == 0x01))
        {
            lenNalHeader = 3;
        }
        
        if(frame_type_ == emType_IDR)
            frame_type_com = kKeyFrame;
        else
            frame_type_com = kDeltaFrame;

        encoded_image_._length          = pNalInfo.i_payload - lenNalHeader;
        encoded_image_._frameType       = frame_type_com;
		encoded_image_._timeStamp = input_image.timestamp();
		encoded_image_.capture_time_ms_ = input_image.render_time_ms();
        encoded_image_._encodedHeight   = config_->iHeight;
        encoded_image_._encodedWidth    = config_->iWidth;
        
        memcpy(encoded_image_._buffer, (pBuf+lenNalHeader), encoded_image_._length);

        WEBRTC_TRACE(webrtc::kTraceApiCall, webrtc::kTraceVideoCoding, -1,
                     "H264EncoderImpl::GetEncodedFrame() frame_type: %d, length:%d",
                     frame_type_com, encoded_image_._length);
        // call back
        encoded_complete_callback_->Encoded(encoded_image_, NULL, NULL);

		//pBuf = pBuf + pNalInfo.i_payload;
       // pNalInfo++;
		nalNum++;
    }

    return WEBRTC_VIDEO_CODEC_OK;
}

int32_t H264EncoderImpl::SetChannelParameters(uint32_t packet_loss, int64_t rtt){
    return WEBRTC_VIDEO_CODEC_OK;
}

int H264EncoderImpl::RegisterEncodeCompleteCallback(
    EncodedImageCallback* callback) {
    encoded_complete_callback_ = callback;
    return WEBRTC_VIDEO_CODEC_OK;
}




#if 1
H264Decoder* H264Decoder::Create() {
    return new H264DecoderImpl();
}

H264DecoderImpl::H264DecoderImpl()      
{
    decode_complete_callback_ = NULL;
    inited_ = false;
    decoder_ = NULL;
    buf_dec_ = NULL;
    yuv_dec_ = NULL;
	_rotation = 0;
	_scale =  1.0f;
	_numerator = 1024;
	decoded_image_ = NULL;	
    memset(&codec_, 0, sizeof(codec_));
}

H264DecoderImpl::~H264DecoderImpl() {
  inited_ = true; // in order to do the actual release
  Release();
}

int H264DecoderImpl::Reset() {
    if (!inited_) {
        return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
    }
 //   InitDecode(&codec_, 1);
    return WEBRTC_VIDEO_CODEC_OK;
}

int32_t H264DecoderImpl::RotateDecodedFrame(const int rotation)
{
	_rotation = rotation;
	return 0;
};

int32_t H264DecoderImpl::SetDecodeFrame(float scale)
{
	_scale = scale;
	_numerator = int(_scale * 1024);
	return 0;
};
int H264DecoderImpl::Release() {
    
    if(decoder_ != NULL) {
        ffh264_release_decoder( &decoder_ );
        decoder_ = NULL;
    }
    
    if(buf_dec_ != NULL) {
        delete [] buf_dec_;
        buf_dec_ = NULL;
    }
    
    if(yuv_dec_ != NULL) {
        delete [] yuv_dec_;
        yuv_dec_ = NULL;
    }
	if (decoded_image_) {
		delete decoded_image_;
		decoded_image_ = NULL;
	}
    
    inited_ = false;
    return WEBRTC_VIDEO_CODEC_OK;
}
    
int H264DecoderImpl::InitDecode(const VideoCodec* inst, int number_of_cores) {
    if (inst == NULL) {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }
    int ret_val = Release();
    if (ret_val < 0 ) {
        return ret_val;
    }
    if (decoder_ == NULL) {
        ffh264_create_decoder( &decoder_ );
    }
    
    if(buf_dec_ == NULL) {
        buf_dec_ = new uint8_t [MAX_ENC_SIZE];
    }

	if (decoded_image_ == NULL){
		decoded_image_ = new I420VideoFrame();
	}
    
    if(yuv_dec_ == NULL) {
        yuv_dec_ = new uint8_t [MAX_ENC_WIDTH * MAX_ENC_HEIGHT * 3 / 2];
		yuv_size = MAX_ENC_WIDTH * MAX_ENC_HEIGHT * 3 / 2;
    }
    
    if (&codec_ != inst) {
        // Save VideoCodec instance for later; mainly for duplicating the decoder.
        codec_ = *inst;
    }
    
    if (number_of_cores < 1) {
        return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
    }

    inited_ = true;
    return WEBRTC_VIDEO_CODEC_OK;
}

int H264DecoderImpl::Decode(const EncodedImage& input_image,
							bool missing_frames,
							const RTPFragmentationHeader* fragmentation,
							const CodecSpecificInfo* codec_specific_info,
							int64_t /*render_time_ms*/) 
{
	dec_stats   ret;
	int ht;
	int width, height;
	uint32_t    ret_dec;
	uint32_t    count_frame;	
	if (!inited_) 
	{
		WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCoding, -1,
			"H264DecoderImpl::Decode, decoder is not initialized");
		return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
	}

	if (decode_complete_callback_ == NULL) {
		WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCoding, -1,
			"H264DecoderImpl::Decode, decode complete call back is not set");
		return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
	}

	if (input_image._buffer == NULL) {
		WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCoding, -1,
			"H264DecoderImpl::Decode, null buffer");
		return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
	}
	if (!codec_specific_info) {
		WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCoding, -1,
			"H264EncoderImpl::Decode, no codec info");
		return WEBRTC_VIDEO_CODEC_ERROR;
	}
	if (codec_specific_info->codecType != kVideoCodecH264) {
		WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCoding, -1,
			"H264EncoderImpl::Decode, non h264 codec %d", codec_specific_info->codecType);
		return WEBRTC_VIDEO_CODEC_ERROR;
	}

#ifdef INDEPENDENT_PARTITIONS
	if (fragmentation == NULL) {
		return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
	}
#endif
	//printf("decode packet len------------------------------------- %d\n", input_image._length);
	//static    unsigned char code_prefix[] = {0, 0, 0, 1};
	//    memset(buf_dec_, 0, MAX_ENC_SIZE);

	
	AVFramePic        Picout;
	AVFramePic*        picout = &Picout;
	ret = ffh264_decode_nocopy(decoder_, input_image._buffer, input_image._length,
		picout, &count_frame);
	width = picout->width;
	height = picout->height;
	switch(ret)
	{

	case dec_failed:
		{
			WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCoding, -1,
				"H264EncoderImpl::Decode error");
			return WEBRTC_VIDEO_CODEC_ERROR;
		}

	case dec_padding:
		{
			WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceVideoCoding, -1,
				"H264EncoderImpl::Decode Padding");
			return WEBRTC_VIDEO_CODEC_OK;
		}

	case dec_mem_shortage:
		{
			unsigned int size = yuv_size * 2;
			delete[] yuv_dec_;
			yuv_dec_ = new uint8_t[size];
			yuv_size = size;
			if (decoded_image_)
			{
				delete decoded_image_;
				decoded_image_ = new I420VideoFrame();
			}
			return WEBRTC_VIDEO_CODEC_ERROR;
		}
	case dec_success:
		{
			switch (_rotation)
			{
			case 0:
				ret_dec = 0;
				width = picout->width;
				height = picout->height;
				if (_numerator >= 0){
					height = (height<<10)/_numerator;// / _scale;
					if (height%8)
					height += 8 - height % 8;
					ht = (height - picout->height) >>1;
					decoded_image_->CreateBlackFrame(width, height, width, width / 2, width / 2);
					libyuv::RotatePlane(picout->data[0], picout->linesize[0],
						decoded_image_->buffer(kYPlane) + ht*width, width,
						picout->width, picout->height, libyuv::kRotate0);

					libyuv::RotatePlane(picout->data[1], picout->linesize[1],
						decoded_image_->buffer(kUPlane) + ht / 2 * width / 2, width / 2,
						picout->width / 2, picout->height / 2, libyuv::kRotate0);

					libyuv::RotatePlane(picout->data[2], picout->linesize[2],
						decoded_image_->buffer(kVPlane) + ht / 2 * width / 2, width / 2,
						picout->width / 2, picout->height / 2, libyuv::kRotate0);
				}
				else
				{
					width = (width<<10) /((-1)*_numerator) ;//(width + scale);
					if (width%8)
					width += 8 - width % 8;
					ht = (width - picout->width) >> 1;
					decoded_image_->CreateBlackFrame(width, height, width, width / 2, width / 2);

					libyuv::RotatePlane(picout->data[0], picout->linesize[0],
						decoded_image_->buffer(kYPlane) + ht, width,
						picout->width, picout->height, libyuv::kRotate0);

					libyuv::RotatePlane(picout->data[1], picout->linesize[1],
						decoded_image_->buffer(kUPlane) + ht / 2, width / 2,
						picout->width / 2, picout->height / 2, libyuv::kRotate0);

					libyuv::RotatePlane(picout->data[2], picout->linesize[2],
						decoded_image_->buffer(kVPlane) + ht / 2 , width / 2,
						picout->width / 2, picout->height / 2, libyuv::kRotate0);
				}
				decoded_image_->set_ht_rotation(ht, _numerator);
				decoded_image_->set_timestamp(input_image._timeStamp);
				decoded_image_->set_ntp_time_ms(input_image.ntp_time_ms_);
				ret_dec = decode_complete_callback_->Decoded(*decoded_image_);
				return ret_dec;

			case 90:
				height = picout->width;
				width = picout->height;
				if (_numerator >= 0){
					height = (height<<10) /_numerator;
					if (height%8)
					height += 8- height % 8;
					ht = (height - picout->width) >> 1;
					decoded_image_->CreateBlackFrame(width, height, width, width / 2, width / 2);

					libyuv::RotatePlane90(picout->data[0], picout->linesize[0],
						decoded_image_->buffer(kYPlane) + width*ht, width,
						picout->width, picout->height);

					libyuv::RotatePlane90(picout->data[1], picout->linesize[1],
						decoded_image_->buffer(kUPlane) + (ht / 2)*(width / 2), width / 2,
						picout->width / 2, picout->height / 2);

					libyuv::RotatePlane90(picout->data[2], picout->linesize[2],
						decoded_image_->buffer(kVPlane) + (ht / 2)*(width / 2), width / 2,
						picout->width / 2, picout->height / 2);
				}
				else
				{
					width = (width<<10) /((-1)*_numerator);//(width + scale);
					if (width%8)
					width += 8 - width % 8;
					ht = (width - picout->height) >> 1;
					decoded_image_->CreateBlackFrame(width, height, width, width / 2, width / 2);

					libyuv::RotatePlane90(picout->data[0], picout->linesize[0],
						decoded_image_->buffer(kYPlane) + ht, width,
						picout->width, picout->height);

					libyuv::RotatePlane90(picout->data[1], picout->linesize[1],
						decoded_image_->buffer(kUPlane) + ht / 2, width / 2,
						picout->width / 2, picout->height / 2);

					libyuv::RotatePlane90(picout->data[2], picout->linesize[2],
						decoded_image_->buffer(kVPlane) + ht / 2, width / 2,
						picout->width / 2, picout->height / 2);
				}
				decoded_image_->set_ht_rotation(ht, _numerator);
				decoded_image_->set_timestamp(input_image._timeStamp);
				decoded_image_->set_ntp_time_ms(input_image.ntp_time_ms_);
				ret_dec = decode_complete_callback_->Decoded(*decoded_image_);

				return ret_dec;

			case 180:
				width = picout->width;
				height = picout->height;
				if (_numerator >= 0){
					height = (height<<10 )/_numerator;
					if (height%8)
					height += 8 - height % 8;
					ht = (height - picout->height) >> 1;

					decoded_image_->CreateBlackFrame(width, height, width, width / 2, width / 2);
					libyuv::RotatePlane180(picout->data[0], picout->linesize[0],
						decoded_image_->buffer(kYPlane) + ht*width, width,
						picout->width, picout->height);

					libyuv::RotatePlane180(picout->data[1], picout->linesize[1],
						decoded_image_->buffer(kUPlane) + ht / 2 * width / 2, width / 2,
						picout->width / 2, picout->height / 2);

					libyuv::RotatePlane180(picout->data[2], picout->linesize[2],
						decoded_image_->buffer(kVPlane) + ht / 2 * width / 2, width / 2,
						picout->width / 2, picout->height / 2);
				}
				else
				{
					width = (width<<10)/((-1)*_numerator);//(width + scale);
					if (width%8)
					width += 8 - width % 8;
					ht = (width - picout->width) >> 1;
					decoded_image_->CreateBlackFrame(width, height, width, width / 2, width / 2);

					libyuv::RotatePlane180(picout->data[0], picout->linesize[0],
						decoded_image_->buffer(kYPlane) + ht, width,
						picout->width, picout->height);

					libyuv::RotatePlane180(picout->data[1], picout->linesize[1],
						decoded_image_->buffer(kUPlane) + ht / 2 , width / 2,
						picout->width / 2, picout->height / 2);

					libyuv::RotatePlane180(picout->data[2], picout->linesize[2],
						decoded_image_->buffer(kVPlane) + ht / 2 , width / 2,
						picout->width / 2, picout->height / 2);
				}
				decoded_image_->set_ht_rotation(ht, _numerator);
				decoded_image_->set_timestamp(input_image._timeStamp);
				decoded_image_->set_ntp_time_ms(input_image.ntp_time_ms_);
				ret_dec = decode_complete_callback_->Decoded(*decoded_image_);

				return ret_dec;
				
			case 270:
				height = picout->width;
				width = picout->height;

				if (_numerator >= 0){
					height = (height<<10) /_numerator;
					if (height%8)
					height += 8 - height % 8;
					ht = (height - picout->width) >> 1;

					decoded_image_->CreateBlackFrame(width, height, width, width / 2, width / 2);

					libyuv::RotatePlane270(picout->data[0], picout->linesize[0],
						decoded_image_->buffer(kYPlane) + ht*width, width,
						picout->width, picout->height);

					libyuv::RotatePlane270(picout->data[1], picout->linesize[1],
						decoded_image_->buffer(kUPlane) + ht / 2 * width / 2, width / 2,
						picout->width / 2, picout->height / 2);

					libyuv::RotatePlane270(picout->data[2], picout->linesize[2],
						decoded_image_->buffer(kVPlane) + ht / 2 * width / 2, width / 2,
						picout->width / 2, picout->height / 2);
				}
				else
				{
					width = (width<<10) /((-1)*_numerator);//(width + scale);
					if (width%8)
					width += 8 - width % 8;
					ht = (width - picout->height) >> 1;

					decoded_image_->CreateBlackFrame(width, height, width, width / 2, width / 2);

					libyuv::RotatePlane270(picout->data[0], picout->linesize[0],
						decoded_image_->buffer(kYPlane) + ht, width,
						picout->width, picout->height);

					libyuv::RotatePlane270(picout->data[1], picout->linesize[1],
						decoded_image_->buffer(kUPlane) + ht / 2 , width / 2,
						picout->width / 2, picout->height / 2);

					libyuv::RotatePlane270(picout->data[2], picout->linesize[2],
						decoded_image_->buffer(kVPlane) + ht / 2 , width / 2,
						picout->width / 2, picout->height / 2);
				}
				decoded_image_->set_ht_rotation(ht, _numerator);
				decoded_image_->set_timestamp(input_image._timeStamp);
				decoded_image_->set_ntp_time_ms(input_image.ntp_time_ms_);
				ret_dec = decode_complete_callback_->Decoded(*decoded_image_);

				return ret_dec;

			default:
				break;
			}
			

			if (decoded_image_->width() != (int)width || decoded_image_->height() != (int)height)
			{
				delete decoded_image_;
				decoded_image_ = new I420VideoFrame();
				return WEBRTC_VIDEO_CODEC_OK;
			}
			decoded_image_->set_timestamp(input_image._timeStamp);
			decoded_image_->set_ntp_time_ms(input_image.ntp_time_ms_);
			ret_dec = decode_complete_callback_->Decoded(*decoded_image_);
			if (ret_dec != 0)
				return ret_dec;

			return WEBRTC_VIDEO_CODEC_OK;
		}
	default:
		break;
	}
	return WEBRTC_VIDEO_CODEC_ERROR;
}

int H264DecoderImpl::RegisterDecodeCompleteCallback(
    DecodedImageCallback* callback) {
  decode_complete_callback_ = callback;
  return WEBRTC_VIDEO_CODEC_OK;
}

VideoDecoder* H264DecoderImpl::Copy() {
    // Sanity checks.
    if (!inited_) {
        // Not initialized.
        assert(false);
        return NULL;
    }
	if (decoded_image_->buffer(kYPlane) == NULL) {
        // Nothing has been decoded before; cannot clone.
        return NULL;
    }
    if (last_keyframe_._buffer == NULL) {
        // Cannot clone if we have no key frame to start with.
        return NULL;
    }
    // Create a new VideoDecoder object
    H264DecoderImpl *copy = new H264DecoderImpl;

    // Initialize the new decoder
    if (copy->InitDecode(&codec_, 1) != WEBRTC_VIDEO_CODEC_OK) {
        delete copy;
        return NULL;
    }

    return static_cast<VideoDecoder*>(copy);
}
#endif

} // namespace webrtc
