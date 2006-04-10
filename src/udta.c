#include <funcprotos.h>
#include <quicktime/quicktime.h>
#include <string.h>

#define ILST_TYPES (LQT_FILE_M4A|LQT_FILE_MP4)

// #define DEFAULT_INFO "Made with Libquicktime"

/* Atom IDs */

static char copyright_id[] = { 0xa9, 'c', 'p', 'y' };
static char info_id[]      = { 0xa9, 'i', 'n', 'f' };
static char name_id[]      = { 0xa9, 'n', 'a', 'm' };
static char artist_id[]    = { 0xa9, 'A', 'R', 'T' };
static char album_id[]     = { 0xa9, 'a', 'l', 'b' };
static char track_id[]     = { 0xa9, 't', 'r', 'k' };
static char comment_id[]   = { 0xa9, 'c', 'm', 't' };
static char author_id[]    = { 0xa9, 'a', 'u', 't' };
static char genre_id[]     = { 0xa9, 'g', 'e', 'n' };
static char trkn_id[]      = {  't', 'r', 'k', 'n' };

int quicktime_udta_init(quicktime_udta_t *udta)
{
        memset(udta, 0, sizeof(*udta));

        // No, no, it's bad to tell lies here (especially when decoding)
        //	udta->info = malloc(strlen(DEFAULT_INFO) + 1);
        //	udta->info_len = strlen(DEFAULT_INFO);
        //	sprintf(udta->info, DEFAULT_INFO);

	udta->is_qtvr = 0;
	quicktime_navg_init(&(udta->navg));
	return 0;
}

int quicktime_udta_delete(quicktime_udta_t *udta)
{
	if(udta->copyright_len)
	{
		free(udta->copyright);
	}
	if(udta->name_len)
	{
		free(udta->name);
	}
	if(udta->info_len)
	{
		free(udta->info);
	}
	if(udta->author_len)
	{
		free(udta->author);
	}
	if(udta->artist_len)
	{
		free(udta->artist);
	}
	if(udta->genre_len)
	{
		free(udta->genre);
	}
	if(udta->comment_len)
	{
		free(udta->comment);
	}
	if(udta->track_len)
	{
		free(udta->track);
	}
	if(udta->album_len)
	{
		free(udta->album);
	}
        // Libquicktime: No, no, this makes a memleak
	// quicktime_udta_init(udta);
	return 0;
}

void quicktime_udta_dump(quicktime_udta_t *udta)
{
	printf(" user data (udta)\n");
	if(udta->copyright_len) printf("  copyright: %s\n", udta->copyright);
	if(udta->name_len)      printf("  name:      %s\n", udta->name);
	if(udta->info_len)      printf("  info:      %s\n", udta->info);
	if(udta->author_len)    printf("  author:    %s\n", udta->author);
	if(udta->artist_len)    printf("  artist:    %s\n", udta->artist);
	if(udta->album_len)     printf("  album:     %s\n", udta->album);
	if(udta->track_len)     printf("  track:     %s\n", udta->track);
	if(udta->genre_len)     printf("  genre:     %s\n", udta->genre);
	if(udta->comment_len)   printf("  comment:   %s\n", udta->comment);
	if(udta->is_qtvr)       printf("  ctyp:      %c%c%c%c\n", udta->ctyp[0],
	    							  udta->ctyp[1],
							    	  udta->ctyp[2],
							          udta->ctyp[3]);
	if(quicktime_match_32(udta->ctyp, "stna")) quicktime_navg_dump(&(udta->navg));
}

int quicktime_read_udta_string(quicktime_t *file, char **string, int *size, int ilst)
{
	quicktime_atom_t leaf_atom;
	int result;
        uint32_t tmp;
	if(*size) free(*string);
        if(!ilst)
          {
          *size = quicktime_read_int16(file);  /* Size of string */
          quicktime_read_int16(file);  /* Discard language code */
          *string = malloc(*size + 1);
          result = quicktime_read_data(file, (uint8_t*)(*string), *size);
          (*string)[*size] = 0;
          return !result;
          }
        else
          {
          quicktime_atom_read_header(file, &leaf_atom);
          if(!quicktime_atom_is(&leaf_atom, "data"))
            return 1;
          tmp = quicktime_read_int32(file); /* Version and flags */
          if(!(tmp & 0x00000001)) /* Contains no text */
            return 1;
          quicktime_read_int32(file); /* Reserved (0) */
          *size = leaf_atom.end - quicktime_position(file);  /* Size of string */
          *string = malloc(*size + 1);
          result = quicktime_read_data(file, (uint8_t*)(*string), *size);
          (*string)[*size] = 0;
          return !result;
          }
}


int quicktime_read_udta(quicktime_t *file, quicktime_udta_t *udta,
                        quicktime_atom_t *udta_atom)
{
	quicktime_atom_t leaf_atom;
	int result = 0, tmp;
        int have_meta = 0, have_ilst = 0;
        do
	{
		quicktime_atom_read_header(file, &leaf_atom);
		
		if(quicktime_atom_is(&leaf_atom, "meta"))
		{
                        /* Skip version and flags (0) */
                        quicktime_read_int32(file);
                        have_meta = 1;
                }
		else
		if(quicktime_atom_is(&leaf_atom, "ilst"))
		{
                        have_ilst = 1;
                }
		else
		if(quicktime_atom_is(&leaf_atom, "hdlr"))
		{
     	                quicktime_atom_skip(file, &leaf_atom);
                }
		else
		if(quicktime_atom_is(&leaf_atom, copyright_id))
		{
                result += quicktime_read_udta_string(file, &(udta->copyright), &(udta->copyright_len), have_ilst);
		}
		else
		if(quicktime_atom_is(&leaf_atom, name_id))
		{
			result += quicktime_read_udta_string(file, &(udta->name), &(udta->name_len), have_ilst);
		}
		else
		if(quicktime_atom_is(&leaf_atom, info_id))
		{
			result += quicktime_read_udta_string(file, &(udta->info), &(udta->info_len), have_ilst);
                }
                else
                if(quicktime_atom_is(&leaf_atom, artist_id))
		{
			result += quicktime_read_udta_string(file, &(udta->artist), &(udta->artist_len), have_ilst);
                }
                else
                if(quicktime_atom_is(&leaf_atom, album_id))
		{
			result += quicktime_read_udta_string(file, &(udta->album), &(udta->album_len), have_ilst);
                }
                else
                if(quicktime_atom_is(&leaf_atom, genre_id))
		{
			result += quicktime_read_udta_string(file, &(udta->genre), &(udta->genre_len), have_ilst);
                }
                else
                if(quicktime_atom_is(&leaf_atom, track_id))
		{
			result += quicktime_read_udta_string(file, &(udta->track), &(udta->track_len), have_ilst);
                }
                else if(quicktime_atom_is(&leaf_atom, trkn_id))
                {
                         quicktime_atom_read_header(file, &leaf_atom);
                         if(!quicktime_atom_is(&leaf_atom, "data"))
                           return 1;
                         quicktime_read_int32(file); /* Version and flags */
                         quicktime_read_int32(file); /* Reserved (0) */
                         quicktime_read_int16(file); /* Emtpy */
                         tmp = quicktime_read_int16(file);
                         quicktime_read_int16(file); /* Total tracks */
                         quicktime_read_int16(file); /* Empty */
                         udta->track_len = 6;
                         udta->track = malloc(6); /* strlen("65535") + 1 */
                         sprintf(udta->track, "%d", tmp);
                }
                else
                if(quicktime_atom_is(&leaf_atom, comment_id))
		{
			result += quicktime_read_udta_string(file, &(udta->comment), &(udta->comment_len), have_ilst);
                }
                else
                if(quicktime_atom_is(&leaf_atom, author_id))
		{
			result += quicktime_read_udta_string(file, &(udta->author), &(udta->author_len), have_ilst);
                }
                else

                if(quicktime_atom_is(&leaf_atom, "NAVG"))
		{
			result += quicktime_read_navg(file, &(udta->navg), &leaf_atom);
                }
		else
                if(quicktime_atom_is(&leaf_atom, "ctyp"))
		{
			udta->ctyp[0] = quicktime_read_char(file);
			udta->ctyp[1] = quicktime_read_char(file);
			udta->ctyp[2] = quicktime_read_char(file);
			udta->ctyp[3] = quicktime_read_char(file);
			if (quicktime_match_32(udta->ctyp, "stna") ||
			    quicktime_match_32(udta->ctyp, "qtvr") ||
			    quicktime_match_32(udta->ctyp, "STpn")) udta->is_qtvr = 1;
                }
		else
		quicktime_atom_skip(file, &leaf_atom);
	}while(quicktime_position(file) < udta_atom->end);

	return result;
}


int quicktime_write_udta_string(quicktime_t *file, char *string, int size, int ilst)
{
        quicktime_atom_t data_atom;
        int new_size = strlen(string);
	int result;
        if(!ilst)
          {
          quicktime_write_int16(file, new_size);    /* String size */
          quicktime_write_int16(file, 0);    /* Language code */
          result = quicktime_write_data(file, (uint8_t*)string, new_size);
          }
        else
          {
          quicktime_atom_write_header(file, &data_atom, "data");
          quicktime_write_int32(file, 0x0000001); /* Version (0), flags 0x00001 (text) */
          quicktime_write_int32(file, 0); /* Reserved */
          quicktime_write_data(file, string, strlen(string));
          quicktime_atom_write_footer(file, &data_atom);
          }
	return !result;
}

void quicktime_write_udta(quicktime_t *file, quicktime_udta_t *udta)
{
quicktime_atom_t atom, subatom, meta_atom, ilst_atom, data_atom;
        quicktime_hdlr_t hdlr;
        int tmp;
        int have_ilst = !!(file->file_type & ILST_TYPES);
        //        fprintf(stderr, "quicktime_write_udta, type: %s, ilst: %d\n",
        //                lqt_file_type_to_string(file->file_type), have_ilst);
        quicktime_atom_write_header(file, &atom, "udta");

        if(have_ilst)
          {
          //          fprintf(stderr, "Writing ilst\n");
          quicktime_atom_write_header(file, &meta_atom, "meta");
          quicktime_write_int32(file, 0); /* Version and flags (probably 0) */

          quicktime_hdlr_init_udta(&hdlr);
          quicktime_write_hdlr(file, &hdlr);
          quicktime_atom_write_header(file, &ilst_atom, "ilst");
          }
        
	if(udta->copyright_len)
	{
		quicktime_atom_write_header(file, &subatom, copyright_id);
		quicktime_write_udta_string(file, udta->copyright, udta->copyright_len, have_ilst);
		quicktime_atom_write_footer(file, &subatom);
	}

	if(udta->name_len)
	{
		quicktime_atom_write_header(file, &subatom, name_id);
		quicktime_write_udta_string(file, udta->name, udta->name_len, have_ilst);
		quicktime_atom_write_footer(file, &subatom);
	}

	if(udta->info_len)
	{
		quicktime_atom_write_header(file, &subatom, info_id);
		quicktime_write_udta_string(file, udta->info, udta->info_len, have_ilst);
		quicktime_atom_write_footer(file, &subatom);
	}

        if(udta->artist_len)
	{
		quicktime_atom_write_header(file, &subatom, artist_id);
		quicktime_write_udta_string(file, udta->artist, udta->artist_len, have_ilst);
		quicktime_atom_write_footer(file, &subatom);
	}

        if(udta->album_len)
	{
		quicktime_atom_write_header(file, &subatom, album_id);
		quicktime_write_udta_string(file, udta->album, udta->album_len, have_ilst);
		quicktime_atom_write_footer(file, &subatom);
	}

        if(udta->genre_len)
	{
		quicktime_atom_write_header(file, &subatom, genre_id);
		quicktime_write_udta_string(file, udta->genre, udta->genre_len, have_ilst);
		quicktime_atom_write_footer(file, &subatom);
	}

        if(udta->track_len)
	{
        if(have_ilst)
          {
          tmp = atoi(udta->track);
          quicktime_atom_write_header(file, &subatom, trkn_id);
          quicktime_atom_write_header(file, &data_atom, "data");
          quicktime_write_int32(file, 0); /* Version and flags */
          quicktime_write_int32(file, 0); /* Reserved (0) */
          quicktime_write_int16(file, 0); /* Emtpy */
          quicktime_write_int16(file, tmp);
          quicktime_write_int16(file, 0); /* Total tracks */
          quicktime_write_int16(file, 0); /* Empty */
          quicktime_atom_write_footer(file, &data_atom);
          quicktime_atom_write_footer(file, &subatom);
          }
        else
          {
          quicktime_atom_write_header(file, &subatom, track_id);
          quicktime_write_udta_string(file, udta->track, udta->track_len, have_ilst);
          quicktime_atom_write_footer(file, &subatom);
          }
	}

        if(udta->comment_len)
	{
		quicktime_atom_write_header(file, &subatom, comment_id);
		quicktime_write_udta_string(file, udta->comment, udta->comment_len, have_ilst);
		quicktime_atom_write_footer(file, &subatom);
	}
        if(udta->author_len)
	{
		quicktime_atom_write_header(file, &subatom, author_id);
		quicktime_write_udta_string(file, udta->author, udta->author_len, have_ilst);
		quicktime_atom_write_footer(file, &subatom);
	}
        if(udta->is_qtvr)
	{
		if (quicktime_match_32(udta->ctyp, "stna")) quicktime_write_navg(file, &(udta->navg));
		
	    	quicktime_atom_write_header(file, &subatom, "ctyp");
		quicktime_write_char(file, udta->ctyp[0]);
		quicktime_write_char(file, udta->ctyp[1]);
		quicktime_write_char(file, udta->ctyp[2]);
		quicktime_write_char(file, udta->ctyp[3]);
		quicktime_atom_write_footer(file, &subatom);
	}	
        if(have_ilst)
          {
          quicktime_atom_write_footer(file, &ilst_atom);
          quicktime_atom_write_footer(file, &meta_atom);
          
          }
   
        quicktime_atom_write_footer(file, &atom);
}

int quicktime_set_udta_string(char **string, int *size, char *new_string)
{
	if(*size) free(*string);
	*size = strlen(new_string);
	*string = malloc(*size + 1);
	strcpy(*string, new_string);
	return 0;
}
