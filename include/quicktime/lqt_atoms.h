#ifndef	_LQT_ATOMS_H_
#define _LQT_ATOMS_H_

/* Fine tuning of quicktime atoms. Use with caution */

int lqt_set_fiel(quicktime_t *, int, int, int);
int lqt_get_fiel(quicktime_t *, int, int *, int *);


/* pasp atom */

typedef struct
{
	int32_t hSpacing;
	int32_t vSpacing;
} quicktime_pasp_t;

int  lqt_set_pasp(quicktime_t *file, int track, quicktime_pasp_t *pasp);
int  lqt_get_pasp(quicktime_t *file, int track, quicktime_pasp_t *pasp);

/* clap atom */

typedef struct
{
	int32_t cleanApertureWidthN;
	int32_t cleanApertureWidthD;
	int32_t cleanApertureHeightN;
	int32_t cleanApertureHeightD;
	int32_t horizOffN;
	int32_t horizOffD;
	int32_t vertOffN;
	int32_t vertOffD;
} quicktime_clap_t;

int  lqt_set_clap(quicktime_t *file, int track, quicktime_clap_t *clap);
int  lqt_get_clap(quicktime_t *file, int track, quicktime_clap_t *clap);

/* colr atom */

typedef struct
{
	int32_t colorParamType;
	int16_t primaries;
	int16_t transferFunction;
	int16_t matrix;
} quicktime_colr_t;

int  lqt_set_colr(quicktime_t *file, int track, quicktime_colr_t *colr);
int  lqt_get_colr(quicktime_t *file, int track, quicktime_colr_t *colr);

#endif /* _LQT_ATOMS_H_ */
