#ifndef	_LQT_ATOMS_H_
#define _LQT_ATOMS_H_

/* Fine tuning of quicktime atoms. Use with caution */

/** \ingroup video_encode
 *  \brief Set the field attributes of a video track
 *  \param file A quicktime handle
 *  \param track Track index (starting with 0)
 *  \param nfields number of fields (1 = progressive, 2 = interlaced)
 *  \param dominance field order/dominance (9 = top first, 14 = bottom first)
 *  \returns 1 if the call was successful, 0 if there is no such track or invalid number of fields or invalid dominance
 *
 *  The dominance parameter may also have the values 0, 1 and 6 but those are
 *  rarely used.  The Apple documentation at
 *
 *     http://developer.apple.com/quicktime/icefloe/dispatch019.html#fiel
 *
 *  has more detailed information about the 'fiel' atom.
 */

int lqt_set_fiel(quicktime_t *file, int track, int nfields, int dominance);

/** \ingroup video_encode
 *  \brief Get the field attributes of a video track
 *  \param file A quicktime handle
 *  \param track Track index (starting with 0)
 *  \param nfields number of fields
 *  \param dominance field order/dominance
 *  \returns 1 if the call was successful, 0 if there is no such track
 *
 *  The Apple documentation at
 *
 *     http://developer.apple.com/quicktime/icefloe/dispatch019.html#fiel
 *
 *  has more detailed information about the 'fiel' atom.
 */

int lqt_get_fiel(quicktime_t *file, int track, int *nfields, int *dominance);

/* pasp atom */

/*! \struct quicktime_pasp_t
 *  \brief Pixel Aspect atom structure
*/
typedef struct
{
	/*! horizontal spacing */
	int32_t hSpacing;
	/*! Vertical spacing */
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
