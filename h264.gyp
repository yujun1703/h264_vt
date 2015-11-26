# Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

{
  'includes': [
    '../../../../build/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'webrtc_h264',
      'type': 'static_library',
      'dependencies': [
        '<(webrtc_root)/common.gyp:webrtc_common',
        '<(webrtc_root)/common_video/common_video.gyp:common_video',
        '<(webrtc_root)/modules/video_coding/utility/video_coding_utility.gyp:video_coding_utility',
        '<(webrtc_root)/system_wrappers/system_wrappers.gyp:system_wrappers',
	#'/Users/gongbikin/yj/trunk64_yj/webrtc/src/chromium/src/base/base_lazy.gyp:base',
      ],

	  #'cflags': [
          #      '-Wnoerror,-Wmacro-redefined',
          #],
      'conditions': [

        ['build_libx264==1', {
         'dependencies': [
            '<(libx264_dir)/libx264.gyp:libx264',
            '<(h264dec_dir)/libh264dec.gyp:libh264dec',
          ],
        }],

        ['build_h264==1', {
	'include_dirs': [
		'include',
		'../../../../',
		'../../../../../',
		'../../../../../../src/chromium/src/third_party/webrtc/system_wrappers/interface/',
		'../../../../../third_party/H264Decoder/h264decoder/',
		'/Users/gongbikin/yj/trunk64_yj/webrtc/src/chromium/src',
#		'/Users/gongbikin/yj/trunk64_yj/webrtc/src/chromium/src/base',
	 ],
          'sources': [
            'include/h264.h',
            'include/x264.h',
            'include/h264_impl.h',
            'include/x264Encoder.h',
            'h264_impl.cc',
	    'x264Encoder.cpp',
	    'coremedia_glue.mm',
	    'videotoolbox_glue.mm',
	    'h264_vt_encoder.cc',

#'coremedia_glue.h',
#'media_export.h',
#'macros.h',
	#	'videotoolbox_glue.mm',
	#	'videotoolbox_glue.h'
          ],
        }
	 ],
      ],

    },
  ],
}
