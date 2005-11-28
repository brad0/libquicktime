#ifndef	_LQT_ATOMS_H_
#define _LQT_ATOMS_H_

/* Fine tuning of quicktime atoms. Use with caution */

/** \defgroup atoms Fine tuning of quicktime atoms
 *
 * Libquicktime tries it's best to produce proper files, with all the
 * information neccesary for decoding the file with the right parameters.
 * In some cases however, it might be useful to tweak the corresponding atoms
 * individually. Please note that the functions described here, allow you full
 * control over the atoms. But they also allow you to create horribly
 * incompatible and undecodable files. Don't say you haven't been warned.
 */
 

/** \ingroup atoms
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

/** \ingroup atoms
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
	/*! Horizontal spacing */
	int32_t hSpacing;
	/*! Vertical spacing */
	int32_t vSpacing;
} quicktime_pasp_t;

/** \ingroup atoms
 *  \brief Set the pixel aspect atom of a video track
 *  \param file A quicktime handle
 *  \param track Track index (starting with 0)
 *  \param pasp Pixel aspect atom
 *  \returns 1 if the call was successful, 0 if there is no such track
 */
int  lqt_set_pasp(quicktime_t *file, int track, quicktime_pasp_t *pasp);

/** \ingroup atoms
 *  \brief Get the pixel aspect atom of a video track
 *  \param file A quicktime handle
 *  \param track Track index (starting with 0)
 *  \param pasp Pixel aspect atom
 *  \returns 1 if the call was successful, 0 if there is no such track
 */
int  lqt_get_pasp(quicktime_t *file, int track, quicktime_pasp_t *pasp);

/*! \struct quicktime_clap_t
 *  \brief Clean Aperture atom structure
*/
typedef struct
{
	/*! width of clean aperture numerator, in pixels */
	int32_t cleanApertureWidthN;
	/*! width of clean aperture denominator, in pixels */
	int32_t cleanApertureWidthD;
	/*! height of clean aperture numerator, in pixels */
	int32_t cleanApertureHeightN;
	/*! height of clean aperture denominator, in pixels */
	int32_t cleanApertureHeightD;
	/*! horzontal offset of clean aperture (numerator) center minus (width-1)/2. Typically 0 */
	int32_t horizOffN;
	/*! horzontal offset of clean aperture (denominator) center minus (width-1)/2. Typically 0 */
	int32_t horizOffD;
	/*! vertical offset of clean aperture (numerator) center minus (width-1)/2. Typically 0 */
	int32_t vertOffN;
	/*! vertical offset of clean aperture (denominator) center minus (width-1)/2. Typically 0 */
	int32_t vertOffD;
} quicktime_clap_t;

/** \ingroup atoms
 *  \brief Set the clean aperture attributes of a video track
 *  \param file A quicktime handle
 *  \param track Track index (starting with 0)
 *  \param clap Clean aperture atom
 *  \returns 1 if the call was successful, 0 if there is no such track
 *
 *  The Apple documentation at
 *
 *     http://developer.apple.com/quicktime/icefloe/dispatch019.html#clap
 *
 *  has more detailed information about the 'clap' atom.
 */
int  lqt_set_clap(quicktime_t *file, int track, quicktime_clap_t *clap);

/** \ingroup atoms
 *  \brief Get the clean aperture attributes of a video track
 *  \param file A quicktime handle
 *  \param track Track index (starting with 0)
 *  \param clap Clean aperture atom
 *  \returns 1 if the call was successful, 0 if there is no such track
 *
 *  The Apple documentation at
 *
 *     http://developer.apple.com/quicktime/icefloe/dispatch019.html#clap
 *
 *  has more detailed information about the 'clap' atom.
 */

int  lqt_get_clap(quicktime_t *file, int track, quicktime_clap_t *clap);

/*! \struct quicktime_colr_t
 *  \brief 'colr' ImageDescription Extension structure
*/
typedef struct
{
	/*! OSType = 'nclc' for video, 'prof' for print */
	int32_t colorParamType;
	/*! CIE 1931 xy chromaticity coordinates of red, green, blue primaries and white point */
	int16_t primaries;
	/*! Nonlinear transfer function from RGB to Er/Eg/Eb */
	int16_t transferFunction;
	/*! Matrix from ErEgEb to Ey/Ecb/Ecr */
	int16_t matrix;
} quicktime_colr_t;

/** \ingroup atoms
 *  \brief Set the 'colr' ImageDescription Extension of a video track
 *  \param file A quicktime handle
 *  \param track Track index (starting with 0)
 *  \param colr Colr atom
 *  \returns 1 if the call was successful, 0 if there is no such track
 *
 *  The Apple documentation at
 *
 *     http://developer.apple.com/quicktime/icefloe/dispatch019.html#colr
 *
 *  has more detailed information about the 'colr' atom.
 */
int  lqt_set_colr(quicktime_t *file, int track, quicktime_colr_t *colr);

/** \ingroup atoms
 *  \brief Get the 'colr' ImageDescription Extension of a video track
 *  \param file A quicktime handle
 *  \param track Track index (starting with 0)
 *  \param colr Colr atom
 *  \returns 1 if the call was successful, 0 if there is no such track
 *
 *  The Apple documentation at
 *
 *     http://developer.apple.com/quicktime/icefloe/dispatch019.html#colr
 *
 *  has more detailed information about the 'colr' atom.
 */
int  lqt_get_colr(quicktime_t *file, int track, quicktime_colr_t *colr);

#endif /* _LQT_ATOMS_H_ */
