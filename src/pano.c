#include <funcprotos.h>
#include <quicktime/quicktime.h>
#include <string.h>


int quicktime_pano_init(quicktime_pano_t *pano)
{
       pano->version = 0;
       pano->revision = 0;

       pano->STrack = 0;
       pano->LowResSTrack = 0;
       pano->HSTrack = 0;

       pano->HPanStart = 0;
       pano->HPanEnd = 360;
       pano->VPanStart = 30;
       pano->VPanEnd = -30;
       pano->MinZoom = 0;
       pano->MaxZoom = 0;

       pano->SHeight = 0;
       pano->SWidth = 0;
       pano->NumFrames = 1;
       pano->SNumFramesHeight = 1;
       pano->SNumFramesWidth = 1;
       pano->SDepth = 32;

       pano->HSHeight = 0;
       pano->HSWidth = 0;
       pano->HSNumFramesHeight = 0;
       pano->HSNumFramesWidth = 0;
       pano->HSDepth = 8;

       return 0;
}

int quicktime_pano_delete(quicktime_pano_t *pano)
{
    return 0;
}

void quicktime_pano_dump(quicktime_pano_t *pano)
{
       printf("       Panorama (pano)\n");
       printf("        Version %i\n", pano->version );
       printf("        Revision %i\n", pano->revision );

       printf("        Scene Track %ld\n", pano->STrack );
       printf("        LowRes Scene Track %ld\n", pano->LowResSTrack );
       printf("        HotSpot Track %ld\n", pano->HSTrack );

       printf("        HPanStart %f\n", pano->HPanStart );
       printf("        HpanEnd %f\n", pano->HPanEnd );
       printf("        VPanStart %f\n", pano->VPanStart );
       printf("        VPanEnd %f\n", pano->VPanEnd );
       printf("        Minimum Zoom %f\n", pano->MinZoom );
       printf("        Maximum Zoom %f\n", pano->MaxZoom );

       printf("        Scene Height %ld\n", pano->SHeight );
       printf("        Scene Width %ld\n", pano->SWidth );
       printf("        Num. Frames %ld\n", pano->NumFrames );
       printf("        Num. Frames(Height) %i\n", pano->SNumFramesHeight );
       printf("        Num. Frames(Width) %i\n", pano->SNumFramesWidth );
       printf("        Scene Depth %i\n", pano->SDepth );

       printf("        HotSpot Height %ld\n", pano->HSHeight );
       printf("        HotSpot Width %ld\n", pano->HSWidth );
       printf("        Num. HotSPot Frames (Height) %i\n", pano->HSNumFramesHeight );
       printf("        Num. HotSpot Frames (Width) %i\n", pano->HSNumFramesWidth );
       printf("        Hotspot Depth %i\n", pano->HSDepth );
}

int quicktime_read_pano(quicktime_t *file, quicktime_pano_t *pano, quicktime_atom_t *pano_atom)
{
       //quicktime_atom_t leaf_atom;
//     pano->reserved1 = quicktime_read_int32(file);
//     pano->reserved2 = quicktime_read_int32(file);

       pano->version = quicktime_read_int16(file);
       pano->revision = quicktime_read_int16(file);

       pano->STrack = quicktime_read_int32(file);
       pano->LowResSTrack = quicktime_read_int32(file);
       quicktime_read_data(file, (char *)pano->reserved3,  4 * 6);
       pano->HSTrack = quicktime_read_int32(file);
       quicktime_read_data(file, (char *)pano->reserved4, 4 * 9);

       pano->HPanStart = quicktime_read_fixed32(file);
       pano->HPanEnd = quicktime_read_fixed32(file);
       pano->VPanStart = quicktime_read_fixed32(file);
       pano->VPanEnd = quicktime_read_fixed32(file);
       pano->MinZoom = quicktime_read_fixed32(file);
       pano->MaxZoom = quicktime_read_fixed32(file);

       pano->SHeight = quicktime_read_int32(file);
       pano->SWidth = quicktime_read_int32(file);
       pano->NumFrames = quicktime_read_int32(file);
       pano->reserved5 = quicktime_read_int16(file);
       pano->SNumFramesHeight = quicktime_read_int16(file);
       pano->SNumFramesWidth = quicktime_read_int16(file);
       pano->SDepth = quicktime_read_int16(file);

       pano->HSHeight = quicktime_read_int32(file);
       pano->HSWidth = quicktime_read_int32(file);
       pano->reserved6 = quicktime_read_int16(file);
       pano->HSNumFramesHeight = quicktime_read_int16(file);
       pano->HSNumFramesWidth = quicktime_read_int16(file);
       pano->HSDepth = quicktime_read_int16(file);
       return 0;
}

void quicktime_write_pano(quicktime_t *file, quicktime_pano_t *pano)
{
/*     quicktime_atom_t atom, subatom;
       quicktime_atom_write_header(file, &atom, "pano");

       quicktime_write_int32(file, pano->reserved1);
       quicktime_write_int32(file, pano->reserved1);
*/
       quicktime_write_int16(file, pano->version);
       quicktime_write_int16(file, pano->revision);

       quicktime_write_int32(file, pano->STrack);
       quicktime_write_int32(file, pano->LowResSTrack);
       quicktime_write_data(file, (char *)pano->reserved3, 4 * 6);
       quicktime_write_int32(file, pano->HSTrack);
       quicktime_write_data(file, (char *)pano->reserved4, 4 * 9);

       quicktime_write_fixed32(file, pano->HPanStart);
       quicktime_write_fixed32(file, pano->HPanEnd);
       quicktime_write_fixed32(file, pano->VPanStart);
       quicktime_write_fixed32(file, pano->VPanEnd);
       quicktime_write_fixed32(file, pano->MinZoom);
       quicktime_write_fixed32(file, pano->MaxZoom);

       quicktime_write_int32(file, pano->SHeight);
       quicktime_write_int32(file, pano->SWidth);
       quicktime_write_int32(file, pano->NumFrames);
       quicktime_write_int16(file, pano->reserved5);
       quicktime_write_int16(file, pano->SNumFramesHeight);
       quicktime_write_int16(file, pano->SNumFramesWidth);
       quicktime_write_int16(file, pano->SDepth);

       quicktime_write_int32(file, pano->HSHeight);
       quicktime_write_int32(file, pano->HSWidth);
       quicktime_write_int16(file, pano->reserved6);
       quicktime_write_int16(file, pano->HSNumFramesHeight);
       quicktime_write_int16(file, pano->HSNumFramesWidth);
       quicktime_write_int16(file, pano->HSDepth);

//     quicktime_atom_write_footer(file, &atom);
}

