#include "lqt_private.h"
#include <string.h>

void quicktime_GUID_dump(quicktime_GUID_t * g)
  {
  lqt_dump("%08x-%04x-%04x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n",
         g->v1, g->v2, g->v3, g->v4[0], g->v4[1], g->v4[2], g->v4[3],
         g->v4[4], g->v4[5], g->v4[6], g->v4[7]);
  }

int  quicktime_GUID_equal(const quicktime_GUID_t * g1, const quicktime_GUID_t * g2)
  {
  return (g1->v1 == g2->v1) &&
    (g1->v2 == g2->v2) &&
    (g1->v3 == g2->v3) &&
    !memcmp(g1->v4, g2->v4, 8);

  }

void quicktime_GUID_read(quicktime_t * file, quicktime_GUID_t * guid)
  {
  guid->v1 = quicktime_read_int32_le(file);
  guid->v2 = quicktime_read_int16_le(file);
  guid->v3 = quicktime_read_int16_le(file);
  quicktime_read_data(file, guid->v4, 8);
  }

void quicktime_GUID_write(quicktime_t * file, quicktime_GUID_t * guid)
  {
  quicktime_write_int32_le(file, guid->v1);
  quicktime_write_int16_le(file, guid->v2);
  quicktime_write_int16_le(file, guid->v3);
  quicktime_write_data(file, guid->v4, 8);
  
  }
