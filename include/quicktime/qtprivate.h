#ifndef PRIVATE_H
#define PRIVATE_H

/* ================================= structures */


/* Version used internally.  You need to query it with the C functions */
/* These must match quicktime4linux !!! */
#define QUICKTIME_MAJOR   2
#define QUICKTIME_MINOR   0
#define QUICKTIME_RELEASE 0

#define HEADER_LENGTH 8
#define MAXTRACKS 1024
#define MAXNODES 1

/* Crazy Mich R. Soft constants */
#define AVI_HASINDEX       0x00000010  // Index at end of file?
#define AVI_MUSTUSEINDEX   0x00000020
#define AVI_ISINTERLEAVED  0x00000100
#define AVI_TRUSTCKTYPE    0x00000800  // Use CKType to find key frames?
#define AVI_WASCAPTUREFILE 0x00010000
#define AVI_COPYRIGHTED    0x00020000
#define AVIF_WASCAPTUREFILE     0x00010000
#define AVI_KEYFRAME       0x10
#define AVI_INDEX_OF_CHUNKS 0x01
#define AVI_INDEX_OF_INDEXES 0x00
                                                                                                                     
#define AVI_FRAME_RATE_BASE 10000
#define MAX_RIFFS  0x100

#define QTVR_OBJ 2
#define QTVR_PAN 3
#define QTVR_QTVR 1

#define QTVR_GRABBER_SCROLLER_UI 1
#define QTVR_OLD_JOYSTICK_UI 2 
#define QTVR_JOYSTICK_UI 3
#define QTVR_GRABBER_UI 4
#define QTVR_ABSOLUTE_UI 5


//#include "codecs.h"
#include <stdio.h>
#include <inttypes.h>

/* Forward declarations */

typedef struct quicktime_strl_s quicktime_strl_t;

typedef struct
{
/* for AVI it's the end of the 8 byte header in the file */
/* for Quicktime it's the start of the 8 byte header in the file */
	int64_t start;
	int64_t end;        /* byte endpoint in file */
	int64_t size;       /* byte size for writing */
	int use_64;         /* Use 64 bit header */
	unsigned char type[4];
} quicktime_atom_t;


typedef struct
{
	int64_t start;
	int64_t end;        /* byte endpoint in file */
	int64_t size;       /* byte size for writing */
	unsigned char type[4];
	int child_count;
	int use_64;
	long ID;
} quicktime_qtatom_t;

typedef struct
{
	float values[9];
} quicktime_matrix_t;


typedef struct
{
	int version;
	long flags;
	unsigned long creation_time;
	unsigned long modification_time;
	int track_id;
	long reserved1;
	long duration;
	uint8_t reserved2[8];
	int layer;
	int alternate_group;
	float volume;
	long reserved3;
	quicktime_matrix_t matrix;
	float track_width;
	float track_height;
} quicktime_tkhd_t;


typedef struct
{
	long seed;
	long flags;
	long size;
	unsigned short int *alpha;
	unsigned short int *red;
	unsigned short int *green;
	unsigned short int *blue;
} quicktime_ctab_t;



/* ===================== sample table ======================== // */



/* sample description */

typedef struct
{
	int motion_jpeg_quantization_table;
} quicktime_mjqt_t;


typedef struct
{
	int motion_jpeg_huffman_table;
} quicktime_mjht_t;

typedef struct
{
	int32_t hSpacing;
	int32_t vSpacing;
} quicktime_pasp_t;

typedef struct
{
	int32_t colorParamType;
	int16_t primaries;
	int16_t transferFunction;
	int16_t matrix;
} quicktime_colr_t;

typedef struct
{
	int32_t cleanApertureWidthN;
	int32_t cleanApertureWidthD;
	int32_t cleanApertureHeightN;
	int32_t cleanApertureHeightD;
	int32_t horizOffN;
	int32_t horizOffD;
	int32_t vertOffN;
	int32_t vertOffD;
} quicktime_clap_t;

typedef struct
{
    int version;
    int revision;
    long imagingMode;
    long imagingValidFlags;
    long correction;
    long quality;
    long directdraw;
    long imagingProperties[5];
    long reserved1;
    long reserved2;
} quicktime_impn_t;

typedef struct
{
    quicktime_impn_t impn;
} quicktime_imgp_t;

typedef struct
{
	long reserved1;
	long reserved2;
	int version;
	int revision;
	
	long STrack; /* Prefix 'S' == Scene */
	long LowResSTrack;
	uint32_t reserved3[6];
	long HSTrack; /* Prefix 'HS' == HotSpot */
	uint32_t reserved4[9];
	
	float HPanStart;
	float HPanEnd;
	float VPanStart;
	float VPanEnd;
	float MinZoom;
	float MaxZoom;
	
	long SHeight;
	long SWidth;
	long NumFrames;
	int reserved5;
	int SNumFramesHeight;
	int SNumFramesWidth;
	int SDepth;
	
	long HSHeight;
	long HSWidth;
	int reserved6;
	int HSNumFramesHeight;
	int HSNumFramesWidth;
	int HSDepth;
} quicktime_pano_t;

typedef struct
{
	int version;
	int revision;
	char nodeType[4];
	long locationFlags;
	long locationData;
	long reserved1;
	long reserved2;
} quicktime_nloc_t;

typedef struct
{
	int version;
	int revision;
	char nodeType[4];
	long nodeID;
	long nameAtomID;
	long commentAtomID;
	long reserved1;
	long reserved2;
} quicktime_ndhd_t;

typedef struct
{
	quicktime_nloc_t nloc;
	int ID;
} quicktime_vrni_t;

typedef struct
{
	quicktime_vrni_t vrni[MAXNODES];
	int children;
} quicktime_vrnp_t;

typedef struct
{
	int version;
	int revision;
	long NameAtomID;
	long DefaultNodeID;
	long flags;
	long reserved1;
	long reserved2;
	
} quicktime_vrsc_t;

typedef struct
{
	quicktime_vrsc_t vrsc;
	quicktime_imgp_t imgp;
	quicktime_vrnp_t vrnp;
} quicktime_qtvr_t;

/* wave atom and subatoms */

typedef struct
  {
  char codec[4];
  /* Remainder could be a WAVEFORMATEX structure */
  } quicktime_frma_t;

typedef struct
  {
  int16_t littleEndian;
  } quicktime_enda_t;

typedef struct
  {
  quicktime_frma_t frma;
  int has_frma;
  quicktime_enda_t enda;
  int has_enda;
  } quicktime_wave_t;

typedef struct
{
	char format[4];
	uint8_t reserved[6];
	int data_reference;

/* common to audio and video */
	int version;
	int revision;
	char vendor[4];

/* video description */
	long temporal_quality;
	long spatial_quality;
	int width;
	int height;
	float dpi_horizontal;
	float dpi_vertical;
	int64_t data_size;
	int frames_per_sample;
	char compressor_name[32];
	int depth;
	int ctab_id;
	quicktime_ctab_t ctab;
	float gamma;
	int fields;    /* 0, 1, or 2 */
	int field_dominance;   /* 0 - unknown     1 - top first     2 - bottom first */
	quicktime_mjqt_t mjqt;
	quicktime_mjht_t mjht;
	quicktime_pasp_t pasp;
	quicktime_colr_t colr;
	quicktime_clap_t clap;
	quicktime_pano_t pano;
	quicktime_qtvr_t qtvr;

        quicktime_wave_t wave;
/* audio description */
	int channels;
	int sample_size;
/* LQT: We have int16_t for the compression_id, because otherwise negative
   values don't show up correctly */
        int16_t compression_id;
	int packet_size;
	float sample_rate;

/* Audio extension for version == 1 */

        uint32_t audio_samples_per_packet;
        uint32_t audio_bytes_per_packet;
        uint32_t audio_bytes_per_frame;
        uint32_t audio_bytes_per_sample;

/* LQT: We store the complete atom (starting with the fourcc)
   here, because this must be passed to the Sorenson 3 decoder */

        unsigned char * extradata;
        int extradata_size;

} quicktime_stsd_table_t;


typedef struct
{
	int version;
	long flags;
	long total_entries;
	quicktime_stsd_table_t *table;
} quicktime_stsd_t;


/* time to sample */
typedef struct
{
	long sample_count;
	long sample_duration;
} quicktime_stts_table_t;

typedef struct
{
	int version;
	long flags;
	long total_entries;
        long entries_allocated;
        int  default_duration;
	quicktime_stts_table_t *table;
} quicktime_stts_t;


/* sync sample */
typedef struct
{
	long sample;
} quicktime_stss_table_t;

typedef struct
{
	int version;
	long flags;
	long total_entries;
	long entries_allocated;
	quicktime_stss_table_t *table;
} quicktime_stss_t;


/* sample to chunk */
typedef struct
{
	long chunk;
	long samples;
	long id;
} quicktime_stsc_table_t;

typedef struct
{
	int version;
	long flags;
	long total_entries;
	
	long entries_allocated;
	quicktime_stsc_table_t *table;
} quicktime_stsc_t;


/* sample size */
typedef struct
{
	int64_t size;
} quicktime_stsz_table_t;

typedef struct
{
	int version;
	long flags;
	int64_t sample_size;
	long total_entries;

	long entries_allocated;    /* used by the library for allocating a table */
	quicktime_stsz_table_t *table;
} quicktime_stsz_t;


/* chunk offset */
typedef struct
{
	int64_t offset;
} quicktime_stco_table_t;

typedef struct
{
	int version;
	long flags;
	long total_entries;
	
	long entries_allocated;    /* used by the library for allocating a table */
	quicktime_stco_table_t *table;
} quicktime_stco_t;


/* sample table */
typedef struct
{
	int version;
	long flags;
	quicktime_stsd_t stsd;
	quicktime_stts_t stts;
	quicktime_stss_t stss;
	quicktime_stsc_t stsc;
	quicktime_stsz_t stsz;
	quicktime_stco_t stco;
} quicktime_stbl_t;

typedef struct
{
    	char refType[4];
	long trackIndex;
} quicktime_tref_t;

/* data reference */

typedef struct
{
	int64_t size;
	char type[4];
	int version;
	long flags;
	char *data_reference;
} quicktime_dref_table_t;

typedef struct
{
	int version;
	long flags;
	long total_entries;
	quicktime_dref_table_t *table;
} quicktime_dref_t;

/* data information */

typedef struct
{
	quicktime_dref_t dref;
} quicktime_dinf_t;

/* video media header */

typedef struct
{
	int version;
	long flags;
	int graphics_mode;
	int opcolor[3];
} quicktime_vmhd_t;


/* sound media header */

typedef struct
{
	int version;
	long flags;
	int balance;
	int reserved;
} quicktime_smhd_t;


/* Base media info */

typedef struct
{
	int version;
	long flags;
	int graphics_mode;
	int opcolor[3];
	int balance;
	int reserved;
} quicktime_gmin_t;


/* Base media header */

typedef struct
{
    	quicktime_gmin_t gmin;
} quicktime_gmhd_t;


/* handler reference */

typedef struct
{
	int version;
	long flags;
	char component_type[4];
	char component_subtype[4];
long component_manufacturer;
	long component_flags;
	long component_flag_mask;
	char component_name[256];
} quicktime_hdlr_t;

/* media information */

typedef struct
{
	int is_video;
	int is_audio;
	int has_baseheader;
	int is_panorama;
	int is_qtvr;
	int is_object;
	quicktime_vmhd_t vmhd;
	quicktime_smhd_t smhd;
	quicktime_gmhd_t gmhd;
	quicktime_stbl_t stbl;
	quicktime_hdlr_t hdlr;
	quicktime_dinf_t dinf;
} quicktime_minf_t;


/* media header */

typedef struct
{
	int version;
	long flags;
	unsigned long creation_time;
	unsigned long modification_time;
	long time_scale;
	long duration;
	int language;
	int quality;
} quicktime_mdhd_t;


/* media */

typedef struct
{
	quicktime_mdhd_t mdhd;
	quicktime_minf_t minf;
	quicktime_hdlr_t hdlr;
} quicktime_mdia_t;

/* edit list */
typedef struct
{
	long duration;
	long time;
	float rate;
} quicktime_elst_table_t;

typedef struct
{
	int version;
	long flags;
	long total_entries;

	quicktime_elst_table_t *table;
} quicktime_elst_t;

typedef struct
{
	quicktime_elst_t elst;
} quicktime_edts_t;


/* qtvr navg (v1.0) */
typedef struct {
    int    version;        // Always 1
    int    columns;    // Number of columns in movie
    int    rows;        // Number rows in movie
    int    reserved;        // Zero
    int    loop_frames;        // Number of frames shot at each position
    int    loop_dur;        // The duration of each frame
    int    movietype;        // kStandardObject, kObjectInScene, or
                    // kOldNavigableMovieScene
    int    loop_timescale;        // Number of ticks before next frame of
                    // loop is displayed
    float    fieldofview;        // 180.0 for kStandardObject or
                    // kObjectInScene, actual  degrees for
                    // kOldNavigableMovieScene.
    float    startHPan;        // Start horizontal pan angle in
                    //  degrees
    float    endHPan;        // End horizontal pan angle in  degrees
    float    endVPan;        // End vertical pan angle in  degrees
    float    startVPan;        // Start vertical pan angle in  degrees
    float    initialHPan;        // Initial horizontal pan angle in
                    //  degrees (poster view)
    float    initialVPan;        // Initial vertical pan angle in  degrees
                    // (poster view)
    long    reserved2;        // Zero
} quicktime_navg_t;


typedef struct
{
	quicktime_tkhd_t tkhd;
	quicktime_mdia_t mdia;
	quicktime_edts_t edts;
	quicktime_tref_t tref;
        quicktime_strl_t * strl; // != NULL for AVI files during reading and writing
        int chunk_sizes_alloc;
        int64_t * chunk_sizes; /* This contains the chunk sizes for audio
                                  tracks. They can not so easily be obtained
                                  during decoding */
	int has_tref;
} quicktime_trak_t;


typedef struct
{
	int version;
	long flags;
	unsigned long creation_time;
	unsigned long modification_time;
	long time_scale;
	long duration;
	float preferred_rate;
	float preferred_volume;
	uint8_t reserved[10];
	quicktime_matrix_t matrix;
	long preview_time;
	long preview_duration;
	long poster_time;
	long selection_time;
	long selection_duration;
	long current_time;
	long next_track_id;
} quicktime_mvhd_t;


typedef struct
{
	char *copyright;
	int copyright_len;
	char *name;
	int name_len;
	char *info;
	int info_len;
/* Additional Metadata for libquicktime */
        char *album;
        int album_len;
        char *author;
        int author_len;
        char *artist;
        int artist_len;
        char *genre;
        int genre_len;
        char *track;
        int track_len;
        char *comment;
        int comment_len;
	int is_qtvr;
	/* player controls */
	char ctyp[4];
	quicktime_navg_t navg;
} quicktime_udta_t;


typedef struct
{
	int total_tracks;

	quicktime_mvhd_t mvhd;
	quicktime_trak_t *trak[MAXTRACKS];
	quicktime_udta_t udta;
	quicktime_ctab_t ctab;
} quicktime_moov_t;

typedef struct
{
	quicktime_atom_t atom;
} quicktime_mdat_t;

typedef struct
{
/* Offset of end of 8 byte chunk header relative to ix->base_offset */
        int relative_offset;
/* size of data without 8 byte header */
        int size;
} quicktime_ixtable_t;
                                                                                                                     
typedef struct
{
        quicktime_atom_t atom;
        quicktime_ixtable_t *table;
        int table_size;
        int table_allocation;
        int longs_per_entry;
        int index_type;
/* ixtable relative_offset is relative to this */
        int64_t base_offset;
/* ix atom title */
        char tag[5];
/* corresponding chunk id */
        char chunk_id[5];
} quicktime_ix_t;
typedef struct
{
        quicktime_atom_t atom;
                                                                                                                     
/* Partial index */
/* For writing only, there are multiple movi objects with multiple ix tables. */
/* This is not used for reading.  Instead an ix_t object in indx_t is used. */
        quicktime_ix_t *ix[MAXTRACKS];
} quicktime_movi_t;
                                                                                                                     
typedef struct
{
/* Start of start of corresponding ix## header */
        int64_t index_offset;
/* Size not including 8 byte header */
        int index_size;
/* duration in "ticks" */
        int duration;
                                                                                                                     
/* Partial index for reading only. */
        quicktime_ix_t *ix;
} quicktime_indxtable_t;

typedef struct
{
        quicktime_atom_t atom;
        int longs_per_entry;
        int index_subtype;
        int index_type;
/* corresponding chunk id: 00wb, 00dc */
        char chunk_id[5];
                                                                                                                     
/* Number of partial indexes here */
        int table_size;
        int table_allocation;
        quicktime_indxtable_t *table;
} quicktime_indx_t;

struct quicktime_strl_s
{
        quicktime_atom_t atom;
/* Super index for reading */
        quicktime_indx_t indx;
/* LIBQUICKTIME: These are the important values to make proper audio
   headers. They are set by the ENCODER in the encode-functions before
   writing the first audio chunk (i.e. after a call to quicktime_set_avi) */

        /* strh stuff */

        int64_t  dwScaleOffset;
        uint32_t dwScale;

        uint32_t dwRate;

        int64_t  dwLengthOffset;

        int64_t dwSampleSizeOffset;
        uint32_t dwSampleSize;

        /* WAVEFORMATEX stuff */

        uint16_t nBlockAlign;

        int64_t nAvgBytesPerSecOffset;
        uint32_t nAvgBytesPerSec;

        uint16_t wBitsPerSample;

        /* The next ones will be used for generating index tables
           with the lowest possible error */

        int64_t total_bytes;
        int64_t total_samples;
        int64_t total_blocks;

/* Start of indx header for later writing */
        int64_t indx_offset;
/* Size of JUNK without 8 byte header which is to be replaced by indx */
        int64_t padding_size;
/* Tag for writer with NULL termination: 00wb, 00dc   Not available in reader.*/
        char tag[5];
/* Flags for reader.  Not available in writer. */
        int is_audio;
        int is_video;
/* Notify reader the super indexes are valid */
        int have_indx;
};

typedef struct
{
        quicktime_atom_t atom;
        int64_t frames_offset;
        int64_t bitrate_offset;
/* Offsets to be written during file closure */
        int64_t total_frames_offset;
                                                                                                                     
/* AVI equivalent for each trak.  Use file->moov.total_tracks */
/* Need it for super indexes during reading. */
        quicktime_strl_t *strl[MAXTRACKS];
} quicktime_hdrl_t;

typedef struct
{
        char tag[5];
        uint32_t flags;
/* Start of 8 byte chunk header relative to start of the 'movi' string */
        int32_t offset;
/* Size of chunk less the 8 byte header */
        int32_t size;
} quicktime_idx1table_t;

typedef struct
{
        quicktime_atom_t atom;
        quicktime_idx1table_t *table;
        int table_size;
        int table_allocation;
} quicktime_idx1_t;
                                                                                                                     
typedef struct
{
        quicktime_atom_t atom;
        quicktime_movi_t movi;
        quicktime_hdrl_t hdrl;
                                                                                                                     
/* Full index */
        quicktime_idx1_t idx1;
/* Notify reader the idx1 table is valid */
        int have_idx1;
        int have_hdrl;
} quicktime_riff_t;


/* table of pointers to every track */
typedef struct
{
	quicktime_trak_t *track; /* real quicktime track corresponding to this table */
	int channels;            /* number of audio channels in the track */
	int64_t current_position;   /* current sample in output file */
	int64_t current_chunk;      /* current chunk in output file */

        /* Last position if set by the codec. If last position and current position
           differ, the codec knows, that a seek happened since the last decode call */
        long last_position;
	void *codec;

        int eof; /* This is set to 1 by the core if one tries to read beyond EOF */

/* Another API enhancement: Codecs only deliver samples in the format specified by
   sample_format. The usual decode() functions will convert them to int16_t or float */
   
        lqt_sample_format_t sample_format; /* Set by the codec */

        uint8_t * sample_buffer;
        int sample_buffer_alloc;  /* Allocated size in SAMPLES of the sample buffer */
        
} quicktime_audio_map_t;

typedef struct
{
	quicktime_trak_t *track;
	long current_position;   /* current frame in output file */
	long current_chunk;      /* current chunk in output file */

	void *codec;
/* Timestamp of the NEXT frame decoded. Unit is the timescale of the track */
/* For Encoding: Timestamp of the LAST encoded frame */
        int64_t timestamp;
        int64_t stts_index;
        int64_t stts_count;
        int stream_cmodel;  /* Colormodel, which is read/written natively by the codec  */
        int stream_row_span, stream_row_span_uv;

        int io_cmodel;      /* Colormodel, which is used by the encode/decode functions */
        int io_row_span, io_row_span_uv;

        /* This is used by the core to do implicit colorspace conversion and scaling (NOT recommended!!) */
        uint8_t ** temp_frame;
        lqt_chroma_placement_t chroma_placement;
        lqt_interlace_mode_t interlace_mode;
} quicktime_video_map_t;

/* obji */

typedef struct
{
    int version;
    int revision;
    int movieType;
    int viewStateCount;
    int defaultViewState;
    int mouseDownViewState;
    long viewDuration;
    long columns;
    long rows;
    float mouseMotionScale;
    float minPan;
    float maxPan;
    float defaultPan;
    float minTilt;
    float maxTilt;
    float defaultTilt;
    float minFOV;
    float FOV;
    float defaultFOV;
    float defaultViewCenterH;
    float defaultViewCenterV;
    float viewRate;
    float frameRate;
    long animSettings;
    long controlSettings;
} quicktime_obji_t;

/* pHdr */

typedef struct
{
    unsigned long    nodeID;
    float        defHPan;
    float        defVPan;
    float        defZoom;

    // constraints for this node; use zero for default
    float        minHPan;
    float        minVPan;
    float        minZoom;
    float        maxHPan;
    float        maxVPan;
    float        maxZoom;

    long        reserved1;        // must be zero
    long        reserved2;        // must be zero
    long        nameStrOffset;        // offset into string table atom
    long        commentStrOffset;    // offset into string table atom
} quicktime_pHdr_t;

/* qtvr node */

typedef struct
{
    	int node_type;
	int64_t node_start; /* start frame */ 
	int64_t node_size; /* size of node in frames */
	quicktime_ndhd_t ndhd;
	quicktime_obji_t obji;
	} quicktime_qtvr_node_t;

/* file descriptor passed to all routines */

typedef struct
{
	FILE *stream;
	int64_t total_length;
	quicktime_mdat_t mdat;
	quicktime_moov_t moov;
	int rd;
	int wr;

/* If the moov atom is compressed */
        int compressed_moov;
        unsigned char *moov_data;
/*
 * Temporary storage of compressed sizes.  If the file length is shorter than the
 * uncompressed sizes, it won't work.
 */
        int64_t moov_end;
        int64_t moov_size;
        int64_t old_preload_size;
        uint8_t *old_preload_buffer;
        int64_t old_preload_start;
        int64_t old_preload_end;
        int64_t old_preload_ptr;

/* ASF section */
        int use_asf;

        int use_avi;
/* AVI tree */
        quicktime_riff_t *riff[MAX_RIFFS];
        int total_riffs;


/* for begining and ending frame writes where the user wants to write the  */
/* file descriptor directly */
	int64_t offset;
/* I/O */
/* Current position of virtual file descriptor */
	int64_t file_position;      
// Work around a bug in glibc where ftello returns only 32 bits by maintaining
// our own position
	int64_t ftell_position;

/* Read ahead buffer */
	int64_t preload_size;      /* Enables preload when nonzero. */
	uint8_t *preload_buffer;
	int64_t preload_start;     /* Start of preload_buffer in file */
	int64_t preload_end;       /* End of preload buffer in file */
	int64_t preload_ptr;       /* Offset of preload_start in preload_buffer */

/* Write ahead buffer */
/* Amount of data in presave buffer */
        int64_t presave_size;
/* Next presave byte's position in file */
        int64_t presave_position;
        uint8_t *presave_buffer;
/* Presave doesn't matter a whole lot, so its size is fixed */
#define QUICKTIME_PRESAVE 0x100000

/* mapping of audio channels to movie tracks */
/* one audio map entry exists for each channel */
	int total_atracks;
	quicktime_audio_map_t *atracks;

/* mapping of video tracks to movie tracks */
	int total_vtracks;
	quicktime_video_map_t *vtracks;

/* Number of processors at our disposal */
	int cpus;

/* Parameters for frame currently being decoded */
	int in_x, in_y, in_w, in_h, out_w, out_h;
/*      Libquicktime change: color_model and row_span are now saved per track */
/*	int color_model, row_span; */

	quicktime_qtvr_node_t qtvr_node[MAXNODES];
} quicktime_t;

typedef struct
{
	int (*delete_vcodec)(quicktime_video_map_t *vtrack);
	int (*delete_acodec)(quicktime_audio_map_t *atrack);
	int (*decode_video)(quicktime_t *file, 
				unsigned char **row_pointers, 
				int track);
	int (*encode_video)(quicktime_t *file, 
				unsigned char **row_pointers, 
				int track);
        /* API Change: Return value is the number of samples */
        int (*decode_audio)(quicktime_t *file, 
                            void * output,
                            long samples, 
                            int track);
	int (*encode_audio)(quicktime_t *file, 
                            void * input,
                            long samples,
                            int track);
	int (*set_parameter)(quicktime_t *file, 
		int track, 
		char *key, 
		void *value);
	void (*flush)(quicktime_t *file, 
		int track);

        /* AVI codec ID for audio.  AVI codec ID's are based on WAV files, by the way. */
        int wav_id;
                                                                                   
/* Pointer to static character code for identifying the codec. */
        char *fourcc;
                                                                                   
/* English title of codec.  Optional. */
        char *title;
                                                                                   
/* English description of codec.  Optional. */
        char *desc;

	void *priv;

        /* The followings are for libquicktime only */
        void *module;     /* Needed by libquicktime for dynamic loading */
        char *codec_name; /* Needed by libquicktime */
} quicktime_codec_t;

#endif
