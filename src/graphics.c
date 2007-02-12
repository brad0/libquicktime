#include "lqt_private.h"
#include "graphics.h"
#include <stdlib.h>

/* Graphics acceleration routines */

quicktime_scaletable_t* quicktime_new_scaletable(int input_w, int input_h, int output_w, int output_h)
{
	quicktime_scaletable_t *result = (quicktime_scaletable_t*)malloc(sizeof(quicktime_scaletable_t));
	float i;
	float scalex = (float)input_w / output_w, scaley = (float)input_h / output_h;

	result->input_x = (int*)malloc(sizeof(int) * output_w);
	result->input_y = (int*)malloc(sizeof(int) * output_h);

	for(i = 0; i < output_w; i++)
	{
		result->input_x[(int)i] = (int)(scalex * i);
	}

	for(i = 0; i < output_h; i++)
	{
		result->input_y[(int)i] = (int)(scaley * i);
	}

	result->in_w = input_w;
	result->in_h = input_h;
	result->out_w = output_w;
	result->out_h = output_h;
	return result;
}

void quicktime_delete_scaletable(quicktime_scaletable_t *scaletable)
{
	free(scaletable->input_x);
	free(scaletable->input_y);
	free(scaletable);
}

/* Return 1 if dimensions are different from scaletable */
int quicktime_compare_scaletable(quicktime_scaletable_t *scaletable, 
	int in_w, 
	int in_h, 
	int out_w, 
	int out_h)
{
	if(scaletable->in_w != in_w ||
		scaletable->in_h != in_h ||
		scaletable->out_w != out_w ||
		scaletable->out_h != out_h)
		return 1;
	else
		return 0;
}

/* Return 1 if the scaletable is 1:1 */
int quicktime_identity_scaletable(quicktime_scaletable_t *scaletable)
{
	if(scaletable->in_w == scaletable->out_w &&
		scaletable->in_h == scaletable->out_h)
		return 1;
	else
		return 0;
}
