#ifndef QUICKTIME_GRAPHICS_H
#define QUICKTIME_GRAPHICS_H

typedef struct
{
	long rtoy_tab[256], gtoy_tab[256], btoy_tab[256];
	long rtou_tab[256], gtou_tab[256], btou_tab[256];
	long rtov_tab[256], gtov_tab[256], btov_tab[256];

	long vtor_tab[256], vtog_tab[256];
	long utog_tab[256], utob_tab[256];
	long *vtor, *vtog, *utog, *utob;
} quicktime_yuv_t;

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
void quicktime_delete_yuv(quicktime_yuv_t *yuv_table);
void quicktime_init_yuv(quicktime_yuv_t *yuv_table);

#endif
