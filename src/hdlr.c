#include <funcprotos.h>
#include <quicktime/quicktime.h>
#include <string.h>


void quicktime_hdlr_init(quicktime_hdlr_t *hdlr)
{
	hdlr->version = 0;
	hdlr->flags = 0;
	hdlr->component_type[0] = 'm';
	hdlr->component_type[1] = 'h';
	hdlr->component_type[2] = 'l';
	hdlr->component_type[3] = 'r';
	hdlr->component_subtype[0] = 'v';
	hdlr->component_subtype[1] = 'i';
	hdlr->component_subtype[2] = 'd';
	hdlr->component_subtype[3] = 'e';
	hdlr->component_manufacturer[0] = 0;
	hdlr->component_manufacturer[1] = 0;
	hdlr->component_manufacturer[2] = 0;
	hdlr->component_manufacturer[3] = 0;
	hdlr->component_flags = 0;
	hdlr->component_flag_mask = 0;
	strcpy(hdlr->component_name, "Linux Media Handler");
}

void quicktime_hdlr_init_qtvr(quicktime_hdlr_t *hdlr, int track_type)
{
    	if (track_type == QTVR_QTVR)
	{
	    hdlr->component_subtype[0] = 'q';
	    hdlr->component_subtype[1] = 't';
	    hdlr->component_subtype[2] = 'v';
	    hdlr->component_subtype[3] = 'r';
	    strcpy(hdlr->component_name, "Linux QTVR Handler");
	} else
	if (track_type == QTVR_OBJ)
	{
	    hdlr->component_subtype[0] = 'o';
	    hdlr->component_subtype[1] = 'b';
	    hdlr->component_subtype[2] = 'j';
	    hdlr->component_subtype[3] = 'e';
	    strcpy(hdlr->component_name, "Linux QTVR OBject Handler");
	}
}

void quicktime_hdlr_init_panorama(quicktime_hdlr_t *hdlr)
{
	hdlr->component_subtype[0] = 'S';
	hdlr->component_subtype[1] = 'T';
	hdlr->component_subtype[2] = 'p';
	hdlr->component_subtype[3] = 'n';
	strcpy(hdlr->component_name, "Linux Panorama Media Handler");
}

void quicktime_hdlr_init_video(quicktime_hdlr_t *hdlr)
{
	hdlr->component_subtype[0] = 'v';
	hdlr->component_subtype[1] = 'i';
	hdlr->component_subtype[2] = 'd';
	hdlr->component_subtype[3] = 'e';
	strcpy(hdlr->component_name, "Linux Video Media Handler");
}

void quicktime_hdlr_init_audio(quicktime_hdlr_t *hdlr)
{
	hdlr->component_subtype[0] = 's';
	hdlr->component_subtype[1] = 'o';
	hdlr->component_subtype[2] = 'u';
	hdlr->component_subtype[3] = 'n';
	strcpy(hdlr->component_name, "Linux Sound Media Handler");
}

/*
  	hdlr->version = quicktime_read_char(file);
	hdlr->flags = quicktime_read_int24(file);
	quicktime_read_char32(file, hdlr->component_type);
	quicktime_read_char32(file, hdlr->component_subtype);
	hdlr->component_manufacturer = quicktime_read_int32(file);
	hdlr->component_flags = quicktime_read_int32(file);
	hdlr->component_flag_mask = quicktime_read_int32(file);
	quicktime_read_pascal(file, hdlr->component_name);
*/

void quicktime_hdlr_init_udta(quicktime_hdlr_t *hdlr)
{
   hdlr->version = 0;
   hdlr->flags = 0;
   hdlr->component_type[0] = 0x00;
   hdlr->component_type[1] = 0x00;
   hdlr->component_type[2] = 0x00;
   hdlr->component_type[3] = 0x00;
   hdlr->component_subtype[0] = 'm';
   hdlr->component_subtype[1] = 'd';
   hdlr->component_subtype[2] = 'i';
   hdlr->component_subtype[3] = 'r';
   hdlr->component_manufacturer[0] = 'a';
   hdlr->component_manufacturer[1] = 'p';
   hdlr->component_manufacturer[2] = 'p';
   hdlr->component_manufacturer[3] = 'l';
   hdlr->component_flags = 0;
   hdlr->component_flag_mask = 0;
   hdlr->component_name[0] = '\0';
}


void quicktime_hdlr_init_data(quicktime_hdlr_t *hdlr)
{
	hdlr->component_type[0] = 'd';
	hdlr->component_type[1] = 'h';
	hdlr->component_type[2] = 'l';
	hdlr->component_type[3] = 'r';
	hdlr->component_subtype[0] = 'a';
	hdlr->component_subtype[1] = 'l';
	hdlr->component_subtype[2] = 'i';
	hdlr->component_subtype[3] = 's';
	strcpy(hdlr->component_name, "Linux Alias Data Handler");
}

void quicktime_hdlr_delete(quicktime_hdlr_t *hdlr)
{
}

void quicktime_hdlr_dump(quicktime_hdlr_t *hdlr)
{
	printf("   handler reference (hdlr)\n");
	printf("    version %d\n", hdlr->version);
	printf("    flags %ld\n", hdlr->flags);
	printf("    component_type %c%c%c%c\n", hdlr->component_type[0], hdlr->component_type[1], hdlr->component_type[2], hdlr->component_type[3]);
	printf("    component_subtype %c%c%c%c\n", hdlr->component_subtype[0], hdlr->component_subtype[1], hdlr->component_subtype[2], hdlr->component_subtype[3]);
	printf("    component_name %s\n", hdlr->component_name);
}

void quicktime_read_hdlr(quicktime_t *file, quicktime_hdlr_t *hdlr, quicktime_atom_t * parent_atom)
{
        int component_name_len;
	hdlr->version = quicktime_read_char(file);
	hdlr->flags = quicktime_read_int24(file);
	quicktime_read_char32(file, hdlr->component_type);
	quicktime_read_char32(file, hdlr->component_subtype);
	quicktime_read_char32(file, hdlr->component_manufacturer);
	hdlr->component_flags = quicktime_read_int32(file);
	hdlr->component_flag_mask = quicktime_read_int32(file);
        /* Difference between mp4 and quicktime: In quicktime, component name
           is a pascal string. In mp4, it's until the end of the atom */
        if(!hdlr->component_type[0] && !hdlr->component_type[1] &&
           !hdlr->component_type[2] && !hdlr->component_type[3])
          {
          component_name_len = parent_atom->end - quicktime_position(file);
          if(component_name_len > 256)
            component_name_len = 256;
          quicktime_read_data(file, hdlr->component_name, component_name_len);
          }
        else
          {
          if(quicktime_position(file) < parent_atom->end)
            quicktime_read_pascal(file, hdlr->component_name);
          }
        /* Main Actor doesn't write component name */
        quicktime_atom_skip(file, parent_atom);

}

void quicktime_write_hdlr(quicktime_t *file, quicktime_hdlr_t *hdlr)
{
	quicktime_atom_t atom;
	quicktime_atom_write_header(file, &atom, "hdlr");

	quicktime_write_char(file, hdlr->version);
	quicktime_write_int24(file, hdlr->flags);
        
        if(file->file_type & (LQT_FILE_M4A|LQT_FILE_MP4))
          quicktime_write_int32(file, 0);
        else
          quicktime_write_char32(file, hdlr->component_type);
        
	quicktime_write_char32(file, hdlr->component_subtype); 
	quicktime_write_char32(file, hdlr->component_manufacturer);
	quicktime_write_int32(file, hdlr->component_flags);
	quicktime_write_int32(file, hdlr->component_flag_mask);

        if(file->file_type & (LQT_FILE_M4A|LQT_FILE_MP4))
          quicktime_write_data(file, hdlr->component_name, strlen(hdlr->component_name));
        else
          quicktime_write_pascal(file, hdlr->component_name);
        
	quicktime_atom_write_footer(file, &atom);
}
