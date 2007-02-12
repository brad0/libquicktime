#include "lqt_private.h"

void quicktime_matrix_init(quicktime_matrix_t *matrix)
{
	int i;
	for(i = 0; i < 9; i++) matrix->values[i] = 0;
	matrix->values[0] = matrix->values[4] = 1;
	matrix->values[8] = 16384;
}

void quicktime_matrix_delete(quicktime_matrix_t *matrix)
{
}

void quicktime_read_matrix(quicktime_t *file, quicktime_matrix_t *matrix)
{
	int i = 0;
	for(i = 0; i < 9; i++)
	{
		matrix->values[i] = quicktime_read_fixed32(file);
	}
}

void quicktime_matrix_dump(quicktime_matrix_t *matrix)
{
	int i;
	lqt_dump("   matrix");
	for(i = 0; i < 9; i++) lqt_dump(" %f", matrix->values[i]);
	lqt_dump("\n");
}

void quicktime_write_matrix(quicktime_t *file, quicktime_matrix_t *matrix)
{
	int i;
	for(i = 0; i < 9; i++)
	{
		quicktime_write_fixed32(file, matrix->values[i]);
	}
}
