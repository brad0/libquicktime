#include <quicktime.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv)
{
	if( argc != 3 )
	{
		printf( "usage: testqt <input file> <output file>\n"
				"\tReads input file and dumps the video from it to the output\n"
				"\tfile as a raw RGB MOV.\n" );
		exit(3);
	}

	char *pcInputFN = argv[1];
	char *pcOutputFN = argv[2];

	quicktime_t *pxQuicktimeInput = quicktime_open( pcInputFN, 1, 0 );
	if( pxQuicktimeInput == NULL )
	{
		printf( "Error opening: %s\n", pcInputFN );
		exit(1);
	}

	quicktime_t *pxQuicktimeOutput = quicktime_open( pcOutputFN, 0, 1 );
	if( pxQuicktimeOutput == NULL )
	{
		printf( "Error opening: %s\n", pcOutputFN );
		exit(2);
	}

	{
		int iFrameWidth = quicktime_video_width( pxQuicktimeInput, 0 );
		int iFrameHeight = quicktime_video_height( pxQuicktimeInput, 0 );
		unsigned char **ppFrameBuffer;
		int i;
		int iFrameNum = 0;

		{
			int cpus = 1;
			if( getenv("CPUS") != NULL )
				cpus = atoi(getenv("CPUS"));
			quicktime_set_cpus( pxQuicktimeInput, cpus );
			quicktime_set_cpus( pxQuicktimeOutput, cpus );
		}
		
		quicktime_set_video( pxQuicktimeOutput, 1, iFrameWidth, iFrameHeight,
							 quicktime_frame_rate( pxQuicktimeInput, 0 ), QUICKTIME_RAW );
		
		ppFrameBuffer = calloc( iFrameHeight, sizeof( unsigned char * ) );
		for( i = 0; i < iFrameHeight; i++ )
		{
			ppFrameBuffer[i] =  calloc( iFrameWidth * 3, sizeof( unsigned char ) );
		}

		while( iFrameNum < quicktime_video_length( pxQuicktimeInput, 0 ) )
		{
			quicktime_decode_video( pxQuicktimeInput, ppFrameBuffer, 0 );
			quicktime_encode_video( pxQuicktimeOutput, ppFrameBuffer, 0 );

			iFrameNum++;
		}
	}

	quicktime_close(pxQuicktimeOutput);
	quicktime_close(pxQuicktimeInput);

	return 0;
}
