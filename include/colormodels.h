/* This file was automatically generated.  Do not edit! */
int cmodel_is_yuv(int colormodel);
int cmodel_from_text(char *text);
void cmodel_to_text(char *string,int cmodel);
int cmodel_bc_to_x(int color_model);
void cmodel_default(PERMUTATION_ARGS);
void cmodel_yuv422(PERMUTATION_ARGS);
void cmodel_yuv420p(PERMUTATION_ARGS);
void cmodel_transfer(unsigned char **output_rows,unsigned char **input_rows,unsigned char *out_y_plane,unsigned char *out_u_plane,unsigned char *out_v_plane,unsigned char *in_y_plane,unsigned char *in_u_plane,unsigned char *in_v_plane,int in_x,int in_y,int in_w,int in_h,int out_x,int out_y,int out_w,int out_h,int in_colormodel,int out_colormodel,int bg_color,int in_rowspan,int out_rowspan);
int cmodel_calculate_datasize(int w,int h,int bytes_per_line,int color_model);
int cmodel_calculate_max(int colormodel);
int cmodel_calculate_pixelsize(int colormodel);
int cmodel_components(int colormodel);
int cmodel_is_planar(int colormodel);
extern cmodel_yuv_t *yuv_table;
void cmodel_delete_yuv(cmodel_yuv_t *yuv_table);
void cmodel_init_yuv(cmodel_yuv_t *yuv_table);
