/*****************************************************************

  lqt_faad2.c

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

extern void quicktime_init_codec_faad2(quicktime_audio_map_t *atrack);

static char * fourccs_faad2[]     = { "mp4a", (char*)0 };

static lqt_codec_info_static_t codec_info_faad2 =
  {
    name:                "faad2",
    long_name:           "MPEG-2/4 AAC decoder",
    description:         "MPEG-2/4 AAC decoder (faad2 based)",
    fourccs:             fourccs_faad2,
    type:                LQT_CODEC_AUDIO,
    direction:           LQT_DIRECTION_DECODE,
    encoding_parameters: (lqt_parameter_info_static_t*)0,
    decoding_parameters: (lqt_parameter_info_static_t*)0
  };


/* These are called from the plugin loader */

extern int get_num_codecs() { return 1; }

extern lqt_codec_info_static_t * get_codec_info(int index)
  {
  switch(index)
    {
    case 0:
      return &codec_info_faad2;
      break;
    }  
  return (lqt_codec_info_static_t*)0;
  }
     
/*
 *   Return the actual codec constructor
 */

extern lqt_init_audio_codec_func_t get_audio_codec(int index)
  {
  return quicktime_init_codec_faad2;
  }

