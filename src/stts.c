#include <funcprotos.h>
#include <quicktime/quicktime.h>



void quicktime_stts_init(quicktime_stts_t *stts)
{
	stts->version = 0;
	stts->flags = 0;
	stts->total_entries = 0;
}

void quicktime_stts_init_table(quicktime_stts_t *stts)
{
	if(!stts->total_entries)
	{
		stts->total_entries = 1;
		stts->table = (quicktime_stts_table_t*)malloc(sizeof(quicktime_stts_table_t) * stts->total_entries);
	}
}

void quicktime_stts_init_video(quicktime_t *file, quicktime_stts_t *stts, int time_scale, double frame_rate)
{
	quicktime_stts_table_t *table;
	quicktime_stts_init_table(stts);
	table = &(stts->table[0]);

	table->sample_count = 0;      /* need to set this when closing */
	table->sample_duration = (int)(((float)time_scale / frame_rate) + 0.5);
//printf("quicktime_stts_init_video %d %f\n", time_scale, frame_rate);
}

void quicktime_stts_init_audio(quicktime_t *file, quicktime_stts_t *stts, int sample_rate)
{
	quicktime_stts_table_t *table;
	quicktime_stts_init_table(stts);
	table = &(stts->table[0]);

	table->sample_count = 0;     /* need to set this when closing */
	table->sample_duration = 1;
}

void quicktime_stts_delete(quicktime_stts_t *stts)
{
	if(stts->total_entries) free(stts->table);
	stts->total_entries = 0;
}

void quicktime_stts_dump(quicktime_stts_t *stts)
{
	int i;
	printf("     time to sample (stts)\n");
	printf("      version %d\n", stts->version);
	printf("      flags %ld\n", stts->flags);
	printf("      total_entries %ld\n", stts->total_entries);
	for(i = 0; i < stts->total_entries; i++)
	{
		printf("       count %ld duration %ld\n", stts->table[i].sample_count, stts->table[i].sample_duration);
	}
}

void quicktime_read_stts(quicktime_t *file, quicktime_stts_t *stts)
{
	int i;
	stts->version = quicktime_read_char(file);
	stts->flags = quicktime_read_int24(file);
	stts->total_entries = quicktime_read_int32(file);

	stts->table = (quicktime_stts_table_t*)malloc(sizeof(quicktime_stts_table_t) * stts->total_entries);
	for(i = 0; i < stts->total_entries; i++)
	{
		stts->table[i].sample_count = quicktime_read_int32(file);
		stts->table[i].sample_duration = quicktime_read_int32(file);
	}
}

void quicktime_write_stts(quicktime_t *file, quicktime_stts_t *stts)
{
	int i;
	quicktime_atom_t atom;
	quicktime_atom_write_header(file, &atom, "stts");

	quicktime_write_char(file, stts->version);
	quicktime_write_int24(file, stts->flags);
	quicktime_write_int32(file, stts->total_entries);
	for(i = 0; i < stts->total_entries; i++)
	{
		quicktime_write_int32(file, stts->table[i].sample_count);
		quicktime_write_int32(file, stts->table[i].sample_duration);
	}

	quicktime_atom_write_footer(file, &atom);
}

int64_t quicktime_time_to_sample(quicktime_stts_t *stts, int64_t * time,
                                 int64_t * stts_index, int64_t * stts_count)
  {
  int64_t ret = 0;
  int64_t time_count = 0;

  *stts_index = 0;
  
  while(1)
    {
    if(time_count + stts->table[*stts_index].sample_duration *
       stts->table[*stts_index].sample_count > *time)
      {
      *stts_count = (*time - time_count) / stts->table[*stts_index].sample_duration;

      time_count += *stts_count * stts->table[*stts_index].sample_duration;
      ret += *stts_count;
      break;
      }
    else
      {
      time_count += stts->table[*stts_index].sample_duration *
        stts->table[*stts_index].sample_count;
      ret += stts->table[*stts_index].sample_count;
      *stts_index++;
      }
    }
  *time = time_count;
  return ret;
  }

/* sample = -1 returns the total duration */

int64_t quicktime_sample_to_time(quicktime_stts_t *stts, int64_t sample,
                                 int64_t * stts_index, int64_t * stts_count)
  {
  int64_t ret = 0;
  int64_t sample_count = 0;
  *stts_index = 0;
  
  if(sample < 0)
    {
    for(*stts_index = 0; *stts_index < stts->total_entries; *stts_index++)
      {
      ret += stts->table[*stts_index].sample_duration *
        stts->table[*stts_index].sample_count;
      }
    return ret;
    }
  while(1)
    {
    if(sample_count + stts->table[*stts_index].sample_count > sample)
      {
      *stts_count = (sample - sample_count);

      ret += *stts_count * stts->table[*stts_index].sample_duration;
      break;
      }
    else
      {
      sample_count += stts->table[*stts_index].sample_count;
      ret += stts->table[*stts_index].sample_duration;
      *stts_index++;
      }
    }
  return ret;
  }
