#include "lqt_private.h"

int quicktime_pHdr_init(quicktime_pHdr_t *pHdr)
{

    pHdr->nodeID = 1;
    pHdr->defHPan = 0;
    pHdr->defVPan = 0;
    pHdr->defZoom = 0;

    // constraints for this node; use zero for default
    pHdr->minHPan = 0;
    pHdr->minVPan = 0;
    pHdr->minZoom = 0;
    pHdr->maxHPan = 0;
    pHdr->maxVPan = 0;
    pHdr->maxZoom = 0;

    pHdr->nameStrOffset = 0;        // offset into string table atom
    pHdr->commentStrOffset = 0;    // offset into string table atom

    return 0;
}

int quicktime_pHdr_delete(quicktime_pHdr_t *pHdr)
{
    return 0;
}

void quicktime_pHdr_dump(quicktime_pHdr_t *pHdr)
{

}

int quicktime_read_pHdr(quicktime_t *file, quicktime_pHdr_t *pHdr, quicktime_atom_t *pHdr_atom)
{

       return 0;
}

void quicktime_write_pHdr(quicktime_t *file, quicktime_pHdr_t *pHdr)
{

}

