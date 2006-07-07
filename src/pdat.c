#include <funcprotos.h>
#include <quicktime/quicktime.h>
#include <string.h>


int quicktime_pdat_init(quicktime_pdat_t *pdat)
{
       pdat->version = 2;
       pdat->revision = 0;
       pdat->imageRefTrackIndex = 1;
       pdat->hotSpotRefTrackIndex = 0;
       pdat->minPan = 0;
       pdat->maxPan = 360;
       pdat->defaultPan = 0;
       pdat->minTilt = 72;
       pdat->maxTilt = -72;
       pdat->defaultTilt = 0;
       pdat->minFOV = 0;
       pdat->maxFOV = 64;
       pdat->defaultFOV = 64;
       pdat->imageSizeX = 0;
       pdat->imageSizeY = 0;
       pdat->imageNumFramesX = 1;
       pdat->imageNumFramesY = 1;
       pdat->hotSpotSizeX = 0;
       pdat->hotSpotSizeY = 0;
       pdat->hotSpotNumFramesX = 0;
       pdat->hotSpotNumFramesY = 0;
       pdat->flags = 0;
       pdat->panoType[0] = '\0';
       pdat->panoType[1] = '\0';
       pdat->panoType[2] = '\0';
       pdat->panoType[3] = '\0';
       return 0;
}

int quicktime_pdat_delete(quicktime_pdat_t *pdat)
{
    return 0;
}

void quicktime_pdat_dump(quicktime_pdat_t *pdat)
{
       printf("object node (pdat)\n");
       printf(" version %i\n", pdat->version );
       printf(" revision %i\n", pdat->revision );

       printf(" image track index %ld\n", pdat->imageRefTrackIndex );
       printf(" hotspot track index %ld\n", pdat->hotSpotRefTrackIndex );       
       printf(" minimum pan %f\n", pdat->minPan );
       printf(" maximum pan %f\n", pdat->maxPan );
       printf(" minimum tilt %f\n", pdat->minTilt );
       printf(" maximum tilt %f\n", pdat->maxTilt );
       printf(" minimum fov %f\n", pdat->minFOV );
       printf(" fov %f\n", pdat->maxFOV );
       printf(" default pan %f\n", pdat->defaultPan );
       printf(" default tilt %f\n", pdat->defaultTilt );
       printf(" default fov %f\n", pdat->defaultFOV );
       printf(" image size x %ld\n", pdat->imageSizeX );
       printf(" image size y %ld\n", pdat->imageSizeY );
       printf(" image frames x %i\n", pdat->imageNumFramesX );
       printf(" image frames y %i\n", pdat->imageNumFramesY );
       printf(" hotspot size x %ld\n", pdat->hotSpotSizeX );
       printf(" hotspot size y %ld\n", pdat->hotSpotSizeY );
       printf(" hotspot frames x %i\n", pdat->hotSpotNumFramesX );
       printf(" hotspot frames y %i\n", pdat->hotSpotNumFramesY );
       printf(" flags %ld\n", pdat->flags );
       printf(" panorama type %c%c%c%c\n",  pdat->panoType[0], pdat->panoType[1], pdat->panoType[2], pdat->panoType[3]);

}

int quicktime_read_pdat(quicktime_t *file, quicktime_pdat_t *pdat)
{
	pdat->version = quicktime_read_int16(file);
	pdat->revision = quicktime_read_int16(file);
	pdat->imageRefTrackIndex = quicktime_read_int32(file);
	pdat->hotSpotRefTrackIndex = quicktime_read_int32(file);
	pdat->minPan = quicktime_read_float32(file);
	pdat->maxPan = quicktime_read_float32(file);
	pdat->minTilt = quicktime_read_float32(file);
	pdat->maxTilt = quicktime_read_float32(file);
	pdat->minFOV = quicktime_read_float32(file);
	pdat->maxFOV = quicktime_read_float32(file);
	pdat->defaultPan = quicktime_read_float32(file);
	pdat->defaultTilt = quicktime_read_float32(file);
	pdat->defaultFOV = quicktime_read_float32(file);
	pdat->imageSizeX = quicktime_read_int32(file);
	pdat->imageSizeY = quicktime_read_int32(file);
	pdat->imageNumFramesX = quicktime_read_int16(file);
	pdat->imageNumFramesY = quicktime_read_int16(file);
	pdat->hotSpotSizeX = quicktime_read_int32(file);
	pdat->hotSpotSizeY = quicktime_read_int32(file);
	pdat->hotSpotNumFramesX = quicktime_read_int16(file);
	pdat->hotSpotNumFramesY = quicktime_read_int16(file);
	pdat->flags = quicktime_read_int32(file);
	quicktime_read_char32(file, pdat->panoType);
	pdat->reserved = quicktime_read_int32(file);
	return 0;
}

void quicktime_write_pdat(quicktime_t *file, quicktime_pdat_t *pdat)
{
	quicktime_write_int16(file, pdat->version);
	quicktime_write_int16(file, pdat->revision);
	quicktime_write_int32(file, pdat->imageRefTrackIndex);
	quicktime_write_int32(file, pdat->hotSpotRefTrackIndex);
	quicktime_write_float32(file, pdat->minPan);
	quicktime_write_float32(file, pdat->maxPan);
	quicktime_write_float32(file, pdat->minTilt);
	quicktime_write_float32(file, pdat->maxTilt);
	quicktime_write_float32(file, pdat->minFOV);
	quicktime_write_float32(file, pdat->maxFOV);
	quicktime_write_float32(file, pdat->defaultPan);
	quicktime_write_float32(file, pdat->defaultPan);
	quicktime_write_float32(file, pdat->defaultPan);
	quicktime_write_int32(file, pdat->imageSizeX);
	quicktime_write_int32(file, pdat->imageSizeY);
	quicktime_write_int16(file, pdat->imageNumFramesX);
	quicktime_write_int16(file, pdat->imageNumFramesY);
	quicktime_write_int32(file, pdat->hotSpotSizeX);
	quicktime_write_int32(file, pdat->hotSpotSizeY);
	quicktime_write_int16(file, pdat->hotSpotNumFramesX);
	quicktime_write_int16(file, pdat->hotSpotNumFramesY);
	quicktime_write_int32(file, pdat->flags);
	quicktime_write_char32(file, pdat->panoType);
	quicktime_write_int32(file, pdat->reserved);	
	return;
}

