#include <string.h>

#include <quicktime/quicktime.h>
#include <funcprotos.h>

#define MK_FOURCC(a, b, c, d) \
  (((uint32_t)a << 24) |      \
   ((uint32_t)b << 16) |      \
   ((uint32_t)c << 8) |       \
   ((uint32_t)d))

#if 0
  uint32_t major_brand;
  uint32_t minor_version;
  int num_compatible_brands;
  uint32_t * compatible_brands;
#endif

static quicktime_ftyp_t ftyp_qt =
  {
    major_brand:           MK_FOURCC('q','t',' ',' '),
    minor_version:         0x20050300,
    num_compatible_brands: 4,
    compatible_brands:     (uint32_t[]){ MK_FOURCC('q','t',' ',' '),0,0,0 },
  };

static quicktime_ftyp_t ftyp_mp4 =
  {
    major_brand:           MK_FOURCC('m','p','4','2'),
    minor_version:         0x0,
    num_compatible_brands: 4,
    compatible_brands:     (uint32_t[]){MK_FOURCC('m','p','4','2'),0,0,0},
  };

static quicktime_ftyp_t ftyp_m4a =
  {
    major_brand:           MK_FOURCC('M','4','A',' '),
    minor_version:         0x0,
    num_compatible_brands: 4,
    compatible_brands:     (uint32_t[]){MK_FOURCC('M','4','A',' '),
                                        MK_FOURCC('m','p','4','2'),
                                        MK_FOURCC('i','s','o','m'),
                                        0},
  };

static void copy_ftyp(quicktime_ftyp_t * dst, quicktime_ftyp_t * src)
  {
  dst->major_brand = src->major_brand;
  dst->minor_version = src->minor_version;
  dst->num_compatible_brands = src->num_compatible_brands;
  dst->compatible_brands = malloc(dst->num_compatible_brands *
                                  sizeof(*dst->compatible_brands));
  memcpy(dst->compatible_brands, src->compatible_brands,
         dst->num_compatible_brands * sizeof(*dst->compatible_brands));
  }

void quicktime_ftyp_init(quicktime_ftyp_t * ftyp, lqt_file_type_t type)
  {
  memset(ftyp, 0, sizeof(*ftyp));
  switch(type)
    {
    case LQT_FILE_NONE:
    case LQT_FILE_QT_OLD:
    case LQT_FILE_AVI:
      return;
    case LQT_FILE_QT:
      copy_ftyp(ftyp, &ftyp_qt);
      return;
    case LQT_FILE_MP4:
      copy_ftyp(ftyp, &ftyp_mp4);
      return;
    case LQT_FILE_M4A:
      copy_ftyp(ftyp, &ftyp_m4a);
      return;
    }
  }

lqt_file_type_t quicktime_ftyp_get_file_type(quicktime_ftyp_t * ftyp)
  {
  switch(ftyp->major_brand)
    {
    case MK_FOURCC('i','s','o','m'):
    case MK_FOURCC('m','p','4','1'):
    case MK_FOURCC('m','p','4','2'):
      return LQT_FILE_MP4;
      break;
    case MK_FOURCC('M','4','A',' '):
      return LQT_FILE_M4A;
      break;
    case MK_FOURCC('q','t',' ',' '):
      return LQT_FILE_QT;
      break;
    }
  return 0;
  }

void quicktime_read_ftyp(quicktime_t *file, quicktime_ftyp_t *ftyp,
                         quicktime_atom_t *parent_atom)
  {
  int i;
  ftyp->major_brand = quicktime_read_int32(file);
  ftyp->minor_version = quicktime_read_int32(file);
  ftyp->num_compatible_brands = (parent_atom->end - quicktime_position(file)) / 4;
  ftyp->compatible_brands = malloc(ftyp->num_compatible_brands *
                                   sizeof(ftyp->compatible_brands));
  for(i = 0; i < ftyp->num_compatible_brands; i++)
    {
    ftyp->compatible_brands[i] = quicktime_read_int32(file);
    }
  }

void quicktime_write_ftyp(quicktime_t *file, quicktime_ftyp_t *ftyp)
  {
  int i;
  quicktime_atom_t atom;
  quicktime_atom_write_header(file, &atom, "ftyp");
  quicktime_write_int32(file, ftyp->major_brand);
  quicktime_write_int32(file, ftyp->minor_version);

  for(i = 0; i < ftyp->num_compatible_brands; i++)
    quicktime_write_int32(file, ftyp->compatible_brands[i]);
  quicktime_atom_write_footer(file, &atom);
  }