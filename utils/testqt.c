#include <quicktime/lqt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int main(int argc, char** argv)
{
	char *pcInputFN = argv[1];
	char *pcOutputFN = argv[2];
	char *pcAudioPrefix = argv[3];
	int i;

	quicktime_t *pxQuicktimeInput;
	quicktime_t *pxQuicktimeOutput;

	if( argc != 4 )
	{
		printf( "usage: testqt <input file> <video output file> <audio output prefix>\n"
				"\tReads input file and dumps the video from it to the video\n"
				"\toutput file as a raw RGB MOV and the audio as a\n"
				"\t<prefix>-<channel #>.sw for each channel. The audio files\n"
				"\tare raw 16bit mono at the sampling rate of the original\n"
			);
		exit(3);
	}

	pxQuicktimeInput = quicktime_open( pcInputFN, 1, 0 );
	if( pxQuicktimeInput == NULL )
	{
		printf( "Error opening: %s\n", pcInputFN );
		exit(1);
	}

	pxQuicktimeOutput = quicktime_open( pcOutputFN, 0, 1 );
	if( pxQuicktimeOutput == NULL )
	{
		printf( "Error opening: %s\n", pcOutputFN );
		exit(2);
	}

	{
		int iFrameWidth = quicktime_video_width( pxQuicktimeInput, 0 );
		int iFrameHeight = quicktime_video_height( pxQuicktimeInput, 0 );
		float fFrameRate = quicktime_frame_rate( pxQuicktimeInput, 0 );
		unsigned char **ppFrameBuffer;
		unsigned char *pFrameBufferLinear;
		int iFrameNum = 0;
		int iSamplesPerFrame =
			ceilf( quicktime_sample_rate( pxQuicktimeInput, 0 ) / fFrameRate );
		int iNAudioOutputs = lqt_total_channels( pxQuicktimeInput );
		FILE **pxAudioOutputs;
		int16_t **ppiAudioBuffer;
		int16_t *piAudioBufferLinear;

		if( fFrameRate <= 0 )
			iSamplesPerFrame = 
				ceilf( quicktime_sample_rate( pxQuicktimeInput, 0 ) );
		

		{
			int cpus = 1;
			if( getenv("CPUS") != NULL )
				cpus = atoi(getenv("CPUS"));
			quicktime_set_cpus( pxQuicktimeInput, cpus );
			quicktime_set_cpus( pxQuicktimeOutput, cpus );
		}
		
		quicktime_set_video( pxQuicktimeOutput, 1, iFrameWidth, iFrameHeight,
							 fFrameRate, QUICKTIME_MJPA );
		
		pFrameBufferLinear = malloc( iFrameHeight * iFrameWidth * 3 *
									 sizeof( unsigned char ) + 1000 );
			
		ppFrameBuffer = calloc( iFrameHeight, sizeof( unsigned char * ) );
		for( i = 0; i < iFrameHeight; i++ )
		{
			ppFrameBuffer[i] = pFrameBufferLinear + (iFrameWidth * 3 * i);
		}

		pxAudioOutputs = malloc( iNAudioOutputs * sizeof( FILE * ) );
		
		//piAudioBuffer = malloc( iSamplesPerFrame * sizeof( int16_t ) );
		piAudioBufferLinear = malloc( iNAudioOutputs * iSamplesPerFrame
									  * sizeof( int16_t ) );
			
		ppiAudioBuffer = calloc( iNAudioOutputs, sizeof( int16_t * ) );
		for( i = 0; i < iNAudioOutputs; i++ )
		{
			//ppiAudioBuffer[i] = malloc( iSamplesPerFrame * sizeof( int16_t ) );
			ppiAudioBuffer[i] = piAudioBufferLinear + (iSamplesPerFrame * i);
		}
		
		for( i = 0; i < iNAudioOutputs; i++ )
		{
			char sName[64];
			snprintf( sName, 64, "%s-%02d.sw", pcAudioPrefix, i );
			pxAudioOutputs[i] = fopen( sName, "wb" );
		}

		/*
		  ppFrameBuffer = calloc( iFrameHeight, sizeof( unsigned char * ) );
		  for( i = 0; i < iFrameHeight; i++ )
		  {
		  ppFrameBuffer[i] =  calloc( iFrameWidth * 3, sizeof( unsigned char ) );
		  }
		*/

		while( iFrameNum < quicktime_video_length( pxQuicktimeInput, 0 )
			   || iFrameNum*iSamplesPerFrame < quicktime_audio_length( pxQuicktimeInput, 0 ) )
		{
			//long int iAudioPos = quicktime_audio_position( pxQuicktimeInput, 0 );
			if( iFrameNum < quicktime_video_length( pxQuicktimeInput, 0 ) )
			{
				quicktime_decode_video( pxQuicktimeInput, ppFrameBuffer, 0 );
				quicktime_encode_video( pxQuicktimeOutput, ppFrameBuffer, 0 );
				printf( "v " );
			}

			if( iFrameNum*iSamplesPerFrame < quicktime_audio_length( pxQuicktimeInput, 0 ) )
			{
				lqt_decode_audio( pxQuicktimeInput, ppiAudioBuffer,
								  NULL, iSamplesPerFrame );
				
				for( i = 0; i < iNAudioOutputs; i++ )
				{
				/*
				quicktime_set_audio_position( pxQuicktimeInput, iAudioPos, 0 );
				quicktime_decode_audio( pxQuicktimeInput, ppiAudioBuffer[0],
				NULL, iSamplesPerFrame, i );
				*/
				
					fwrite( ppiAudioBuffer[i], iSamplesPerFrame,
							sizeof( int16_t ),
							pxAudioOutputs[i] );
				}
				printf( "a %d(%d) < %ld ", iFrameNum*iSamplesPerFrame, iSamplesPerFrame, quicktime_audio_length( pxQuicktimeInput, 0 ) );
			}

			printf( "At frame #%d\n", iFrameNum );

			iFrameNum++;
		}

		for( i = 0; i < iNAudioOutputs; i++ )
		{
			fclose( pxAudioOutputs[i] );
		}
		
	}

	quicktime_close(pxQuicktimeOutput);
	quicktime_close(pxQuicktimeInput);
	

	return 0;
}
