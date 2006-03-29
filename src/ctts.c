#include <funcprotos.h>
#include <quicktime/quicktime.h>

void quicktime_ctts_init(quicktime_ctts_t *ctts)
  {
  
  }

void quicktime_ctts_delete(quicktime_ctts_t *ctts)
  {
  if(ctts->table)
    free(ctts->table);
  }

void quicktime_ctts_dump(quicktime_ctts_t *ctts)
  {
  int i;
  printf("     composition time to sample (ctts)\n");
  printf("      version %d\n", ctts->version);
  printf("      flags %ld\n", ctts->flags);
  printf("      total_entries %ld\n", ctts->total_entries);
  for(i = 0; i < ctts->total_entries; i++)
    {
    printf("       count %ld duration %ld\n", ctts->table[i].sample_count,
           ctts->table[i].sample_duration);
    }
  }

void quicktime_read_ctts(quicktime_t *file, quicktime_ctts_t *ctts)
  {
  int i;
  ctts->version = quicktime_read_char(file);
  ctts->flags = quicktime_read_int24(file);
  ctts->total_entries = quicktime_read_int32(file);
  
  ctts->table = malloc(sizeof(quicktime_ctts_table_t) * ctts->total_entries);
  for(i = 0; i < ctts->total_entries; i++)
    {
    ctts->table[i].sample_count = quicktime_read_int32(file);
    ctts->table[i].sample_duration = quicktime_read_int32(file);
    }
  }

void quicktime_write_ctts(quicktime_t *file, quicktime_ctts_t *ctts)
  {
  int i;
  quicktime_atom_t atom;
  quicktime_atom_write_header(file, &atom, "ctts");

  quicktime_write_char(file, ctts->version);
  quicktime_write_int24(file, ctts->flags);
  quicktime_write_int32(file, ctts->total_entries);
  for(i = 0; i < ctts->total_entries; i++)
    {
    quicktime_write_int32(file, ctts->table[i].sample_count);
    quicktime_write_int32(file, ctts->table[i].sample_duration);
    }

  quicktime_atom_write_footer(file, &atom);

  }
