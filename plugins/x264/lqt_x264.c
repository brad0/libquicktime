/*******************************************************************************
 lqt_x264.c

 libquicktime - A library for reading and writing quicktime/avi/mp4 files.
 http://libquicktime.sourceforge.net

 Copyright (C) 2002 Heroine Virtual Ltd.
 Copyright (C) 2002-2011 Members of the libquicktime project.

 This library is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation; either version 2.1 of the License, or (at your option)
 any later version.

 This library is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along
 with this library; if not, write to the Free Software Foundation, Inc., 51
 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*******************************************************************************/ 

#include "lqt_private.h"
#include <quicktime/lqt_codecapi.h>
#include <x264.h> // X264_BUILD value
#include "qtx264.h"

static char * fourccs_x264[]  = { "avc1", (char*)0 };

static lqt_parameter_info_static_t encode_parameters_x264[] =
  {
    {
      .name =        "x264_sec_frame_type",
      .real_name =   TRS("Frame-type options"),
      .type =        LQT_PARAMETER_SECTION
    },
    {
      .name =        "x264_i_keyint_max",
      .real_name =   TRS("Maximum GOP size"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 250 },
    },
    {
      .name =        "x264_i_keyint_min",
      .real_name =   TRS("Minimum GOP size"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 25 },
    },
    {
      .name =        "x264_i_scenecut_threshold",
      .real_name =   TRS("Scenecut threshold"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 40 },
      .help_string = TRS("How aggressively to insert extra I-frames")
    },
    {
      .name =        "x264_i_bframe",
      .real_name =   TRS("B-Frames"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 0 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 16 },
      .help_string = TRS("Number of B-frames between I and P"),
    },
#if X264_BUILD < 63
    {
      .name =        "x264_b_bframe_adaptive",
      .real_name =   TRS("Adaptive B-frame decision"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 1 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 1 },
    },
#else
    {
    .name =        "x264_i_bframe_adaptive",
    .real_name =   TRS("Adaptive B-frame decision"),
    .type =        LQT_PARAMETER_STRINGLIST,
    .val_default = { .val_string = "Fast" },
    .stringlist_options = (char*[]){ TRS("None"),
                                     TRS("Fast"),
                                     TRS("Trellis"),
                                     (char*)0 },
    },
#endif
    {
      .name =        "x264_i_bframe_bias",
      .real_name =   TRS("B-frame bias"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 0 },
      .val_min =     { .val_int =  -90 },
      .val_max =     { .val_int = 100 },
      .help_string = TRS("Influences how often B-frames are used"),
    },
#if X264_BUILD >= 88
    {
      .name =        "x264_i_bframe_pyramid",
      .real_name =   TRS("B-frame pyramid"),
      .type =        LQT_PARAMETER_STRINGLIST,
      .val_default = { .val_string = "Normal" },
      .stringlist_options = (char*[]){ TRS("None"),
                                       TRS("Strict"),
                                       TRS("Normal"),
                                       (char*)0 },
      .help_string = TRS("Keep some B-frames as references")
    },
#elif X264_BUILD >= 78
    {
      .name =        "x264_i_bframe_pyramid",
      .real_name =   TRS("B-frame pyramid"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 0 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 2 },
      .help_string = TRS("Keep some B-frames as references")
    },
#else
    {
      .name =        "x264_b_bframe_pyramid",
      .real_name =   TRS("B-frame pyramid"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 0 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 1 },
      .help_string = TRS("Keep some B-frames as references")
    },
#endif
    {
      .name =        "x264_sec_ratecontrol",
      .real_name =   TRS("Ratecontrol"),
      .type =        LQT_PARAMETER_SECTION
    },
    {
      .name =        "x264_i_rc_method",
      .real_name =   TRS("Ratecontrol method"),
      .type =        LQT_PARAMETER_STRINGLIST,
      .val_default = { .val_string = "CRF based VBR" },
      .stringlist_options = (char*[]){ TRS("Constant quality"),
                                       TRS("CRF based VBR"),
                                       TRS("Average bitrate"),
                                       (char*)0 },
      .help_string = TRS("Ratecontrol method:\n"
                     "Constant quality: Specify a quantizer parameter below\n"
                     "CRF based VBR: Specify a rate factor below\n"
                     "Average bitrate: Specify a bitrate below\n"
                         "Selecting 2-pass encoding will force Average bitrate."),
    },
    {
      .name =        "x264_i_bitrate",
      .real_name =   TRS("Bitrate"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 0 },
      .help_string = TRS("Bitrate in kbit/s. 0 means VBR (recommended)")
    },
    {
      .name =        "x264_f_rate_tolerance",
      .real_name =   TRS("Bitrate tolerance"),
      .type =        LQT_PARAMETER_FLOAT,
      .val_default = { .val_float = 1.0 },
      .val_min =     { .val_float = 0.0 },
      .val_max =     { .val_float = 100.0 },
      .num_digits =  1,
      .help_string = TRS("Allowed variance of average bitrate")
    },
    {
      .name =        "x264_i_vbv_max_bitrate",
      .real_name =   TRS("Maximum local bitrate"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 0 },
      .help_string = TRS("Sets a maximum local bitrate in kbits/s.")
    },
    {
      .name =        "x264_i_vbv_buffer_size",
      .real_name =   TRS("VBV Buffer size"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 0 },
      .help_string = TRS("Averaging period for the maximum local bitrate. "
                         "Measured in kbits.")
    },
    {
      .name =        "x264_f_vbv_buffer_init",
      .real_name =   TRS("Initial VBV buffer occupancy"),
      .type =        LQT_PARAMETER_FLOAT,
      .num_digits =  2,
      .val_default = { .val_float = 0.9 },
      .val_min =     { .val_float = 0.0 },
      .val_max =     { .val_float = 1.0 },
      .help_string = TRS("Sets the initial VBV buffer occupancy as a fraction of "
                         "the buffer size.")
    },
#if X264_BUILD >= 69
    {
      .name =        "x264_b_psy",
      .real_name =   TRS("Psy optimizations"),
      .type =        LQT_PARAMETER_INT,
      .val_min     = { .val_int = 0 },
      .val_max     = { .val_int = 1 },
      .val_default = { .val_int = 1 },
      .help_string = TRS("Psychovisual optimization"),
    },
#endif
    
#if X264_BUILD >= 63
    {
      .name =        "x264_f_psy_rd",
      .real_name =   TRS("Psy RD strength"),
      .type =        LQT_PARAMETER_FLOAT,
      .num_digits =  2,
      .val_min     = { .val_float = 0.0 },
      .val_max     = { .val_float = 10.0 },
      .val_default = { .val_float = 1.0 },
      .help_string = TRS("Strength of psychovisual optimization: RD (requires Partition decision >= 6)"),
    },
    {
      .name =        "x264_f_psy_trellis",
      .real_name =   TRS("Psy trellis strength"),
      .type =        LQT_PARAMETER_FLOAT,
      .num_digits =  2,
      .val_min     = { .val_float = 0.0 },
      .val_max     = { .val_float = 10.0 },
      .val_default = { .val_float = 0.0 },
      .help_string = TRS("Strength of psychovisual optimization (requires trellis)"),
    },
#endif
#if X264_BUILD >= 69
    {
      .name =        "x264_b_mb_tree",
      .real_name =   TRS("Macroblock-tree ratecontrol"),
      .type =        LQT_PARAMETER_INT,
      .val_min     = { .val_int = 0 },
      .val_max     = { .val_int = 1 },
      .val_default = { .val_int = 1 },
    },
    {
      .name =        "x264_i_lookahead",
      .real_name =   TRS("Lookahead"),
      .type =        LQT_PARAMETER_INT,
      .val_min     = { .val_int = 0 },
      .val_max     = { .val_int = 250 },
      .val_default = { .val_int = 40 },
      .help_string = TRS("Number of frames for frametype lookahead"),
    },
#endif
    {
      .name =        "x264_sec_quantizer",
      .real_name =   TRS("Quantizer"),
      .type =        LQT_PARAMETER_SECTION
    },
#if X264_BUILD < 54
    {
      .name =        "x264_i_rf_constant",
      .real_name =   TRS("Nominal Quantizer parameter"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 26 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 51 },
      .help_string = TRS("This selects the nominal quantizer to use (1 to 51). "
                     "Lower values result in better fidelity, but higher "
                         "bitrates. 26 is a good default value. 0 means lossless.")
    },
#else
    {
      .name =        "x264_f_rf_constant",
      .real_name =   TRS("Nominal Quantizer parameter"),
      .type =        LQT_PARAMETER_FLOAT,
      .val_default = { .val_float = 23.0 },
      .val_min =     { .val_float = 0.0 },
      .val_max =     { .val_float = 51.0 },
      .help_string = TRS("This selects the nominal quantizer to use (1 to 51). "
                     "Lower values result in better fidelity, but higher "
                         "bitrates. 26 is a good default value. 0 means lossless.")
    },
#endif
    {
      .name =        "x264_i_qp_constant",
      .real_name =   TRS("Quantizer parameter"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 23 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 51 },
      .help_string = TRS("This selects the quantizer to use (1 to 51). Lower "
                     "values result in better fidelity, but higher bitrates. "
                         "26 is a good default value. 0 means lossless.")
    },
    {
      .name =        "x264_i_qp_min",
      .real_name =   TRS("Minimum quantizer parameter"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 10 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 51 },
      .help_string = TRS("Minimum quantizer parameter")
    },
    {
      .name =        "x264_i_qp_max",
      .real_name =   TRS("Maximum quantizer parameter"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 51 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 51 },
      .help_string = TRS("Maximum quantizer parameter")
    },
    {
      .name =        "x264_i_qp_step",
      .real_name =   TRS("Maximum QP step"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 4 },
      .val_min =     { .val_int = 1 },
      .val_max =     { .val_int = 50 },
      .help_string = TRS("Maximum quantizer step")
    },
    {
      .name =        "x264_f_ip_factor",
      .real_name =   TRS("QP factor between I and P"),
      .type =        LQT_PARAMETER_FLOAT,
      .num_digits =  2,
      .val_default = { .val_float = 1.40 },
    },
    {
      .name =        "x264_f_pb_factor",
      .real_name =   TRS("QP factor between P and B"),
      .type =        LQT_PARAMETER_FLOAT,
      .num_digits =  2,
      .val_default = { .val_float = 1.30 },
    },
#if X264_BUILD >= 62
    {
      .name =        "x264_i_aq_mode",
      .real_name =   TRS("Adaptive quantization"),
      .type =        LQT_PARAMETER_STRINGLIST,
      .val_default = { .val_string = "Variance AQ (complexity mask)" },
      .stringlist_options = (char*[]){ TRS("None"),
                                       TRS("Variance AQ (complexity mask)"),
#if X264_BUILD >= 69
                                       TRS("Autovariance AQ (experimental)"),
#endif
                                       (char*)0 },
    },
    {
      .name =        "x264_f_aq_strength",
      .real_name =   TRS("AQ strength"),
      .type =        LQT_PARAMETER_FLOAT,
      .num_digits =  2,
      .val_min     = { .val_float = 0.5 },
      .val_max     = { .val_float = 1.5 },
      .val_default = { .val_float = 1.0 },
      .help_string = TRS("Adaptive quantization strength:\n"
                         "Reduces blocking and blurring in flat and\n"
                         "textured areas"),
    },
#endif

#if X264_BUILD >= 88
    {
      .name =        "x264_f_qcompress",
      .real_name =   TRS("QP curve compression"),
      .type =        LQT_PARAMETER_FLOAT,
      .num_digits =  2,
      .val_min     = { .val_float = 0.0 },
      .val_max     = { .val_float = 1.0 },
      .val_default = { .val_float = 0.6 },
      .help_string = TRS("Only for 2-pass encoding"),
    },
    {
      .name =        "x264_f_qblur",
      .real_name =   TRS("QP Reduce fluctuations in QP (after curve compression)"),
      .type =        LQT_PARAMETER_FLOAT,
      .num_digits =  2,
      .val_default = { .val_float = 0.5 },
      .help_string = TRS("Only for 2-pass encoding"),
    },
    {
      .name =        "x264_f_complexity_blur",
      .real_name =   TRS("Temporally blur complexity"),
      .type =        LQT_PARAMETER_FLOAT,
      .num_digits =  2,
      .val_default = { .val_float = 20 },
      .help_string = TRS("Only for 2-pass encoding"),
    },
    {
      .name =        "x264_i_chroma_qp_offset",
      .real_name =   TRS("QP difference between chroma and luma"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 0 },
    },
    {
      .name =        "x264_i_luma_deadzone_0",
      .real_name =   TRS("Inter luma quantization deadzone"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 21 },
    },
    {
      .name =        "x264_i_luma_deadzone_1",
      .real_name =   TRS("Intra luma quantization deadzone"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 11 },
    },
#endif

    {
      .name =        "x264_sec_partitions",
      .real_name =   TRS("Partitions"),
      .type =        LQT_PARAMETER_SECTION,
    },
    {
      .name =        "x264_analyse_8x8_transform",
      .real_name =   TRS("8x8 transform"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 0 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 1 },
    },
    {
      .name =        "x264_analyse_psub16x16",
      .real_name =   TRS("8x16, 16x8 and 8x8 P-frame search"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 1 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 1 },
    },
    {
      .name =        "x264_analyse_bsub16x16",
      .real_name =   TRS("8x16, 16x8 and 8x8 B-frame search"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 1 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 1 },
    },
    {
      .name =        "x264_analyse_psub8x8",
      .real_name =   TRS("4x8, 8x4 and 4x4 P-frame search"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 0 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 1 },
    },
    {
      .name =        "x264_analyse_i8x8",
      .real_name =   TRS("8x8 Intra search"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 1 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 1 },
      .help_string = TRS("8x8 Intra search requires 8x8 transform"),
    },
    {
      .name =        "x264_analyse_i4x4",
      .real_name =   TRS("4x4 Intra search"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 1 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 1 },
    },
    {
      .name =        "x264_sec_me",
      .real_name =   TRS("Motion estimation"),
      .type =        LQT_PARAMETER_SECTION,
    },
    {
      .name =        "x264_i_me_method",
      .real_name =   TRS("Method"),
      .type =        LQT_PARAMETER_STRINGLIST,
      .val_default = { .val_string = "Hexagonal search" },
      .stringlist_options = (char*[]){ TRS("Diamond search"),
                                       TRS("Hexagonal search"),
                                       TRS("Uneven Multi-Hexagon"),
                                       TRS("Exhaustive search"),
#if X264_BUILD > 57
                                       TRS("Hadamard exhaustive search (slow)"),
#endif

                                       (char*)0 },
      .help_string = TRS("Motion estimation method\n"
                     "Diamond search: fastest\n"
                     "Hexagonal search: default setting\n"
                     "Uneven Multi-Hexagon: better but slower\n"
                         "Exhaustive search: extremely slow, primarily for testing")
    },
    {
      .name =        "x264_i_subpel_refine",
      .real_name =   TRS("Partition decision"),
      .type =        LQT_PARAMETER_INT,
      .val_min =     { .val_int = 1 },
#if X264_BUILD < 65
      .val_max =     { .val_int = 7 },
#else
      .val_max =     { .val_int = 9 },
#endif
#if X264_BUILD < 88
      .val_default = { .val_int = 5 },
#else
      .val_default = { .val_int = 7 },
#endif
      .help_string = TRS("Subpixel motion estimation and partition decision "
#if X264_BUILD < 65
                         "quality: 1=fast, 7=best.")
#else
                         "quality: 1=fast, 9=best.")
#endif
    },
#if X264_BUILD < 65
    {
      .name =        "x264_b_bframe_rdo",
      .real_name =   TRS("RD based mode decision for B-frames"),
      .type =        LQT_PARAMETER_INT,
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 1 },
      .val_default = { .val_int = 0 },
      .help_string = TRS("RD based mode decision for B-frames. Requires partition "
                         "decision 6.")
    },
#endif
    {
      .name =        "x264_i_me_range",
      .real_name =   TRS("Search range"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 16 },
      .help_string = TRS("Maximum distance to search for motion estimation, "
                     "measured from predicted position(s). Default of 16 is "
                     "good for most footage, high motion sequences may benefit "
                         "from settings between 24-32.")
    },
    {
      .name =        "x264_i_frame_reference",
      .real_name =   TRS("Max Ref. frames"),
      .type =        LQT_PARAMETER_INT,
      .val_min =     { .val_int = 1 },
      .val_max =     { .val_int = 16 },
      .val_default = { .val_int = 1 },
      .help_string = TRS("This is effective in Anime, but seems to make little "
                     "difference in live-action source material. Some decoders "
                         "are unable to deal with large frameref values.")
    },
    {
      .name =        "x264_b_chroma_me",
      .real_name =   TRS("Chroma motion estimation"),
      .type =        LQT_PARAMETER_INT,
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 1 },
      .val_default = { .val_int = 1 },
    },
    {
      .name =        "x264_b_mixed_references",
      .real_name =   TRS("Mixed references"),
      .type =        LQT_PARAMETER_INT,
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 1 },
      .val_default = { .val_int = 1 },
      .help_string = TRS("Allow each MB partition in P-frames to have it's own "
                         "reference number")
    },
#if X264_BUILD < 65
    {
      .name =        "x264_b_bidir_me",
      .real_name =   TRS("Bidirectional ME"),
      .type =        LQT_PARAMETER_INT,
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 1 },
      .val_default = { .val_int = 0 },
      .help_string = TRS("Jointly optimize both MVs in B-frames")
    },
#endif
    {
      .name =        "x264_b_weighted_bipred",
      .real_name =   TRS("Weighted biprediction"),
      .type =        LQT_PARAMETER_INT,
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 1 },
      .val_default = { .val_int = 0 },
      .help_string = TRS("Implicit weighting for B-frames")
    },
#if X264_BUILD >= 88
    {
      .name =        "x264_i_weighted_pred",
      .real_name =   TRS("Weighted prediction for P-frames"),
      .type =        LQT_PARAMETER_STRINGLIST,
      .val_default = { .val_string = "Smart analysis" },
      .stringlist_options = (char*[]){ TRS("Disabled"),
                                       TRS("Blind offset"),
                                       TRS("Smart analysis"),
                                       (char*)0 },
    },
#endif
    {
      .name =        "x264_i_direct_mv_pred",
      .real_name =   TRS("Direct MV prediction mode"),
      .type =        LQT_PARAMETER_STRINGLIST,
      .val_default = { .val_string = "Spatial" },
      .stringlist_options = (char*[]){ TRS("None"),
                                       TRS("Spatial"),
                                       TRS("Temporal"),
                                       TRS("Auto"),
                                       (char*)0 },
    },
    {
      .name =        "x264_sec_misc",
      .real_name =   TRS("Misc"),
      .type =        LQT_PARAMETER_SECTION,
    },
    {
      .name =        "x264_b_deblocking_filter",
      .real_name =   TRS("Deblocking filter"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 1 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 1 },
      .help_string = TRS("Use deblocking loop filter (increases quality).")
    },
    {
      .name =        "x264_i_deblocking_filter_alphac0",
      .real_name =   TRS("Deblocking filter strength"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 0 },
      .val_min =     { .val_int = -6 },
      .val_max =     { .val_int = 6 },
      .help_string = TRS("Loop filter AlphaC0 parameter")
    },
    {
      .name =        "x264_i_deblocking_filter_beta",
      .real_name =   TRS("Deblocking filter threshold"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 0 },
      .val_min =     { .val_int = -6 },
      .val_max =     { .val_int = 6 },
      .help_string = TRS("Loop filter Beta parameter")
    },
    {
      .name =        "x264_b_cabac",
      .real_name =   TRS("Enable CABAC"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 1 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 1 },
      .help_string = TRS("Enable CABAC (Context-Adaptive Binary Arithmetic "
                     "Coding). Slightly slows down encoding and decoding, but "
                         "should save 10-15% bitrate.")
    },
    {
      .name =        "x264_i_trellis",
      .real_name =   TRS("Trellis RD quantization"),
      .type =        LQT_PARAMETER_STRINGLIST,
      .val_default = { .val_string = "Enabled (final)" },
      .stringlist_options = (char*[]){ TRS("Disabled"),
                                       TRS("Enabled (final)"),
                                       TRS("Enabled (always)"),
                                       (char*)0 },
      .help_string = TRS("Trellis RD quantization. Requires CABAC. Can be enabled "
                     "either for the final encode of a MB or for all mode "
                         "desisions")
    },
    {
      .name =        "x264_i_noise_reduction",
      .real_name =   TRS("Noise reduction"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 0 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 1<<16 } 
    },
#if X264_BUILD >= 28
    {
      .name =        "x264_i_threads",
      .real_name =   TRS("Threads"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 1 },
      // .val_min =     { .val_int =  },
      // .val_max =     { .val_int = 256 },
      .help_string = TRS("Number of threads")
    },
#endif
#if X264_BUILD >= 88
    {
      .name =        "x264_b_fast_pskip",
      .real_name =   TRS("early SKIP detection on P-frames"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 1 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 1 },
    },
    {
      .name =        "x264_b_dct_decimate",
      .real_name =   TRS("Transform coefficient thresholding on P-frames"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 1 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 1 },
    },
#endif
    { /* End of parameters */ }
  };

static lqt_codec_info_static_t codec_info_x264 =
  {
    .name =                "x264",
    .long_name =           TRS("H.264 (MPEG4 AVC) encoder"),
    .description =         TRS("Based on the x264 library"),
    .fourccs =             fourccs_x264,
    .type =                LQT_CODEC_VIDEO,
    .direction =           LQT_DIRECTION_ENCODE,
    .encoding_parameters = encode_parameters_x264,
    .decoding_parameters = (lqt_parameter_info_static_t*)0,
    .compatibility_flags = LQT_FILE_QT_OLD |
                           LQT_FILE_QT |
                           LQT_FILE_MP4 |
                           LQT_FILE_AVI |
                           LQT_FILE_AVI_ODML,
    .compression_id      = LQT_COMPRESSION_H264,
  };

/* These are called from the plugin loader */

LQT_EXTERN int get_num_codecs() { return 1; }

LQT_EXTERN lqt_codec_info_static_t * get_codec_info(int index)
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

LQT_EXTERN lqt_init_codec_func_t get_codec(int index)
  {
  switch(index)
    {
    case 0:
      return quicktime_init_codec_x264;
      break;
    }
  return (lqt_init_codec_func_t)0;
  }
