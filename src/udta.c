#include <funcprotos.h>
#include <quicktime/quicktime.h>
#include <string.h>

#define DEFAULT_INFO "Made with Libquicktime"

/* Atom IDs */

static unsigned char copyright_id[] = { 0xa9, 'c', 'p', 'y' };
static unsigned char info_id[]      = { 0xa9, 'i', 'n', 'f' };
static unsigned char name_id[]      = { 0xa9, 'n', 'a', 'm' };
static unsigned char artist_id[]    = { 0xa9, 'A', 'R', 'T' };
static unsigned char album_id[]     = { 0xa9, 'a', 'l', 'b' };
static unsigned char track_id[]     = { 0xa9, 't', 'r', 'k' };
static unsigned char comment_id[]   = { 0xa9, 'c', 'm', 't' };
static unsigned char author_id[]    = { 0xa9, 'a', 'u', 't' };
static unsigned char genre_id[]     = { 0xa9, 'g', 'e', 'n' };

int quicktime_udta_init(quicktime_udta_t *udta)
{
        memset(udta, 0, sizeof(*udta));
	udta->info = malloc(strlen(DEFAULT_INFO) + 1);
	udta->info_len = strlen(DEFAULT_INFO);
	sprintf(udta->info, DEFAULT_INFO);
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
	quicktime_udta_init(udta);
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
	if(udta->is_qtvr) quicktime_navg_dump(&(udta->navg));
}

int quicktime_read_udta_string(quicktime_t *file, char **string, int *size)
{
	int result;
	if(*size) free(*string);
	*size = quicktime_read_int16(file);  /* Size of string */
	quicktime_read_int16(file);  /* Discard language code */
	*string = malloc(*size + 1);
	result = quicktime_read_data(file, *string, *size);
	(*string)[*size] = 0;
	return !result;
}

int quicktime_write_udta_string(quicktime_t *file, char *string, int size)
{
	int new_size = strlen(string);
	int result;
	quicktime_write_int16(file, new_size);    /* String size */
	quicktime_write_int16(file, 0);    /* Language code */
	result = quicktime_write_data(file, string, new_size);
	return !result;
}

int quicktime_read_udta(quicktime_t *file, quicktime_udta_t *udta, quicktime_atom_t *udta_atom)
{
	quicktime_atom_t leaf_atom;
	int result = 0;

	do
	{
		quicktime_atom_read_header(file, &leaf_atom);
		
		if(quicktime_atom_is(&leaf_atom, copyright_id))
		{
			result += quicktime_read_udta_string(file, &(udta->copyright), &(udta->copyright_len));
		}
		else
		if(quicktime_atom_is(&leaf_atom, name_id))
		{
			result += quicktime_read_udta_string(file, &(udta->name), &(udta->name_len));
		}
		else
		if(quicktime_atom_is(&leaf_atom, info_id))
		{
			result += quicktime_read_udta_string(file, &(udta->info), &(udta->info_len));
                }
                else
                if(quicktime_atom_is(&leaf_atom, artist_id))
		{
			result += quicktime_read_udta_string(file, &(udta->artist), &(udta->artist_len));
                }
                else
                if(quicktime_atom_is(&leaf_atom, album_id))
		{
			result += quicktime_read_udta_string(file, &(udta->album), &(udta->album_len));
                }
                else
                if(quicktime_atom_is(&leaf_atom, genre_id))
		{
			result += quicktime_read_udta_string(file, &(udta->genre), &(udta->genre_len));
                }
                else
                if(quicktime_atom_is(&leaf_atom, track_id))
		{
			result += quicktime_read_udta_string(file, &(udta->track), &(udta->track_len));
                }
                else
                if(quicktime_atom_is(&leaf_atom, comment_id))
		{
			result += quicktime_read_udta_string(file, &(udta->comment), &(udta->comment_len));
                }
                else
                if(quicktime_atom_is(&leaf_atom, author_id))
		{
			result += quicktime_read_udta_string(file, &(udta->author), &(udta->author_len));
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
			if (strcmp(udta->ctyp, "stna") ||
			    strcmp(udta->ctyp, "STpn")) udta->is_qtvr = 1;
                }
		else
		quicktime_atom_skip(file, &leaf_atom);
	}while(quicktime_position(file) < udta_atom->end);

	return result;
}

void quicktime_write_udta(quicktime_t *file, quicktime_udta_t *udta)
{
	quicktime_atom_t atom, subatom;
	quicktime_atom_write_header(file, &atom, "udta");


	if(udta->copyright_len)
	{
		quicktime_atom_write_header(file, &subatom, copyright_id);
		quicktime_write_udta_string(file, udta->copyright, udta->copyright_len);
		quicktime_atom_write_footer(file, &subatom);
	}

	if(udta->name_len)
	{
		quicktime_atom_write_header(file, &subatom, name_id);
		quicktime_write_udta_string(file, udta->name, udta->name_len);
		quicktime_atom_write_footer(file, &subatom);
	}

	if(udta->info_len)
	{
		quicktime_atom_write_header(file, &subatom, info_id);
		quicktime_write_udta_string(file, udta->info, udta->info_len);
		quicktime_atom_write_footer(file, &subatom);
	}

        if(udta->artist_len)
	{
		quicktime_atom_write_header(file, &subatom, artist_id);
		quicktime_write_udta_string(file, udta->artist, udta->artist_len);
		quicktime_atom_write_footer(file, &subatom);
	}

        if(udta->album_len)
	{
		quicktime_atom_write_header(file, &subatom, album_id);
		quicktime_write_udta_string(file, udta->album, udta->album_len);
		quicktime_atom_write_footer(file, &subatom);
	}

        if(udta->genre_len)
	{
		quicktime_atom_write_header(file, &subatom, genre_id);
		quicktime_write_udta_string(file, udta->genre, udta->genre_len);
		quicktime_atom_write_footer(file, &subatom);
	}

        if(udta->track_len)
	{
		quicktime_atom_write_header(file, &subatom, track_id);
		quicktime_write_udta_string(file, udta->track, udta->track_len);
		quicktime_atom_write_footer(file, &subatom);
	}

        if(udta->comment_len)
	{
		quicktime_atom_write_header(file, &subatom, comment_id);
		quicktime_write_udta_string(file, udta->comment, udta->comment_len);
		quicktime_atom_write_footer(file, &subatom);
	}
        if(udta->author_len)
	{
		quicktime_atom_write_header(file, &subatom, author_id);
		quicktime_write_udta_string(file, udta->author, udta->author_len);
		quicktime_atom_write_footer(file, &subatom);
	}
        if(udta->is_qtvr)
	{
		quicktime_write_navg(file, &(udta->navg));
		
	    	quicktime_atom_write_header(file, &subatom, "ctyp");
		quicktime_write_char(file, udta->ctyp[0]);
		quicktime_write_char(file, udta->ctyp[1]);
		quicktime_write_char(file, udta->ctyp[2]);
		quicktime_write_char(file, udta->ctyp[3]);
		quicktime_atom_write_footer(file, &subatom);
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
