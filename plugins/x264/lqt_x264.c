#include "lqt_private.h"
#include <quicktime/lqt_codecapi.h>
#include <x264.h> // X264_BUILD value

void quicktime_init_codec_x264(quicktime_video_map_t *vtrack);

static char * fourccs_x264[]  = { "avc1", (char*)0 };

static lqt_parameter_info_static_t encode_parameters_x264[] =
  {
    {
      .name =        "x264_frame_type",
      .real_name =   "Frame-type options",
      .type =        LQT_PARAMETER_SECTION
    },
    {
      .name =        "x264_i_keyint_max",
      .real_name =   "Maximum GOP size",
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 250 },
    },
    {
      .name =        "x264_i_keyint_min",
      .real_name =   "Minimum GOP size",
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 25 },
    },
    {
      .name =        "x264_i_scenecut_threshold",
      .real_name =   "Scenecut threshold",
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 40 },
      .help_string = "How aggressively to insert extra I-frames"
    },
    {
      .name =        "x264_i_bframe",
      .real_name =   "B-Frames",
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 0 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 16 },
      .help_string = "Number of B-frames between I and P",
    },
    {
      .name =        "x264_b_bframe_adaptive",
      .real_name =   "Adaptive B-frame decision",
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 1 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 1 },
    },
    {
      .name =        "x264_i_bframe_bias",
      .real_name =   "B-frame bias",
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 0 },
      .val_min =     { .val_int =  -90 },
      .val_max =     { .val_int = 100 },
      .help_string = "Influences how often B-frames are used",
    },
    {
      .name =        "x264_b_bframe_pyramid",
      .real_name =   "B-frame pyramid",
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 0 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 1 },
      .help_string = "Keep some B-frames as references"
    },
    {
      .name =        "x264_ratecontrol",
      .real_name =   "Ratecontrol",
      .type =        LQT_PARAMETER_SECTION
    },
    {
      .name =        "x264_i_rc_method",
      .real_name =   "Ratecontrol method",
      .type =        LQT_PARAMETER_STRINGLIST,
      .val_default = { .val_string = "Constant quality" },
      .stringlist_options = (char*[]){ "Constant quality",
                                       "Average bitrate",
                                       "CRF based VBR",
                                       (char*)0 },
      .help_string = "Ratecontrol method:\n"
                     "Constant quality: Specify a quantizer parameter below\n"
                     "Average bitrate: Specify a bitrate below\n"
                     "CRF based VBR: Specify a rate factor below\n"
                     "Selecting 2-pass encoding will force Average bitrate.",
    },
    {
      .name =        "x264_i_bitrate",
      .real_name =   "Bitrate",
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 0 },
      .help_string = "Bitrate in kbit/s. 0 means VBR (recommended)"
    },
#if X264_BUILD < 54
    {
      .name =        "x264_i_rf_constant",
      .real_name =   "Nominal Quantizer parameter",
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 26 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 51 },
      .help_string = "This selects the nominal quantizer to use (1 to 51). "
                     "Lower values result in better fidelity, but higher "
                     "bitrates. 26 is a good default value. 0 means lossless."
    },
#else
    {
      .name =        "x264_f_rf_constant",
      .real_name =   "Nominal Quantizer parameter",
      .type =        LQT_PARAMETER_FLOAT,
      .val_default = { .val_float = 26.0 },
      .val_min =     { .val_float = 0.0 },
      .val_max =     { .val_float = 51.0 },
      .help_string = "This selects the nominal quantizer to use (1 to 51). "
                     "Lower values result in better fidelity, but higher "
                     "bitrates. 26 is a good default value. 0 means lossless."
    },
#endif
    {
      .name =        "x264_i_qp_constant",
      .real_name =   "Quantizer parameter",
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 26 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 51 },
      .help_string = "This selects the quantizer to use (1 to 51). Lower "
                     "values result in better fidelity, but higher bitrates. "
                     "26 is a good default value. 0 means lossless."
    },
{
      .name =        "x264_i_qp_min",
      .real_name =   "Minimum quantizer parameter",
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 10 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 51 },
      .help_string = "Minimum quantizer parameter"
    },
    {
      .name =        "x264_i_qp_max",
      .real_name =   "Maximum quantizer parameter",
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 51 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 51 },
      .help_string = "Maximum quantizer parameter"
    },
    {
      .name =        "x264_i_qp_step",
      .real_name =   "Maximum QP step",
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 4 },
      .val_min =     { .val_int = 1 },
      .val_max =     { .val_int = 50 },
      .help_string = "Maximum quantizer step"
    },
    {
      .name =        "x264_f_rate_tolerance",
      .real_name =   "Bitrate tolerance",
      .type =        LQT_PARAMETER_FLOAT,
      .val_default = { .val_float = 1.0 },
      .val_min =     { .val_float = 0.0 },
      .val_max =     { .val_float = 100.0 },
      .num_digits =  1,
      .help_string = "Allowed variance of average bitrate"
    },
    {
      .name =        "x264_i_vbv_max_bitrate",
      .real_name =   "Maximum local bitrate",
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 0 },
      .help_string = "Sets a maximum local bitrate in kbits/s."
    },
    {
      .name =        "x264_i_vbv_buffer_size",
      .real_name =   "VBV Buffer size",
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 0 },
      .help_string = "Averaging period for the maximum local bitrate. "
                     "Measured in kbits."
    },
    {
      .name =        "x264_f_vbv_buffer_init",
      .real_name =   "Initial VBV buffer occupancy",
      .type =        LQT_PARAMETER_FLOAT,
      .num_digits =  2,
      .val_default = { .val_float = 0.9 },
      .val_min =     { .val_float = 0.0 },
      .val_max =     { .val_float = 1.0 },
      .help_string = "Sets the initial VBV buffer occupancy as a fraction of "
                     "the buffer size."
    },
    {
      .name =        "x264_f_ip_factor",
      .real_name =   "QP factor between I and P",
      .type =        LQT_PARAMETER_FLOAT,
      .num_digits =  2,
      .val_default = { .val_float = 1.40 },
    },
    {
      .name =        "x264_f_pb_factor",
      .real_name =   "QP factor between P and B",
      .type =        LQT_PARAMETER_FLOAT,
      .num_digits =  2,
      .val_default = { .val_float = 1.30 },
    },
    {
      .name =        "x264_partitions",
      .real_name =   "Partitions",
      .type =        LQT_PARAMETER_SECTION,
    },
    {
      .name =        "x264_analyse_8x8_transform",
      .real_name =   "8x8 transform",
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 0 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 1 },
    },
    {
      .name =        "x264_analyse_psub16x16",
      .real_name =   "8x16, 16x8 and 8x8 P-frame search",
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 1 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 1 },
    },
    {
      .name =        "x264_analyse_bsub16x16",
      .real_name =   "8x16, 16x8 and 8x8 B-frame search",
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 1 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 1 },
    },
    {
      .name =        "x264_analyse_psub8x8",
      .real_name =   "4x8, 8x4 and 4x4 P-frame search",
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 0 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 1 },
    },
    {
      .name =        "x264_analyse_i8x8",
      .real_name =   "8x8 Intra search",
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 1 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 1 },
      .help_string = "8x8 Intra search requires 8x8 transform",
    },
    {
      .name =        "x264_analyse_i4x4",
      .real_name =   "4x4 Intra search",
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 1 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 1 },
    },
    {
      .name =        "x264_me",
      .real_name =   "Motion estimation",
      .type =        LQT_PARAMETER_SECTION,
    },
    {
      .name =        "x264_i_me_method",
      .real_name =   "Method",
      .type =        LQT_PARAMETER_STRINGLIST,
      .val_default = { .val_string = "Hexagonal search" },
      .stringlist_options = (char*[]){ "Diamond search",
                                       "Hexagonal search",
                                       "Uneven Multi-Hexagon",
                                       "Exhaustive search",
                                       (char*)0 },
      .help_string = "Motion estimation method\n"
                     "Diamond search: fastest\n"
                     "Hexagonal search: default setting\n"
                     "Uneven Multi-Hexagon: better but slower\n"
                     "Exhaustive search: extremely slow, primarily for testing"
    },
    {
      .name =        "x264_i_subpel_refine",
      .real_name =   "Partition decision",
      .type =        LQT_PARAMETER_INT,
      .val_min =     { .val_int = 1 },
      .val_max =     { .val_int = 7 },
      .val_default = { .val_int = 5 },
      .help_string = "Subpixel motion estimation and partition decision "
                     "quality: 1=fast, 7=best."
    },
    {
      .name =        "x264_b_bframe_rdo",
      .real_name =   "RD based mode decision for B-frames",
      .type =        LQT_PARAMETER_INT,
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 1 },
      .val_default = { .val_int = 0 },
      .help_string = "RD based mode decision for B-frames. Requires partition "
                     "decision 6."
    },
    {
      .name =        "x264_i_me_range",
      .real_name =   "Search range",
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 16 },
      .help_string = "Maximum distance to search for motion estimation, "
                     "measured from predicted position(s). Default of 16 is "
                     "good for most footage, high motion sequences may benefit "
                     "from settings between 24-32."
    },
    {
      .name =        "x264_i_frame_reference",
      .real_name =   "Max Ref. frames",
      .type =        LQT_PARAMETER_INT,
      .val_min =     { .val_int = 1 },
      .val_max =     { .val_int = 16 },
      .val_default = { .val_int = 1 },
      .help_string = "This is effective in Anime, but seems to make little "
                     "difference in live-action source material. Some decoders "
                     "are unable to deal with large frameref values."
    },
    {
      .name =        "x264_b_chroma_me",
      .real_name =   "Chroma motion estimation",
      .type =        LQT_PARAMETER_INT,
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 1 },
      .val_default = { .val_int = 0 },
    },
    {
      .name =        "x264_b_mixed_references",
      .real_name =   "Mixed references",
      .type =        LQT_PARAMETER_INT,
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 1 },
      .val_default = { .val_int = 0 },
      .help_string = "Allow each MB partition in P-frames to have it's own "
                     "reference number"
    },
    {
      .name =        "x264_b_bidir_me",
      .real_name =   "Bidirectional ME",
      .type =        LQT_PARAMETER_INT,
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 1 },
      .val_default = { .val_int = 0 },
      .help_string = "Jointly optimize both MVs in B-frames"
    },
    {
      .name =        "x264_b_weighted_bipred",
      .real_name =   "Weighted biprediction",
      .type =        LQT_PARAMETER_INT,
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 1 },
      .val_default = { .val_int = 0 },
      .help_string = "Implicit weighting for B-frames"
    },
    {
      .name =        "x264_i_direct_mv_pred",
      .real_name =   "Direct MV prediction mode",
      .type =        LQT_PARAMETER_STRINGLIST,
      .val_default = { .val_string = "Spatial" },
      .stringlist_options = (char*[]){ "None",
                                       "Spatial",
                                       "Temporal",
                                       "Auto",
                                       (char*)0 },
    },
    {
      .name =        "x264_misc",
      .real_name =   "Misc",
      .type =        LQT_PARAMETER_SECTION,
    },
    {
      .name =        "x264_b_deblocking_filter",
      .real_name =   "Deblocking filter",
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 1 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 1 },
      .help_string = "Use deblocking loop filter (increases quality)."
    },
    {
      .name =        "x264_i_deblocking_filter_alphac0",
      .real_name =   "Deblocking filter strength",
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 0 },
      .val_min =     { .val_int = -6 },
      .val_max =     { .val_int = 6 },
      .help_string = "Loop filter AlphaC0 parameter"
    },
    {
      .name =        "x264_i_deblocking_filter_beta",
      .real_name =   "Deblocking filter threshold",
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 0 },
      .val_min =     { .val_int = -6 },
      .val_max =     { .val_int = 6 },
      .help_string = "Loop filter Beta parameter"
    },
    {
      .name =        "x264_b_cabac",
      .real_name =   "Enable CABAC",
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 1 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 1 },
      .help_string = "Enable CABAC (Context-Adaptive Binary Arithmetic "
                     "Coding). Slightly slows down encoding and decoding, but "
                     "should save 10-15% bitrate."
    },
    {
      .name =        "x264_i_trellis",
      .real_name =   "Trellis RD quantization",
      .type =        LQT_PARAMETER_STRINGLIST,
      .val_default = { .val_string = "Disabled" },
      .stringlist_options = (char*[]){ "Disabled",
                                       "Enabled (final)",
                                       "Enabled (always)",
                                       (char*)0 },
      .help_string = "Trellis RD quantization. Requires CABAC. Can be enabled "
                     "either for the final encode of a MB or for all mode "
                     "desisions"
    },
    {
      .name =        "x264_i_noise_reduction",
      .real_name =   "Noise reduction",
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 0 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 1<<16 } 
    },
    { /* End of parameters */ }
  };

static lqt_codec_info_static_t codec_info_x264 =
  {
    .name =                "x264",
    .long_name =           "H.264 (MPEG4 AVC) encoder",
    .description =         "Based on the x264 library",
    .fourccs =             fourccs_x264,
    .type =                LQT_CODEC_VIDEO,
    .direction =           LQT_DIRECTION_ENCODE,
    .encoding_parameters = encode_parameters_x264,
    .decoding_parameters = (lqt_parameter_info_static_t*)0,
    .compatibility_flags = LQT_FILE_QT_OLD | LQT_FILE_QT | LQT_FILE_MP4
  };

/* These are called from the plugin loader */

extern int get_num_codecs() { return 1; }

extern lqt_codec_info_static_t * get_codec_info(int index)
  {
  switch(index)
    {
    case 0:
      return &codec_info_x264;
    }
  return (lqt_codec_info_static_t*)0;
  }

/*
 *   Return the actual codec constructor
 */

extern lqt_init_video_codec_func_t get_video_codec(int index)
  {
  switch(index)
    {
    case 0:
      return quicktime_init_codec_x264;
      break;
    }
  return (lqt_init_video_codec_func_t)0;
  }
