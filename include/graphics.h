#ifndef QUICKTIME_GRAPHICS_H
#define QUICKTIME_GRAPHICS_H

typedef struct
{
	int *input_x;
	int *input_y;
	int in_w, in_h, out_w, out_h;
} quicktime_scaletable_t;

int quicktime_identity_scaletable(quicktime_scaletable_t *scaletable);
int quicktime_compare_scaletable(quicktime_scaletable_t *scaletable,int in_w,int in_h,int out_w,int out_h);
void quicktime_delete_scaletable(quicktime_scaletable_t *scaletable);
quicktime_scaletable_t *quicktime_new_scaletable(int input_w,int input_h,int output_w,int output_h);

#endif
