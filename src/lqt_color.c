#include <quicktime/lqt.h>
#include <quicktime/colormodels.h>
#include <string.h>

typedef struct
  {
  char * name;
  int colormodel;
  } lqt_colormodel_tab;

static lqt_colormodel_tab colormodel_table[] =
  {
    { "Transparency", BC_TRANSPARENCY },
    { "Compressed", BC_COMPRESSED },
    { "8 bpp BGB", BC_RGB8 },
    { "16 bpp RGB 565", BC_RGB565 },
    { "16 bpp BGR 565", BC_BGR565 },
    { "24 bpp BGR", BC_BGR888 },
    { "32 bpp BGR", BC_BGR8888 },
    
    { "24 bpp RGB", BC_RGB888 },
    { "32 bpp RGBA", BC_RGBA8888 },
    { "32 bpp ARGB", BC_ARGB8888 },
    { "32 bpp ARGB", BC_ABGR8888  },  
    { "48 bpp RGB", BC_RGB161616  }, 
    { "64 bpp RGB", BC_RGBA16161616  },
    { "24 bpp YUV", BC_YUV888  },
    { "32 bpp YUVA", BC_YUVA8888  },   
    { "48 bpp YUV", BC_YUV161616  }, 
    { "48 bpp", BC_YUVA16161616  },
    { "YUV 4:2:2 packed (YUY2)", BC_YUV422  },
    { "8 bpp Alpha", BC_A8  },
    { "16 bpp Alpha", BC_A16 },
    { "30 bpp YUV", BC_YUV101010 },
    { "24 bpp VYU", BC_VYU888 }, 
    { "32 bpp UYVA", BC_UYVA8888 },
    
    { "YUV 4:2:0 planar", BC_YUV420P },
    { "YUV 4:2:2 planar", BC_YUV422P },
    { "YUV 4:1:1 planar", BC_YUV411P },
    { (char*)0, LQT_COLORMODEL_NONE }
  };

const char * lqt_colormodel_to_string(int colormodel)
  {
  int index = 0;
    
  while(1)
    {
    if((colormodel_table[index].colormodel == colormodel) ||
       (colormodel_table[index].colormodel == LQT_COLORMODEL_NONE))
      break;
    index++;
    }
  return colormodel_table[index].name;
  }

int lqt_string_to_colormodel(const char * str)
  {
  int index = 0;

  while(1)
    {
    if((!colormodel_table[index].name) ||
       !strcmp(colormodel_table[index].name, str))
      break;
    index++;
    }
  return colormodel_table[index].colormodel;
  }

int lqt_num_colormodels()
  {
  int ret = 0;

  while(colormodel_table[ret].name)
    ret++;

  return ret;
  }

const char * lqt_get_colormodel_string(int index)
  {
  return colormodel_table[index].name;
  }

int lqt_get_colormodel(int index)
  {
  return colormodel_table[index].colormodel;
  }
