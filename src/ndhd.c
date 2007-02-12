#include "lqt_private.h"

int quicktime_ndhd_init(quicktime_ndhd_t *ndhd)
{
       ndhd->version = 2;
       ndhd->revision = 0;
       ndhd->nodeType[0] = 'o';
       ndhd->nodeType[1] = 'b';
       ndhd->nodeType[2] = 'j';
       ndhd->nodeType[3] = 'e';
       ndhd->nodeID = 1;
       ndhd->nameAtomID = 0;
       ndhd->commentAtomID = 0;
       return 0;
}

int quicktime_ndhd_delete(quicktime_ndhd_t *ndhd)
{
    return 0;
}

void quicktime_ndhd_dump(quicktime_ndhd_t *ndhd)
{
       lqt_dump("node header (ndhd)\n");
       lqt_dump(" version %i\n", ndhd->version );
       lqt_dump(" revision %i\n", ndhd->revision );

       lqt_dump(" node type %c%c%c%c\n", ndhd->nodeType[0],ndhd->nodeType[1],ndhd->nodeType[2],ndhd->nodeType[3] );
       lqt_dump(" node id %ld\n", ndhd->nodeID );
       lqt_dump(" name atom id %ld\n", ndhd->nameAtomID );
       lqt_dump(" comment atom id %ld\n", ndhd->commentAtomID );
}

int quicktime_read_ndhd(quicktime_t *file, quicktime_ndhd_t *ndhd)
{
       ndhd->version = quicktime_read_int16(file);
       ndhd->revision = quicktime_read_int16(file);

       quicktime_read_char32(file, ndhd->nodeType);
       ndhd->nodeID = quicktime_read_int32(file);
       ndhd->nameAtomID = quicktime_read_int32(file);
       ndhd->commentAtomID = quicktime_read_int32(file);
       
       return 0;
}

void quicktime_write_ndhd(quicktime_t *file, quicktime_ndhd_t *ndhd)
{
    	quicktime_write_int16(file, ndhd->version);
    	quicktime_write_int16(file, ndhd->revision);
	quicktime_write_char32(file, ndhd->nodeType);
	quicktime_write_int32(file, ndhd->nodeID);
    	quicktime_write_int32(file, ndhd->nameAtomID);
    	quicktime_write_int32(file, ndhd->commentAtomID);
    	quicktime_write_int32(file, ndhd->reserved1);
    	quicktime_write_int32(file, ndhd->reserved2);
    	
	return;
}

