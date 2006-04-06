/*
 *   IMPORTANT: To keep the mess at a reasonable level,
 *   *all* parameters *must* appear in the same order as in
 *   the AVCocecContext structure, except the flags, which come at the very end
 */


/** Rate control */
#define PARAM_BITRATE_AUDIO                     \
  { \
    name:      "ff_bit_rate_audio",                 \
    real_name: "Bit rate (kbps)",                \
    type:      LQT_PARAMETER_INT,                \
    val_default: { val_int: 128 },               \
  }

/** Rate control */
#define PARAM_BITRATE_VIDEO \
  { \
    name:      "ff_bit_rate_video",                 \
    real_name: "Bit rate (kbps)",                \
    type:      LQT_PARAMETER_INT,                \
    val_default: { val_int: 800 },               \
  }

/** Rate control */
#define PARAM_BITRATE_TOLERANCE                 \
  {                                               \
    name:        "ff_bit_rate_tolerance",           \
    real_name:   "Bitrate tolerance (kbps)",             \
    type:        LQT_PARAMETER_INT,             \
    val_default: { val_int: 8000 },           \
    help_string: "Number of bits the bitstream is allowed to diverge from the reference.\
 Unused for constant quantizer encoding" \
  }

/** Motion estimation */
#define PARAM_ME_METHOD                         \
  {\
    name:      "ff_me_method",\
    real_name: "Motion estimation method",\
    type:      LQT_PARAMETER_STRINGLIST,\
    val_default: {val_string: "Zero"},\
    stringlist_options: (char *[]){"Zero", "Phods", "Log", "X1", "Epzs", "Full", (char *)0} \
  }

/** Frame types */
#define PARAM_GOP_SIZE                          \
  { \
    name:      "gop_size", \
    real_name: "GOP size (0 = intra only)", \
    type:      LQT_PARAMETER_INT, \
    val_default: { val_int: 250 },   \
    val_min:     { val_int: 0 },     \
    val_max:     { val_int: 300 },   \
  } \

/** Quantizer */
#define PARAM_QCOMPRESS \
  {                     \
    name:        "ff_qcompress",                   \
    real_name:   "Quantizer compression",\
    type:        LQT_PARAMETER_FLOAT,  \
    val_default: { val_float: 0.5 }, \
    val_min:     { val_float: 0.0 }, \
    val_max:     { val_float: 1.0 }, \
    num_digits:  2, \
    help_string: "Amount of qscale change between easy & hard scenes" \
  }

/** Quantizer */
#define PARAM_QBLUR \
  {                 \
    name:        "ff_qblur",                     \
    real_name:   "Quantizer blur", \
    type:        LQT_PARAMETER_FLOAT,  \
    val_default: { val_float: 0.0 }, \
    val_min:     { val_float: 0.0 }, \
    val_max:     { val_float: 1.0 }, \
    num_digits:  2,                  \
    help_string: "Amount of qscale smoothing over time",        \
  }

/** Quantizer */
#define PARAM_QMIN \
  {\
    name:      "ff_qmin", \
    real_name: "Minimum quantizer scale", \
    type:      LQT_PARAMETER_INT, \
    val_default: { val_int: 2 }, \
    val_min:     { val_int: 0 }, \
    val_max:     { val_int: 31 },               \
  }

/** Quantizer */
#define PARAM_QMAX                              \
  {\
    name:      "ff_qmax", \
    real_name: "Maximum quantizer scale", \
    type:      LQT_PARAMETER_INT, \
    val_default: { val_int: 31 }, \
    val_min:     { val_int: 0 }, \
    val_max:     { val_int: 31 },               \
  } \

/** Quantizer */
#define PARAM_MAX_QDIFF \
  {                     \
    name:      "ff_max_qdiff",                                  \
    real_name: "Maximum quantizer difference",                  \
    type:      LQT_PARAMETER_INT,                               \
    val_default: { val_int: 3 },                                \
    val_min:     { val_int: 0 },                                \
    val_max:     { val_int: 31 },                               \
    help_string: "Maximum quantizer difference between frames"  \
  }

/** Frame types */
#define PARAM_MAX_B_FRAMES \
  {                                                                     \
    name:      "ff_max_b_frames",                                       \
    real_name: "Max B-Frames",                                          \
    type:      LQT_PARAMETER_INT,                                       \
    val_default: { val_int: 0 },                                        \
    val_min:     { val_int: 0 },                                        \
    val_max:     { val_int: FF_MAX_B_FRAMES },                          \
    help_string: "Maximum number of B-frames between non B-frames"      \
  }

/** Quantizer */
#define PARAM_B_QUANT_FACTOR \
  {                                                                     \
    name:      "ff_b_quant_factor",                                     \
    real_name: "B quantizer factor",                                    \
    type:        LQT_PARAMETER_FLOAT,                                   \
    val_default: { val_float: 1.25 },                                 \
    val_min:     { val_float: -31.0 },                                \
    val_max:     { val_float: 31.0 },                                 \
    num_digits:  2, \
    help_string: "Quantizer factor between B-frames and non-B-frames",  \
  }

/** Frame types */
#define PARAM_B_FRAME_STRATEGY \
  {                                                             \
    name:        "ff_b_frame_strategy",                         \
    real_name:   "Avoid B-frames in high motion scenes",        \
    type:        LQT_PARAMETER_INT,                             \
    val_default: { val_int: 0 },                                \
    val_min:     { val_int: 0 },                                \
    val_max:     { val_int: 1 },                                \
  }

#define PARAM_LUMA_ELIM_THRESHOLD               \
  {                                             \
    name:      "ff_luma_elim_threshold",          \
    real_name: "Luma elimination threshold",    \
    type:        LQT_PARAMETER_INT,             \
    val_default: { val_int:  0 },                                \
    val_min:     { val_int: -99 },                                \
    val_max:     { val_int:  99 },                                \
    help_string: "Single coefficient elimination threshold for " \
"luminance. Negative values also consider dc coefficient. -4 is " \
"JVT recommendation" \
  }
    
#define PARAM_CHROMA_ELIM_THRESHOLD               \
  {                                             \
    name:      "ff_chroma_elim_threshold",          \
    real_name: "Chroma elimination threshold",    \
    type:        LQT_PARAMETER_INT,             \
    val_default: { val_int:  0 },                                \
    val_min:     { val_int: -99 },                                \
    val_max:     { val_int:  99 },                                \
    help_string: "Single coefficient elimination threshold for " \
"chrominamce. Negative values also consider dc coefficient. 7 is " \
"JVT recommendation" \
  }

#define PARAM_STRICT_STANDARD_COMPLIANCE \
  {                                                              \
    name: "ff_strict_std_compliance",                              \
    real_name:   "Standards compliance",                         \
    type:        LQT_PARAMETER_INT,                              \
    val_default: { val_int:  0 },                                \
    val_min:     { val_int: -2 },                                \
    val_max:     { val_int:  2 },                                \
    help_string: "2: Strictly conform to a older more strict version\
 of the spec or reference software\n\
1: Strictly conform to all the things in the spec no matter what \
consequences\n\
0: Default\n\
-1: Allow inofficial extensions\n\
-2: Allow non standarized experimental things" \
  }

/** Quantizer */
#define PARAM_B_QUANT_OFFSET \
  { \
    name:       "ff_b_quant_offset",                    \
    real_name:  "B quantizer offset",                   \
    type:       LQT_PARAMETER_FLOAT,                    \
    val_default: { val_float: 1.25 },                   \
    val_min:     { val_float: 0.0 },                    \
    val_max:     { val_float: 31.0 },                   \
    num_digits:  2, \
    help_string: "Quantizer offset between B-frames and non-B-frames\n"\
    "if > 0 then the last p frame quantizer will be used (q= lastp_q*factor+offset)\n"\
    "if < 0 then normal ratecontrol will be done (q= -normal_q*factor+offset)\n"\
  }

/** Rate control */
#define PARAM_RC_MIN_RATE \
  { \
    name:        "ff_rc_min_rate",\
    real_name:   "Minimum bitrate (kbps)",\
    type:        LQT_PARAMETER_INT,\
    val_default: { val_int: 0 },                        \
    help_string: "Minimum bitrate (0 means arbitrary)", \
  }

/** Rate control */
#define PARAM_RC_MAX_RATE \
  {                                             \
    name:      "ff_rc_max_rate",\
    real_name: "Maximum bitrate (kbps)",\
    type:      LQT_PARAMETER_INT,\
    val_default: { val_int: 0 },                        \
    help_string: "Maximum bitrate (0 means arbitrary)", \
  }

/** Rate control */
#define PARAM_RC_BUFFER_SIZE \
  {                          \
    name:      "ff_rc_buffer_size",        \
    real_name: "RC buffer size",           \
    type:      LQT_PARAMETER_INT,          \
    val_default: { val_int: 0 },           \
    help_string: "Decoder bitstream buffer size in kbits. When encoding " \
"with max and/or min bitrate, this must be specified." \
  }

/** Rate control */
#define PARAM_RC_BUFFER_AGGRESSIVITY \
  {                                           \
    name:       "ff_rc_buffer_aggressivity",    \
    real_name:  "RC buffer aggressivity",               \
    type:       LQT_PARAMETER_FLOAT,                    \
    val_default: { val_float: 1.0 },                   \
    val_min:     { val_float: 0.01 },                    \
    val_max:     { val_float: 99.0 },                   \
    num_digits:  2, \
  }

/** Quantizer */
#define PARAM_I_QUANT_FACTOR \
  {                                                                     \
    name:      "ff_i_quant_factor",                                     \
    real_name: "I quantizer factor",                                    \
    type:        LQT_PARAMETER_FLOAT,                                   \
    val_default: { val_float: -0.8 },                                 \
    val_min:     { val_float: -31.0 },                                \
    val_max:     { val_float: 31.0 },                                 \
    num_digits:  1, \
    help_string: "Quantizer factor between P-frames and I-frames.\n"\
"If > 0 then the last P frame quantizer will be used (q= lastp_q*factor+offset).\n"\
"If < 0 then normal ratecontrol will be done (q= -normal_q*factor+offset)\n",  \
  }

/** Quantizer */
#define PARAM_I_QUANT_OFFSET \
  {                                                              \
    name:      "ff_i_quant_offset",                              \
    real_name: "I quantizer offset",                             \
    type:        LQT_PARAMETER_FLOAT,                            \
    val_default: { val_float:  0.0 },                            \
    val_min:     { val_float:  0.0 },                            \
    val_max:     { val_float: 31.0 },                                 \
    num_digits:  1, \
    help_string: "Quantizer offset between P-frames and I-frames",  \
  }

/** Rate control */
#define PARAM_RC_INITIAL_COMPLEX \
  {                                                          \
    name:      "ff_rc_initial_cplx",                           \
    real_name: "Initial RC complexity",                      \
    type:      LQT_PARAMETER_FLOAT,\
    val_default: { val_float:  0.0 },                            \
    val_min:     { val_float:  0.0 },                            \
    val_max:     { val_float: 99.0 },                                 \
    num_digits:  1, \
  }

/** Masking */
#define PARAM_LUMI_MASKING                      \
  {                                             \
    name:        "ff_lumi_masking",               \
    real_name: "Luminance masking",      \
    type:      LQT_PARAMETER_FLOAT,                              \
    val_default: { val_float:  0.0 },                            \
    val_min:     { val_float:  0.0 },                            \
    val_max:     { val_float:  1.0 },                          \
    num_digits:  2, \
    help_string: "Encode very bright image parts with reduced quality."\
    " 0 means disabled, 0-0.3 is a sane range.",      \
  }

/** Masking */
#define PARAM_TEMPORAL_CPLX_MASKING                      \
  {                                             \
    name:        "ff_temporal_cplx_masking",               \
    real_name: "Temporary complexity masking",      \
    type:      LQT_PARAMETER_FLOAT,                              \
    val_default: { val_float:  0.0 },                            \
    val_min:     { val_float:  0.0 },                            \
    val_max:     { val_float:  1.0 },                          \
    num_digits:  2, \
    help_string: "Encode very fast moving image parts with reduced quality."\
    " 0 means disabled.",      \
  }

/** Masking */
#define PARAM_SPATIAL_CPLX_MASKING             \
  {                                             \
    name:        "ff_spatial_cplx_masking",               \
    real_name: "Spatial complexity masking",      \
    type:      LQT_PARAMETER_FLOAT,                              \
    val_default: { val_float:  0.0 },                            \
    val_min:     { val_float:  0.0 },                            \
    val_max:     { val_float:  1.0 },                          \
    num_digits:  2, \
    help_string: "Encode very complex image parts with reduced quality."\
    " 0 means disabled, 0-0.5 is a sane range.",                          \
  }

/** Masking */
#define PARAM_P_MASKING             \
  {                                             \
    name:        "ff_p_masking",               \
    real_name: "Inter block masking",      \
    type:      LQT_PARAMETER_FLOAT,                              \
    val_default: { val_float:  0.0 },                            \
    val_min:     { val_float:  0.0 },                            \
    val_max:     { val_float:  1.0 },                          \
    num_digits:  2, \
    help_string: "Encode inter blocks with reduced quality (increases the quality of intra blocks). "\
    " 0 means disabled, 1 will double the bits allocated for intra blocks.",      \
  }

/** Masking */
#define PARAM_DARK_MASKING             \
  {                                             \
    name:        "ff_dark_masking",               \
    real_name: "Darkness masking",      \
    type:      LQT_PARAMETER_FLOAT,                              \
    val_default: { val_float:  0.0 },                            \
    val_min:     { val_float:  0.0 },                            \
    val_max:     { val_float:  1.0 },                            \
    num_digits:  2, \
    help_string: "Encode very dark image parts with reduced quality." \
    "0 means disabled, 0-0.3 is a sane range.",                          \
  }

#define PARAM_PREDICTION_METHOD                 \
  {                                               \
    name:       "ff_prediction_method",             \
    real_name: "Precition method",                \
    type:      LQT_PARAMETER_STRINGLIST,          \
    val_default: { val_string: "Left" },\
stringlist_options: (char*[]){ "Left", "Plane", "Median", (char*)0 },    \
  }

/** Quantizer */
#define PARAM_MB_QMIN \
  {                   \
  name:      "ff_mb_qmin", \
    real_name: "Minimum MB quantizer",          \
    type:    LQT_PARAMETER_INT,                 \
    val_default: { val_int: 2 },\
    val_min:     { val_int: 0 },\
    val_max:     { val_int: 31 },\
  }

/** Quantizer */
#define PARAM_MB_QMAX \
  {                   \
  name:      "ff_mb_qmax", \
    real_name: "Maximum MB quantizer",          \
    type:    LQT_PARAMETER_INT,                 \
    val_default: { val_int: 31 },\
    val_min:     { val_int: 0 },\
    val_max:     { val_int: 31 },\
  }

#define COMPARE_FUNCS (char*[]){ "SAD", "SSE", "SATD", "DCT", "PSNR", \
                        "BIT", "RD", "ZERO", "VSAD", "VSSE", "NSSE" }

#define COMPARE_FUNCS_HELP "SAD: Sum of absolute differences\n"\
"SSE: Sum of squared errors\n"\
"SATD: Sum of absolute Hadamard transformed differences\n"\
"DCT: Sum of absolute DCT transformed differences\n"\
"PSNR: Sum of squared quantization errors (low quality)\n"\
"BIT: Number of bits needed for the block\n"\
"RD: Rate distortion optimal (slow)\n"\
"ZERO: 0\n"\
"VSAD: Sum of absolute vertical differences\n"\
"VSSE: Sum of squared vertical differences\n"\
"NSSE: Noise preserving sum of squared differences"

/** Motion estimation */
#define PARAM_ME_CMP \
  {\
    name: "ff_me_cmp",                          \
    real_name: "ME compare function",           \
    type:  LQT_PARAMETER_STRINGLIST,                \
    val_default: { val_string: "SAD" },         \
    stringlist_options: COMPARE_FUNCS,          \
    help_string: "Motion estimation compare function.\n"COMPARE_FUNCS_HELP \
  }

/** Motion estimation */
#define PARAM_ME_CMP_CHROMA \
  { \
    name: "ff_me_cmp_chroma",                 \
    real_name: "Enable chroma ME compare",    \
    type: LQT_PARAMETER_INT,                  \
    val_default: { val_int: 0 },              \
    val_min:     { val_int: 0 },              \
    val_max:     { val_int: 1 },              \
  }

/** Motion estimation */
#define PARAM_ME_SUB_CMP                        \
  {\
    name: "ff_me_sub_cmp",                          \
    real_name: "Subpixel ME compare function",           \
    type:  LQT_PARAMETER_STRINGLIST,                \
    val_default: { val_string: "SAD" },         \
    stringlist_options: COMPARE_FUNCS,          \
    help_string: "Subpixel motion estimation compare function.\n"COMPARE_FUNCS_HELP \
  }

/** Motion estimation */
#define PARAM_ME_SUB_CMP_CHROMA \
  { \
    name: "ff_me_sub_cmp_chroma",                 \
    real_name: "Enable chroma subpixel ME compare",    \
    type: LQT_PARAMETER_INT,                  \
    val_default: { val_int: 0 },              \
    val_min:     { val_int: 0 },              \
    val_max:     { val_int: 1 },              \
  }


#define PARAM_MB_CMP \
  {\
    name: "ff_mb_cmp",                          \
    real_name: "MB compare function",           \
    type:  LQT_PARAMETER_STRINGLIST,                \
    val_default: { val_string: "SAD" },         \
    stringlist_options: COMPARE_FUNCS,          \
    help_string: "Macroblock compare function.\n"COMPARE_FUNCS_HELP \
  }

#define PARAM_MB_CMP_CHROMA \
  { \
    name: "ff_mb_cmp_chroma",                 \
    real_name: "Enable chroma macroblock ME compare",    \
    type: LQT_PARAMETER_INT,                  \
    val_default: { val_int: 0 },              \
    val_min:     { val_int: 0 },              \
    val_max:     { val_int: 1 },              \
  }


#define PARAM_ILDCT_CMP \
  {\
    name: "ff_ildct_cmp",                          \
    real_name: "ILDCT compare function",           \
    type:  LQT_PARAMETER_STRINGLIST,                \
    val_default: { val_string: "SAD" },         \
    stringlist_options: COMPARE_FUNCS,          \
    help_string: "Interlaced dct compare function.\n"COMPARE_FUNCS_HELP \
  }

#define PARAM_ICDCT_CMP_CHROMA \
  { \
    name: "ff_ildct_cmp_chroma",                 \
    real_name: "Enable chroma ILDCT ME compare",    \
    type: LQT_PARAMETER_INT,                  \
    val_default: { val_int: 0 },              \
    val_min:     { val_int: 0 },              \
    val_max:     { val_int: 1 },              \
  }

/** Motion estimation */
#define PARAM_DIA_SIZE \
  { \
    name: "ff_dia_size",                        \
    real_name: "ME diamond size & shape",\
    type: LQT_PARAMETER_INT,                  \
    val_default: { val_int: 0 },              \
    val_min:     { val_int: -9 },              \
    val_max:     { val_int:  9 },              \
    help_string: "Motion estimation diamond size. Negative means shape adaptive." \
  }

#define PARAM_LAST_PREDICTOR_COUNT \
  { \
    name: "ff_last_predictor_count",                        \
    real_name: "Last predictor count",\
    type: LQT_PARAMETER_INT,                  \
    val_default: { val_int: 0 },              \
    val_min:     { val_int: 0 },              \
    val_max:     { val_int: 99 },              \
    help_string: "Amount of motion predictors from the previous frame.\n"\
"0 (default)\n"\
"a Will use 2a+1 x 2a+1 macroblock square of motion vector predictors " \
"from the previous frame."\
  }

/** Motion estimation */
#define PARAM_PRE_ME \
  { \
    name: "ff_pre_me", \
    real_name: "ME pre-pass", \
    type: LQT_PARAMETER_INT,  \
    val_default: { val_int: 0 },              \
    val_min:     { val_int: 0 },              \
    val_max:     { val_int: 2 },              \
    help_string: "Motion estimation pre-pass\n"\
"0: disabled\n"\
"1: only after I-frames\n"\
"2: always" \
   }

/** Motion estimation */
#define PARAM_ME_PRE_CMP \
  {\
    name: "ff_me_pre_cmp",                          \
    real_name: "ME pre-pass compare function",           \
    type:  LQT_PARAMETER_STRINGLIST,                \
    val_default: { val_string: "SAD" },         \
    stringlist_options: COMPARE_FUNCS,          \
    help_string: "Motion estimation pre-pass compare function.\n"COMPARE_FUNCS_HELP\
  }

/** Motion estimation */
#define PARAM_ME_PRE_CMP_CHROMA \
  { \
    name: "ff_me_pre_cmp_chroma",                 \
    real_name: "Enable chroma ME pre-pass compare",    \
    type: LQT_PARAMETER_INT,                  \
    val_default: { val_int: 0 },              \
    val_min:     { val_int: 0 },              \
    val_max:     { val_int: 1 },              \
  }

/** Motion estimation */
#define PARAM_PRE_DIA_SIZE \
  { \
    name: "ff_pre_dia_size",                        \
    real_name: "ME pre-pass diamond size & shape",           \
    type: LQT_PARAMETER_INT,                  \
    val_default: { val_int: 0 },              \
    val_min:     { val_int: -9 },              \
    val_max:     { val_int:  9 },              \
    help_string: "Motion estimation pre-pass diamond size. Negative means shape adaptive." \
  }

/** Motion estimation */
#define PARAM_ME_SUBPEL_QUALITY \
  { \
    name: "ff_me_subpel_quality",                 \
    real_name: "Subpel ME quality",               \
    type: LQT_PARAMETER_INT,                  \
    val_default: { val_int: 8 },              \
    val_min:     { val_int: 1 },              \
    val_max:     { val_int: 8 },              \
    help_string: "Subpel motion estimation refinement quality (for qpel). Higher values "\
"mean higher quality but slower encoding." \
  }

/** Motion estimation */
#define PARAM_ME_RANGE \
  {                    \
    name: "ff_me_range",                          \
    real_name: "Motion estimation range",       \
    type: LQT_PARAMETER_INT,                    \
    val_default: { val_int: 0 },                \
    val_min: { val_int: 0 },                    \
    val_max: { val_int: 1000 },                 \
    help_string: "Motion estimation search range (0 means unlimited)",  \
  }

/** Motion estimation */
#define PARAM_MB_DECISION \
  {                       \
    name: "ff_mb_decision",                     \
    real_name: "MB decision mode",              \
    type: LQT_PARAMETER_STRINGLIST,             \
    val_default: { val_string: "Use compare function" },        \
    stringlist_options: (char*[]){ "Use compare function", \
                          "Fewest bits", "Rate distoration" },\
  }

/** Frame types */
#define PARAM_SCENE_CHANGE_THESHOLD \
  {                                 \
    name:      "ff_scenechange_threshold",        \
    real_name: "Scenechange theshold",            \
    type: LQT_PARAMETER_INT,             \
    val_default: { val_int: 0 },                \
    val_min: { val_int: -1000000000 },                    \
    val_max: { val_int: 1000000000 },                 \
    help_string: "Threshold for scene change detection.\n\
Negative values mean more sensitivity (more keyframes)" \
  }

/** Rate control */
#define PARAM_LMIN \
  {                \
    name: "ff_lmin", \
    real_name: "Minimum lagrange multipler",    \
    type: LQT_PARAMETER_FLOAT,             \
    val_default: { val_float: 2.0 }, \
    val_min:     {  val_float: 1.0 }, \
    val_min:     {  val_float: 31.0 }, \
    num_digits:  1, \
    help_string: "Minimum Lagrange multiplier for ratecontrol. "\
"Should possibly be the same as minimum quantizer scale."\
  }

/** Rate control */
#define PARAM_LMAX \
  {                \
    name: "ff_lmax", \
    real_name: "Maximum lagrange multipler",    \
    type: LQT_PARAMETER_FLOAT,             \
    val_default: { val_float: 31.0 }, \
    val_min:     {  val_float: 1.0 }, \
    val_min:     {  val_float: 31.0 }, \
    num_digits:  1, \
    help_string: "Maximum Lagrange multiplier for ratecontrol. "\
"Should possibly be the same as maximum quantizer scale."\
  }

#define PARAM_NOISE_REDUCTION \
  { \
    name: "ff_noise_reduction", \
    real_name: "Noise reduction", \
    type: LQT_PARAMETER_INT, \
    val_default: { val_int: 0 },            \
    val_min: { val_int: 0 },                \
    val_max: { val_int: 2000 },             \
  }

/** Rate control */
#define PARAM_RC_INITIAL_BUFFER_OCCUPANCY \
  { \
    name: "ff_rc_initial_buffer_occupancy",   \
    real_name: "Initial RC buffer occupancy",   \
    type: LQT_PARAMETER_INT,                \
    val_default: { val_int: 0 },            \
    help_string: "Number of kilobits which should be loaded into \
the rc buffer before encoding starts. Must not be larger than \
RC buffer size" \
  }


/* Does nothing */
/** Frame types */
#define PARAM_INTER_THRESHOLD \
  { \
    name: "ff_inter_threshold",   \
    real_name: "Inter theshold",   \
    type: LQT_PARAMETER_INT,                \
    val_default: { val_int: 0 },            \
  }

/** Quantizer */
#define PARAM_QUANTIZER_NOISE_SHAPING \
  { \
  name: "ff_quantizer_noise_shaping", \
    real_name: "Quantizer noise shaping",\
    type: LQT_PARAMETER_INT, \
    val_default: { val_int: 0 },\
    val_min:     { val_int: 0 },\
    val_max:     { val_int: 3 },\
    help_string: "Choose quantization such that noise will be masked by "\
"similar-frequency content in the image" \
  }

/** Motion estimation */
#define PARAM_ME_THRESHOLD \
  { \
    name: "ff_me_threshold", \
    real_name: "ME Theshold",                   \
    type: LQT_PARAMETER_INT, \
    val_default: { val_int: 0 },\
    val_min:     { val_int: 0 },\
    val_max:     { val_int: 4000000 },\
    help_string: "Motion estimation threshold. under which no motion estimation is performed, but instead the user specified motion vectors are used" \
  }

#define PARAM_MB_THRESHOLD \
  { \
    name: "ff_mb_threshold", \
    real_name: "MB Theshold",\
    type: LQT_PARAMETER_INT, \
    val_default: { val_int: 0 },\
    val_min:     { val_int: 0 },\
    val_max:     { val_int: 4000000 },\
    help_string: "Macroblock threshold. under which the user specified macroblock types will be used" \
  }

#define PARAM_NSSE_WEIGHT \
  { \
    name: "ff_nsse_weight", \
    real_name: "NSSE weight",                   \
    type: LQT_PARAMETER_INT,                    \
    val_default: { val_int: 8 },\
    help_string: "Noise vs. SSE weight for the NSSE comparsion function. "\
"0 is identical to SSE"\
  }

/** Masking */
#define PARAM_BORDER_MASKING \
  { \
    name: "ff_border_masking", \
    real_name: "Border masking", \
    type: LQT_PARAMETER_FLOAT,                    \
    val_default: { val_float: 0.0 }, \
    val_min:     { val_float: 0.0 }, \
    val_max:     { val_float: 1.0 }, \
    num_digits:  2, \
    help_string: "Encode image parts near the border with reduced quality."\
    " 0 means disabled"\
  }

#define PARAM_MB_LMIN \
  {                \
    name: "ff_mb_lmin", \
    real_name: "Minimum MB lagrange multipler",    \
    type: LQT_PARAMETER_FLOAT,             \
    val_default: { val_float: 2.0 }, \
    val_min:     {  val_float: 1.0 }, \
    val_min:     {  val_float: 31.0 }, \
    num_digits:  1, \
    help_string: "Minimum macroblock Lagrange multiplier." \
  }

#define PARAM_MB_LMAX \
  {                \
    name: "ff_mb_lmax", \
    real_name: "Maximum MB lagrange multipler",    \
    type: LQT_PARAMETER_FLOAT,             \
    val_default: { val_float: 31.0 }, \
    val_min:     {  val_float: 1.0 }, \
    val_min:     {  val_float: 31.0 }, \
    num_digits:  1, \
    help_string: "Maximum macroblock Lagrange multiplier." \
  }

/** Motion estimation */
#define PARAM_ME_PENALTY_COMPENSATION  \
  {                \
    name: "ff_me_penalty_compensation", \
    real_name: "ME penalty compensation",    \
    type: LQT_PARAMETER_INT,             \
    val_default: { val_int: 256 }, \
  }

#define PARAM_BIDIR_REFINE  \
  {                \
    name: "ff_bidir_refine", \
    real_name: "Bidir refine",    \
    type: LQT_PARAMETER_INT,             \
    val_default: { val_int: 0 }, \
    val_min:     { val_int: 0 }, \
    val_max:     { val_int: 4 }, \
  }

#define PARAM_BRD_SCALE  \
  {                \
    name: "ff_brd_scale", \
    real_name: "BRD scale",    \
    type: LQT_PARAMETER_INT,             \
    val_default: { val_int: 0 }, \
    val_min:     { val_int: 0 }, \
    val_max:     { val_int: 10 }, \
  }


/** Frame types */
#define PARAM_SCENECHANGE_FACTOR  \
  { \
    name: "ff_scenechange_factor", \
    real_name: "Scenechange factor",    \
    type: LQT_PARAMETER_INT,             \
    val_default: { val_int: 0 }, \
    help_string: "Multiplied by qscale for each frame and added to scene_change_score" \
  }

/* Flags */

/** Quantizer */
#define PARAM_FLAG_QSCALE \
  {                              \
    name:        "ff_flag_qscale", \
    real_name:   "Use fixed qscale",              \
    type:        LQT_PARAMETER_INT, \
    val_default: { val_int: 0 },    \
    val_min:     { val_int: 0 },    \
    val_max:     { val_int: 1 },    \
  }

/** Motion estimation */
#define PARAM_FLAG_4MV \
  {                              \
    name:        "ff_flag_4mv", \
    real_name:   "4 MV per MB allowed",      \
    type:        LQT_PARAMETER_INT, \
    val_default: { val_int: 0 },    \
    val_min:     { val_int: 0 },    \
    val_max:     { val_int: 1 },    \
    help_string: "Allow 4 motion vectors per macroblock (slightly better quality). Works better if MB decision mode is \"Fewest bits\" or \"Rate distoration\"." \
  }

/** Motion estimation */
#define PARAM_FLAG_QPEL \
  {                              \
    name:        "ff_flag_qpel", \
    real_name:   "Use qpel MC",      \
    type:        LQT_PARAMETER_INT, \
    val_default: { val_int: 0 },    \
    val_min:     { val_int: 0 },    \
    val_max:     { val_int: 1 },    \
    help_string: "Use 1/4 pixel motion compensation.  Warning: QPEL is not supported by all decoders." \
  }

/** Motion estimation */
#define PARAM_FLAG_GMC \
  {                              \
    name:        "ff_flag_gmc", \
    real_name:   "Use global motion compensation",      \
    type:        LQT_PARAMETER_INT, \
    val_default: { val_int: 0 },    \
    val_min:     { val_int: 0 },    \
    val_max:     { val_int: 1 },    \
    help_string: "Warning: GMC is not supported by all decoders" \
  }

#define PARAM_FLAG_PART \
  {                              \
    name:        "ff_flag_part", \
    real_name:   "Use data partitioning",      \
    type:        LQT_PARAMETER_INT, \
    val_default: { val_int: 0 },    \
    val_min:     { val_int: 0 },    \
    val_max:     { val_int: 1 },    \
    help_string: "Use data partitioning for more robustnedd if the video is "\
"for transmitting over unreliable channels" \
  }


#define PARAM_FLAG_GRAY \
  {                              \
    name:        "ff_flag_gray", \
    real_name:   "Grayscale mode",      \
    type:        LQT_PARAMETER_INT, \
    val_default: { val_int: 0 },    \
    val_min:     { val_int: 0 },    \
    val_max:     { val_int: 1 },    \
  }

#define PARAM_FLAG_EMU_EGDE \
  {                              \
    name:        "ff_flag_emu_edge", \
    real_name:   "Don't draw edges",      \
    type:        LQT_PARAMETER_INT, \
    val_default: { val_int: 0 },    \
    val_min:     { val_int: 0 },    \
    val_max:     { val_int: 1 },    \
  }

/** Masking */
#define PARAM_FLAG_NORMALIZE_AQP   \
  {                              \
    name:        "ff_flag_normalize_aqp", \
    real_name:   "Normalize adaptive quantization",      \
    type:        LQT_PARAMETER_INT, \
    val_default: { val_int: 0 },    \
    val_min:     { val_int: 0 },    \
    val_max:     { val_int: 1 },    \
    help_string: "When using masking, try to adjust the per "\
    "macroblock quantizers to maintain the desired average" \
  }

#define PARAM_FLAG_ALT_SCAN   \
  {                              \
    name:        "ff_flag_alt_scan", \
    real_name:   "Use alternative scantable",      \
    type:        LQT_PARAMETER_INT, \
    val_default: { val_int: 0 },    \
    val_min:     { val_int: 0 },    \
    val_max:     { val_int: 1 },    \
  }

/** Quantizer */
#define PARAM_FLAG_TRELLIS_QUANT   \
  {                              \
    name:        "ff_flag_trellis_quant", \
    real_name:   "Use trellis quantization",      \
    type:        LQT_PARAMETER_INT, \
    val_default: { val_int: 0 },    \
    val_min:     { val_int: 0 },    \
    val_max:     { val_int: 1 },    \
    help_string: "Use trellis quantization (improves quality)" \
  }

#define PARAM_FLAG_BITEXACT   \
  {                              \
    name:        "ff_flag_bitexact", \
    real_name:   "Use only bitexact stuff",      \
    type:        LQT_PARAMETER_INT, \
    val_default: { val_int: 0 },    \
    val_min:     { val_int: 0 },    \
    val_max:     { val_int: 1 },    \
    help_string: "Use only bitexact stuff (except (i)dct)" \
  }

#define PARAM_FLAG_AC_PRED_H263   \
  {                              \
    name:        "ff_flag_ac_pred", \
    real_name:   "Advanced intra coding",  \
    type:        LQT_PARAMETER_INT, \
    val_default: { val_int: 0 },    \
    val_min:     { val_int: 0 },    \
    val_max:     { val_int: 1 },    \
  }

#define PARAM_FLAG_AC_PRED_MPEG4   \
  {                              \
    name:        "ff_flag_ac_pred", \
    real_name:   "MPEG-4 AC prediction",  \
    type:        LQT_PARAMETER_INT, \
    val_default: { val_int: 0 },    \
    val_min:     { val_int: 0 },    \
    val_max:     { val_int: 1 },    \
  }

#define PARAM_FLAG_H263P_UMV   \
  {                              \
    name:        "ff_flag_h263p_umv", \
    real_name:   "Unlimited motion vector",  \
    type:        LQT_PARAMETER_INT, \
    val_default: { val_int: 0 },    \
    val_min:     { val_int: 0 },    \
    val_max:     { val_int: 1 },    \
  }

#define PARAM_FLAG_CBP_RD \
  {                       \
    name: "ff_flag_cbp_rd", \
    real_name: "CBP RD", \
    type:        LQT_PARAMETER_INT, \
    val_default: { val_int: 0 },    \
    val_min:     { val_int: 0 },    \
    val_max:     { val_int: 1 },    \
    help_string: "Use rate distortion optimization for cbp",\
  }

#define PARAM_FLAG_QP_RD \
  {                      \
    name: "ff_flag_qp_rd", \
    real_name: "QP RD", \
    type:        LQT_PARAMETER_INT, \
    val_default: { val_int: 0 },    \
    val_min:     { val_int: 0 },    \
    val_max:     { val_int: 1 },    \
    help_string: "use rate distortion optimization for qp selectioon",\
  }

#define PARAM_FLAG_H263P_AIV \
  {                      \
    name: "ff_flag_h263p_aiv", \
    real_name: "Alternative inter vlc", \
    type:        LQT_PARAMETER_INT, \
    val_default: { val_int: 0 },    \
    val_min:     { val_int: 0 },    \
    val_max:     { val_int: 1 },    \
  }

/* H.263(+) */
#define PARAM_FLAG_OBMC \
  { \
    name: "ff_flag_obmc", \
    real_name: "OBMC", \
    type:        LQT_PARAMETER_INT, \
    val_default: { val_int: 0 },    \
    val_min:     { val_int: 0 },    \
    val_max:     { val_int: 1 },    \
    help_string: "Overlapped block motion compensation (only supported with with simple MB decision)" \
  }

#define PARAM_FLAG_LOOP_FILTER              \
  { \
    name: "ff_flag_loop_filter", \
    real_name: "Loop filter", \
    type:        LQT_PARAMETER_INT, \
    val_default: { val_int: 0 },    \
    val_min:     { val_int: 0 },    \
    val_max:     { val_int: 1 },    \
  }

#define PARAM_FLAG_H263P_SLICE_STRUCT      \
  { \
    name: "ff_flag_h263p_slice_struct", \
    real_name: "Slice struct", \
    type:        LQT_PARAMETER_INT, \
    val_default: { val_int: 0 },    \
    val_min:     { val_int: 0 },    \
    val_max:     { val_int: 1 },    \
  }

/** Frame types */
#define PARAM_FLAG_CLOSED_GOP \
  { \
    name: "ff_flag_closed_gop", \
    real_name: "Close all GOPs", \
    type:        LQT_PARAMETER_INT, \
    val_default: { val_int: 0 },    \
    val_min:     { val_int: 0 },    \
    val_max:     { val_int: 1 },    \
  }

#define PARAM_FLAG2_FAST \
  { \
    name: "ff_flag2_fast", \
    real_name: "Allow fast encoding", \
    type:        LQT_PARAMETER_INT, \
    val_default: { val_int: 0 },    \
    val_min:     { val_int: 0 },    \
    val_max:     { val_int: 1 },    \
    help_string: "Allow non spec compliant speedup tricks" \
}

/** Frame types */
#define PARAM_FLAG2_STRICT_GOP                     \
  { \
    name: "ff_flag2_strict_gop", \
    real_name: "Strictly enforce GOP size", \
    type:        LQT_PARAMETER_INT, \
    val_default: { val_int: 0 },    \
    val_min:     { val_int: 0 },    \
    val_max:     { val_int: 1 },    \
}

