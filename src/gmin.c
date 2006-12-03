

#include <funcprotos.h>
#include <quicktime/quicktime.h>
#include <string.h>

void quicktime_gmin_init(quicktime_gmin_t *gmin)
{
       gmin->version = 0;
       gmin->flags = 0;
       gmin->graphics_mode = 64;
       gmin->opcolor[0] = 32768;
       gmin->opcolor[1] = 32768;
       gmin->opcolor[2] = 32768;
       gmin->balance = 0;
}

void quicktime_gmin_delete(quicktime_gmin_t *gmin) { }

void quicktime_gmin_dump(quicktime_gmin_t *gmin)
{
       lqt_dump("     Base media info\n");
       lqt_dump("      version %d\n", gmin->version);
       lqt_dump("      flags %ld\n", gmin->flags);
       lqt_dump("      graphics_mode %d\n", gmin->graphics_mode);
       lqt_dump("      opcolor %d %d %d\n", gmin->opcolor[0], gmin->opcolor[1], gmin->opcolor[2]);
       lqt_dump("      balance %d\n", gmin->balance);
}

void quicktime_read_gmin(quicktime_t *file, quicktime_gmin_t *gmin)
{
       int i;
       gmin->version = quicktime_read_char(file);
       gmin->flags = quicktime_read_int24(file);
       gmin->graphics_mode = quicktime_read_int16(file);
       for(i = 0; i < 3; i++)
               gmin->opcolor[i] = quicktime_read_int16(file);
       gmin->balance = quicktime_read_int16(file);
}

void quicktime_write_gmin(quicktime_t *file, quicktime_gmin_t *gmin)
{
       quicktime_atom_t atom;
       int i;
       quicktime_atom_write_header(file, &atom, "gmin");

       quicktime_write_char(file, gmin->version);
       quicktime_write_int24(file, gmin->flags);
       quicktime_write_int16(file, gmin->graphics_mode);
       for(i = 0; i < 3; i++)
               quicktime_write_int16(file, gmin->opcolor[i]);
       quicktime_write_int16(file, gmin->balance);
       quicktime_write_int16(file, gmin->reserved);

       quicktime_atom_write_footer(file, &atom);
}

