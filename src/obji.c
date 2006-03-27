#include <funcprotos.h>
#include <quicktime/quicktime.h>
#include <string.h>


int quicktime_obji_init(quicktime_obji_t *obji)
{
       obji->version = 2;
       obji->revision = 0;
       obji->movieType = 1;
       obji->viewStateCount = 1;
       obji->defaultViewState = 1;
       obji->mouseDownViewState = 1;
       obji->viewDuration = 600;
       obji->mouseMotionScale = 180;
       obji->minPan = 0;
       obji->maxPan = 360;
       obji->defaultPan = 0;
       obji->minTilt = 72;
       obji->maxTilt = -72;
       obji->defaultTilt = 0;
       obji->minFOV = 0;
       obji->FOV = 64;
       obji->defaultFOV = 64;
       obji->defaultViewCenterH = 120;
       obji->defaultViewCenterV = 160;
       obji->viewRate = 1;
       obji->frameRate = 1;
       obji->controlSettings = 69;

       return 0;
}

int quicktime_obji_delete(quicktime_obji_t *obji)
{
    return 0;
}

void quicktime_obji_dump(quicktime_obji_t *obji)
{
       printf("object node (obji)\n");
       printf(" version %i\n", obji->version );
       printf(" revision %i\n", obji->revision );

       printf(" movie type %i\n", obji->movieType );
       printf(" view state count %i\n", obji->viewStateCount );
       printf(" default viewstate %i\n", obji->defaultViewState );
       printf(" mousedown viewstate %i\n", obji->mouseDownViewState );
       printf(" view duration %ld\n", obji->viewDuration );
       printf(" columns %ld\n", obji->columns );
       printf(" rows %ld\n", obji->rows );
       
       printf(" mouse motion scale %f\n", obji->mouseMotionScale);
       printf(" minimum pan %f\n", obji->minPan );
       printf(" maximum pan %f\n", obji->maxPan );
       printf(" default pan %f\n", obji->defaultPan );
       printf(" minimum tilt %f\n", obji->minTilt );
       printf(" maximum tilt %f\n", obji->maxTilt );
       printf(" default tilt %f\n", obji->defaultTilt );
       printf(" minimum fov %f\n", obji->minFOV );
       printf(" fov %f\n", obji->FOV );
       printf(" default fov %f\n", obji->defaultFOV );
       printf(" default horizontal viewcenter %f\n", obji->defaultViewCenterH );
       printf(" default vertical viewcenter %f\n", obji->defaultViewCenterV );
       printf(" view rate %f\n", obji->viewRate );
       printf(" frame rate %f\n", obji->frameRate );
       
       printf(" animation settings %ld\n", obji->animSettings );
       printf(" control settings %ld\n", obji->controlSettings );
}

int quicktime_read_obji(quicktime_t *file, quicktime_obji_t *obji)
{
       obji->version = quicktime_read_int16(file);
       obji->revision = quicktime_read_int16(file);

       obji->movieType = quicktime_read_int16(file);
       obji->viewStateCount = quicktime_read_int16(file);
       obji->defaultViewState = quicktime_read_int16(file);
       obji->mouseDownViewState = quicktime_read_int16(file);
       
       obji->viewDuration = quicktime_read_int32(file);
       obji->columns = quicktime_read_int32(file);
       obji->rows = quicktime_read_int32(file);
       
       obji->mouseMotionScale = quicktime_read_float32(file);
       obji->minPan = quicktime_read_float32(file);
       obji->maxPan = quicktime_read_float32(file);
       obji->defaultPan = quicktime_read_float32(file);
       obji->minTilt = quicktime_read_float32(file);
       obji->maxTilt = quicktime_read_float32(file);
       obji->defaultTilt = quicktime_read_float32(file);
       obji->minFOV = quicktime_read_float32(file);
       obji->FOV = quicktime_read_float32(file);
       obji->defaultFOV = quicktime_read_float32(file);
       obji->defaultViewCenterH = quicktime_read_float32(file);
       obji->defaultViewCenterV = quicktime_read_float32(file);
       obji->viewRate = quicktime_read_float32(file);
       obji->frameRate = quicktime_read_float32(file);

       obji->animSettings = quicktime_read_int32(file);
       obji->controlSettings = quicktime_read_int32(file);

       return 0;
}

void quicktime_write_obji(quicktime_t *file, quicktime_obji_t *obji)
{
	quicktime_write_int16(file, obji->version);
	quicktime_write_int16(file, obji->revision);
	quicktime_write_int16(file, obji->movieType);
	
	quicktime_write_int16(file, obji->viewStateCount);
	quicktime_write_int16(file, obji->defaultViewState);
	quicktime_write_int16(file, obji->mouseDownViewState);
	
	quicktime_write_int32(file, obji->viewDuration);
	quicktime_write_int32(file, obji->columns);
	quicktime_write_int32(file, obji->rows);
	
	quicktime_write_float32(file, obji->mouseMotionScale);
	quicktime_write_float32(file, obji->minPan);
	quicktime_write_float32(file, obji->maxPan);
	quicktime_write_float32(file, obji->defaultPan);
	quicktime_write_float32(file, obji->minTilt);
	quicktime_write_float32(file, obji->maxTilt);
	quicktime_write_float32(file, obji->defaultTilt);
	quicktime_write_float32(file, obji->minFOV);
	quicktime_write_float32(file, obji->FOV);
	quicktime_write_float32(file, obji->defaultFOV);
	quicktime_write_float32(file, obji->defaultViewCenterH);
	quicktime_write_float32(file, obji->defaultViewCenterV);
	quicktime_write_float32(file, obji->viewRate);
	quicktime_write_float32(file, obji->frameRate);
	
	quicktime_write_int32(file, obji->animSettings);
	quicktime_write_int32(file, obji->controlSettings);
	return;
}

