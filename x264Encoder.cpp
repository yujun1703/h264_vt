#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "x264Encoder.h"
#ifdef WIN32
//#include "stdint_h264.h"
#else
#include <unistd.h>
#endif
#include "x264.h"

#ifdef WIN32
//#pragma comment(lib, "F:\\trunk64_2\\webrtc\\src\\webrtc\\modules\\video_coding\\codecs\\h264\\libx264.dll.a")
//#pragma comment(lib, "..\\..\\..\\..\\..\\src\\webrtc\\modules\\video_coding\\codecs\\h264\\libx264.dll.a")
//#pragma comment(lib, "..\\webrtc\\modules\\video_coding\\codecs\\h264\\libx264.dll.a")
#endif
#include "video_frame.h"
//#define MY_JNI

#ifdef ANDROID
#include <jni.h>
#include <android/log.h>
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, "libnav",__VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG  , "libnav",__VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO   , "libnav",__VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN   , "libnav",__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR  , "libnav",__VA_ARGS__)
#define EXPORT 
#else
#define LOGE  printf
#ifdef WIN32
#define EXPORT __declspec(dllexport)
#endif
#endif

#define OUTBUF_SIZE		(512 * 1024)  // 512KB
//#define LOG fprintf
#define CLIP3(a, min, max) (a) > (max) ? (max) : (a) < (min) ? (min) : (a)


int x264_cpu_num_processors(void);

//fred temp for cmd param
#define ENCODER_DEBUG_OPT   0   //be carefull!!
namespace webrtc
{
	typedef struct user_param_t
	{
		int         b_rc_vbv;
		float       vbv_buffer_percent;
		float       vbv_drop_thr;
		int         max_drop_num;
		int         b_mbrc_strict;
		int         drop_start_pos;
		int         b_adaptive_qp;

		int         cqp_qp;
	} user_param;

	CX264Encoder::CX264Encoder() :
		m_px264Handle(NULL),
		m_pPic(NULL)
		//m_pcOut(NULL)
	{
	}

	CX264Encoder::~CX264Encoder()
	{
		UnInit();
	}

	HRESULT CX264Encoder::Init(X264ENCPARAM param)
	{
		// memset(m_level, (unsigned char)tp_lvl_2, sizeof(m_level));
		fprintf(stderr, "x264 [error]: x264_encoder_open \n");
		m_stEncParam = param;

		x264_param_t st264Param;
		// memset(&st264Param, 0, sizeof(st264Param));
		ConfigParam(&st264Param);

		/*
		x264_param_default( &st264Param );
		st264Param.i_width = 352;
		st264Param.i_height = 288;
		st264Param.rc.i_rc_method = X264_RC_ABR;
		st264Param.rc.i_bitrate = 200;
		st264Param.i_fps_num = 15;
		st264Param.i_fps_den = 1;
		st264Param.b_vfr_input = 0;
		*/

		x264_t *h;
		if ((h = x264_encoder_open(&st264Param)) == NULL)
		{
			fprintf(stderr, "x264 []: x264_encoder_open failed\n");
			return false;
		}

		x264_encoder_parameters(h, &st264Param);
		m_px264Handle = (void*)h;

		x264_picture_t *pic = (x264_picture_t*)malloc(sizeof(x264_picture_t));

		memset(pic, 0, sizeof(x264_picture_t));
		pic->i_type = X264_TYPE_AUTO;
		pic->i_qpplus1 = X264_QP_AUTO;
		pic->i_pic_struct = X264_CSP_I420;

		pic->img.i_csp = X264_CSP_I420;
		pic->img.i_plane = 3;
		pic->img.i_stride[0] = m_stEncParam.iWidth;
		pic->img.i_stride[1] = pic->img.i_stride[2] = m_stEncParam.iWidth >> 1;

		m_pPic = (void*)pic;
		return true;
	}

	HRESULT CX264Encoder::UnInit()
	{
		fprintf(stderr, "x264 []: x264_encoder_close \n");

		if (m_px264Handle != NULL)
			x264_encoder_close((x264_t*)m_px264Handle);

		m_px264Handle = NULL;

		if (m_pPic != NULL)
			free((x264_picture_t*)m_pPic);

		m_pPic = NULL;
		return true;
	}

	HRESULT CX264Encoder::Encode(const I420VideoFrame& input_image, x264_nal_t **nal,int* i_nal, VideoFrameType VideoFrameType)
	{
		x264_picture_t pic_out;		
		//x264_nal_t *nal;
		int i_frame_size = 0;
//		fprintf(stderr, "x264 []: x264_encoder_encode \n");

		uint8_t* pInY = (uint8_t*)input_image.buffer(kYPlane);
		uint8_t* pInU = (uint8_t*)input_image.buffer(kUPlane);
		uint8_t* pInV = (uint8_t*)input_image.buffer(kVPlane);

		uint32_t rawY_stride = input_image.stride(kYPlane);
		uint32_t rawUV_stride = input_image.stride(kUPlane);

		if (!pInY )
		{
			return E_INVALIDARG;
		}

		/*
		int nPixelSize = m_stEncParam.iWidth * m_stEncParam.iHeight;
		int nFrameSize = nPixelSize * 3 / 2;
		if (nInLen < nFrameSize)
		{
			return E_INVALIDARG;
		}
		*/

		((x264_picture_t*)m_pPic)->img.plane[0] = (VQQUCHAR*)pInY;	// pIn;
		((x264_picture_t*)m_pPic)->img.plane[1] = pInU;	// pIn + nPixelSize;
		((x264_picture_t*)m_pPic)->img.plane[2] = pInV;	// pIn + nPixelSize * 5 / 4;

		((x264_picture_t*)m_pPic)->img.i_stride[0] = rawY_stride;
		((x264_picture_t*)m_pPic)->img.i_stride[1] = rawUV_stride;
		((x264_picture_t*)m_pPic)->img.i_stride[2] = rawUV_stride;
		((x264_picture_t*)m_pPic)->i_type = (VideoFrameType == kKeyFrame) ? X264_TYPE_IDR : X264_TYPE_AUTO;
		/*
		if ( 0 != SetFrameRefInfo( (x264_picture_t*)m_pPic, FrameType ) )
		{
		fprintf(stderr, "x264 [error]: Frame Type unsupport\n");
		return false;
		}
		*/

		//LOGE("encoding a frame!!");

		if ((i_frame_size = x264_encoder_encode((x264_t*)m_px264Handle, nal, i_nal, (x264_picture_t*)m_pPic, &pic_out)) < 0)
		{
			fprintf(stderr, "x264 [error]: x264_encoder_encode failed\n");
			return i_frame_size;
		}

		/*
		if (i_frame_size > 0)
		{
			*ppOut = nal[0].p_payload;
			UpdateRefStatus(VideoFrameType);
		}
		else if (!i_frame_size)
			fprintf("drop 1 frame\n");


		*pOutLen = i_frame_size;
		*/
		return i_frame_size;
	}

	void CX264Encoder::ResetEncoder(int width, int height)
	{
		m_stEncParam.iWidth = width;
		m_stEncParam.iHeight = height;
		((x264_picture_t*)m_pPic)->img.i_stride[0] = m_stEncParam.iWidth;
		((x264_picture_t*)m_pPic)->img.i_stride[1] = ((x264_picture_t*)m_pPic)->img.i_stride[2] = m_stEncParam.iWidth >> 1;

		//devinyu
		// x264_encoder_reset_esolution((x264_t*)m_px264Handle, width, height);
	}

	int CX264Encoder::SetFrameRefInfo(void* ptr, VideoFrameType FrameType)
	{
		int index = (int)FrameType;
		/*
		temporal_levle_idc ref_frame_level[ 5 ] =
		{
		tp_lvl_2,	//idr
		tp_lvl_2,	//gf
		tp_lvl_1,	//sp
		tp_lvl_0,	//pwithsp
		tp_lvl_1_p	//pnosp
		};*/

		int x264_frame_type[] =
		{
			X264_TYPE_IDR,
			X264_TYPE_P,
			X264_TYPE_P,
			X264_TYPE_P,
			X264_TYPE_P
		};

		if (ptr)
		{
			((x264_picture_t*)ptr)->i_type = x264_frame_type[index];
			//  ((x264_picture_t*)ptr)->curr_level = ref_frame_level[ index ];
			//   ((x264_picture_t*)ptr)->ref_level  = (temporal_levle_idc)m_level[ ref_frame_level[ index ] ];
		}
		return 0;

	}

	void CX264Encoder::UpdateRefStatus(VideoFrameType FrameType)
	{
	//	int index = (int)FrameType;
		/*
		temporal_levle_idc ref_frame_level[ 5 ] =
		{
		tp_lvl_2,	//idr
		tp_lvl_2,	//gf
		tp_lvl_1,	//sp
		tp_lvl_0,	//pwithsp
		tp_lvl_1_p	//pnosp
		};

		//pnosp
		if ( emType_PnoSP == FrameType )
		{
		m_level[tp_lvl_1_p] = tp_lvl_1_p;
		}
		else
		{
		for ( int i = 0; i <= (int)ref_frame_level[index] && 0 != (int)ref_frame_level[index]; i++ )
		{
		m_level[i] = ref_frame_level[index];

		m_level[tp_lvl_1_p] = ref_frame_level[index];
		//fred
		//虽然每个参考等级的帧都更新 p_nosp 的参考类型，但是最终起作用的是I帧和gf帧
		}
		}*/
	}

	void CX264Encoder::SetPeriod(int GOP, int gfGOP, int spGOP)
	{
		if (!m_px264Handle)
			return;

		//x264_encoder_set_period( (x264_t*)m_px264Handle, GOP, gfGOP, spGOP );
	}

	void CX264Encoder::SetDelayTime(int sec)
	{
		if (!m_px264Handle)
			return;

		//  x264_encoder_set_delaytime( (x264_t*)m_px264Handle, sec );
	}
#if 1
	HRESULT CX264Encoder::SetParam(int nBitrate, int nFPS)
	{
		if(!m_px264Handle)
			return false;
		if ( nBitrate <= 0 || nFPS <= 0 )
		{
			fprintf(stderr, "ChangeParam(): invalid value\n");
			return E_INVALIDARG;
		}

		if ( nBitrate == m_stEncParam.iBitrate 
			&& nFPS == m_stEncParam.iFPS )
		{
			return S_FALSE;
		}    

		//x264_t *h = (x264_t*)m_px264Handle;
		//x264_picture_t *pic = (x264_picture_t*)m_pPic;

		m_stEncParam.iBitrate	= nBitrate;
		m_stEncParam.iFPS		= nFPS;

		x264_param_t st264Param;
		ConfigParam( &st264Param );

		if ( x264_encoder_reconfig( (x264_t*)m_px264Handle, &st264Param ) < 0 )
		{
			return false;
		}
		return true;
	}
#endif

	void CX264Encoder::ResetRCBuffer()
	{
		if (!m_px264Handle)
			return;
		//x264_encoder_reset_rc_buffer( (x264_t*)m_px264Handle );
	}

/*
	static void x264_log_default(void *p_unused, int i_level, const char *psz_fmt, va_list arg)
	{
		char *psz_prefix;
		char buf[1000] = { 0 };
		switch (i_level)
		{
		case X264_LOG_ERROR:
			psz_prefix = "error";
			break;
		case X264_LOG_WARNING:
			psz_prefix = "warning";
			break;
		case X264_LOG_INFO:
			psz_prefix = "info";
			break;
		case X264_LOG_DEBUG:
			psz_prefix = "debug";
			break;
		default:
			psz_prefix = "unknown";
			break;
		}
		//vsfprintf(buf, psz_fmt, arg);
		// LOGD("%s", buf);
		//  ffprintf( stderr, "x264 [%s]: ", psz_prefix );
		//   vffprintf( stderr, psz_fmt, arg );
	}
*/

	void CX264Encoder::ConfigParam(void* param)
	{
		if (!param)
			return;

		x264_param_t* pParam = (x264_param_t*)param;
		x264_param_default((x264_param_t*)pParam);
		pParam->i_width = m_stEncParam.iWidth;
		pParam->i_height = m_stEncParam.iHeight;
		pParam->rc.i_bitrate = m_stEncParam.iBitrate;
		pParam->rc.i_rc_method = X264_RC_ABR;
		pParam->i_fps_den = 1;
		pParam->i_fps_num = m_stEncParam.iFPS * pParam->i_fps_den;
		pParam->i_frame_reference = 1;
		pParam->analyse.b_psnr = 0;
		pParam->analyse.b_ssim = 0;
		pParam->b_annexb = 1;
		pParam->analyse.b_weighted_bipred = 0;
		pParam->analyse.b_psy = 0;
		
		#ifdef _WIN32
		pParam->i_threads = CLIP3(x264_cpu_num_processors(), 1,  3);
		#else
		pParam->i_threads = CLIP3(sysconf( _SC_NPROCESSORS_CONF ), 1,  3);
		#endif
		pParam->rc.i_qp_min = CLIP3(m_stEncParam.iMinQP, 0, 51);
		pParam->rc.i_qp_max = CLIP3(m_stEncParam.iMaxQP, 20, 51);
		pParam->b_vfr_input = 0;

		//emMode    
		pParam->i_keyint_max = 250;
		pParam->i_lookahead_threads = 0;

		pParam->i_sync_lookahead = 0;

		pParam->rc.b_mb_tree = 0;
		pParam->i_bframe = 0;
		pParam->i_log_level = -1;
		pParam->analyse.i_weighted_pred = X264_WEIGHTP_NONE;
		pParam->rc.f_ip_factor = 0.8;
#ifdef ANDROID
		if(sysconf( _SC_NPROCESSORS_CONF ) > 4)
		{
			m_stEncParam.cp = X264ENCPARAM::cp_best;
		}
#endif

#ifdef __APPLE__
		if(sizeof( long ) == 8)
		{
			m_stEncParam.cp = X264ENCPARAM::cp_best;
		}
#endif


		/*  先回退回abr  */
#if 0
		//pParam->i_threads = m_stEncParam.iThreads;//CLIP3(m_stEncParam.iThreads, 1,  16);
		pParam->rc.i_rc_method = X264_RC_LAMBDA;
		pParam->rc.i_aq_mode = 3;
		//pParam->rc.i_qp_constant = m_stEncParam.iQP;
		pParam->rc.b_mb_tree = 0;
		pParam->analyse.b_psnr = 0;		//===输出PSNR===//	      
		pParam->analyse.b_ssim = 0;    	//===输出SSIM===//	
		//pParam->i_frame_total = m_stEncParam.itotal;
		pParam->rc.i_bitrate = m_stEncParam.iBitrate;
#endif

#ifdef WIN32
		pParam->i_threads = 1;
		//complexity + -标识相对于normal的变化
		switch (m_stEncParam.cp)
		{
		case X264ENCPARAM::cp_best:

			pParam->analyse.i_mv_range = -1;
			pParam->analyse.b_mixed_references = 1;
			pParam->b_cabac = 1;
			pParam->analyse.b_transform_8x8 = 1;
			pParam->analyse.i_me_method = X264_ME_HEX;
			pParam->analyse.inter = X264_ANALYSE_I4x4 | X264_ANALYSE_PSUB16x16 | X264_ANALYSE_BSUB16x16 | X264_ANALYSE_PSUB8x8; //+
			pParam->analyse.intra = X264_ANALYSE_I4x4 | X264_ANALYSE_I8x8;
			pParam->analyse.i_subpel_refine = 4;
			pParam->analyse.i_me_range = 24;
			pParam->i_frame_reference = 1;
			pParam->i_bframe_adaptive = X264_B_ADAPT_TRELLIS;
			pParam->analyse.i_direct_mv_pred = X264_DIRECT_PRED_AUTO;
			pParam->analyse.inter |= X264_ANALYSE_PSUB8x8;
			pParam->analyse.i_trellis = 2;

			break;

		case X264ENCPARAM::cp_fast:
			pParam->analyse.i_subpel_refine = 1; //-
			pParam->analyse.i_mv_range = 64;//-
			pParam->analyse.b_mixed_references = 0; //-
			pParam->analyse.i_trellis = 0; //- 
			pParam->b_cabac = 1;
			pParam->analyse.b_transform_8x8 = 0; //-
			pParam->analyse.i_me_method = X264_ME_DIA; //-
			pParam->analyse.inter = X264_ANALYSE_I4x4 | X264_ANALYSE_PSUB16x16;
			pParam->analyse.intra = X264_ANALYSE_I4x4; //-        
			//      pParam->analyse.i_cmplx_level	        = 0;
			break;

		case X264ENCPARAM::cp_normal:
		default:
			pParam->analyse.i_subpel_refine = 3;
			pParam->analyse.i_mv_range = -1;
			pParam->analyse.b_mixed_references = 1;
			pParam->analyse.i_trellis = 1;
			pParam->b_cabac = 1;
			pParam->analyse.b_transform_8x8 = 1;
			pParam->analyse.i_me_method = X264_ME_HEX;
			pParam->analyse.inter = X264_ANALYSE_I4x4 | X264_ANALYSE_PSUB16x16 | X264_ANALYSE_BSUB16x16;
			pParam->analyse.intra = X264_ANALYSE_I4x4 | X264_ANALYSE_I8x8;
			//     pParam->analyse.i_cmplx_level	        = 1;
			break;
		}

#else
		//complexity + -标识相对于normal的变化
		switch (m_stEncParam.cp)
		{
		case X264ENCPARAM::cp_best:
			pParam->analyse.i_subpel_refine = 3;
			pParam->analyse.b_mixed_references = 1;
			pParam->b_cabac = 1;
			pParam->analyse.i_me_method = X264_ME_DIA;
			pParam->analyse.inter = X264_ANALYSE_I4x4 | X264_ANALYSE_PSUB16x16;
			pParam->analyse.intra = X264_ANALYSE_I4x4;
		    pParam->analyse.i_trellis               = 0;
            pParam->analyse.b_transform_8x8			= 1;
            pParam->i_frame_reference = 1;
			break;

		case X264ENCPARAM::cp_fast:
			pParam->analyse.i_subpel_refine = 1; //-
			pParam->b_cabac = 1;
			pParam->analyse.i_me_method = X264_ME_DIA; //-
			pParam->analyse.inter = X264_ANALYSE_I4x4 | X264_ANALYSE_PSUB16x16;
			pParam->analyse.intra = X264_ANALYSE_I4x4;
			pParam->analyse.b_transform_8x8			= 0;
			pParam->analyse.b_mixed_references      = 0; //-
            pParam->analyse.i_trellis               = 0; //-
            pParam->i_frame_reference = 1;
		
			break;

		case X264ENCPARAM::cp_normal:
		default:
			pParam->analyse.i_subpel_refine = 2; //-
			pParam->b_cabac = 1;
			pParam->analyse.i_me_method = X264_ME_DIA; //-
			pParam->analyse.inter = X264_ANALYSE_I4x4 | X264_ANALYSE_PSUB16x16;
			pParam->analyse.intra = X264_ANALYSE_I4x4;
			pParam->analyse.b_transform_8x8			= 0;
			pParam->analyse.b_mixed_references      = 0; //-
            pParam->analyse.i_trellis               = 0; //-
            pParam->i_frame_reference = 1;
				
			break;
		}
#endif   
		//profile, force to conform with H.264 standard
		switch (m_stEncParam.eProfileLevel)
		{
		case X264ENCPARAM::emProfileLevel_High:
			break;

		case X264ENCPARAM::emProfileLevel_Main:
		//	pParam->analyse.b_transform_8x8 = 0;
			break;

		case X264ENCPARAM::emProfileLevel_Base:
			pParam->analyse.b_transform_8x8 = 0;
			pParam->b_cabac = 0;
			break;

		default:
			break;
		}
#ifdef WIN32
		//桌面模式I帧开I8x8，反正新码控对桌面模式又没啥用 kevin 2013.11.27
		if (X264ENCPARAM::emMode_Dsktop == m_stEncParam.mode)
		{
			pParam->analyse.intra |= X264_ANALYSE_I8x8;
		}

		if (X264ENCPARAM::emMode_Dsktop == m_stEncParam.mode)
		{
			//桌面模式关键在于I帧的质量，I帧开启I8x8可以提升压缩3%，关掉trellis不影响I帧的质量，但是可以提升整体压缩性能50%
			pParam->analyse.i_trellis = 0;
			pParam->analyse.intra |= X264_ANALYSE_I8x8;
		}
#endif


	}
	//added by fred
	HRESULT CX264Encoder::GetFrameAvgQP(float* pQP)
	{
		HRESULT ret = S_OK;

		if (NULL == pQP)
		{
			ret = false;
		}

		// *pQP = get_avg_qp( (x264_t*)m_px264Handle );

		return ret;
	}

	/*
	extern int release_enc_com_data_core();
	extern int init_enc_com_data_core();


	//
	int CX264Encoder::release_enc_com_data()
	{
		return release_enc_com_data_core();
	}
	
	//
	int CX264Encoder::init_enc_com_data()
	{
		return init_enc_com_data_core();
	}*/
}
