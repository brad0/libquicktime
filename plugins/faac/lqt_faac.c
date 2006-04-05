/*****************************************************************

  lqt_faac.c

  Copyright (c) 2005 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de

  http://libquicktime.sourceforge.net

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.

*****************************************************************/

#include <quicktime/lqt.h>
#include <quicktime/lqt_codecapi.h>

extern void quicktime_init_codec_faac(quicktime_audio_map_t *atrack);

static char * fourccs_faac[]     = { "mp4a", (char*)0 };


static lqt_parameter_info_static_t encode_parameters_faac[] =
  {
    {
      name:        "faac_bitrate",
      real_name:   "Bitrate (kbps, 0 = VBR)",
      type:        LQT_PARAMETER_INT,
      val_default: { val_int: 0 }
    },
    {
      name:        "quality",
      real_name:   "VBR Quality",
      type:        LQT_PARAMETER_INT,
      val_min:     { val_int: 10 },
      val_max:     { val_int: 500 },
      val_default: { val_int: 100 },
    },
    { /* End of parameters */ }
  };

static lqt_codec_info_static_t codec_info_faac =
  {
    name:                "faac",
    long_name:           "MPEG-2/4 AAC encoder",
    description:         "MPEG-2/4 AAC encoder (faac based)",
    fourccs:             fourccs_faac,
    type:                LQT_CODEC_AUDIO,
    direction:           LQT_DIRECTION_ENCODE,
    encoding_parameters: encode_parameters_faac,
    decoding_parameters: (lqt_parameter_info_static_t*)0
  };

/* These are called from the plugin loader */

extern int get_num_codecs() { return 1; }

extern lqt_codec_info_static_t * get_codec_info(int index)
  {
  switch(index)
    {
    case 0:
      return &codec_info_faac;
      break;
    }  
  return (lqt_codec_info_static_t*)0;
  }
     
/*
 *   Return the actual codec constructor
 */

extern lqt_init_audio_codec_func_t get_audio_codec(int index)
  {
  return quicktime_init_codec_faac;
  }

