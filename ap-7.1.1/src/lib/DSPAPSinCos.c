/*
 * DSPAPSinCos.c
 *		Generate sin and cos tables for use with AP functions.
 *
 *	Modification History:
 *	05/22/89/mtm - new file
 *	04/18/90/mtm - import stdlib.h
 */
 
#import <math.h>
#import <stdlib.h>
#import <dsp/dsp.h>

/* private functions from libdsp */
extern int _DSPErr();

/*
 * DSPAPSinTable
 *		Generate a 1/2 period (num_points / 2) negative sine table at
 *		frequency 1/num_points.
 */
float	*DSPAPSinTable(int num_points)
{
	float	*sin_table;		/* Pointer to generated sin table */
	int		i;
	
	if ((sin_table = (float *) malloc((num_points / 2) * sizeof(float)))
		 == NULL)
		_DSPErr("DSPAPSinTable: insufficient free storage");
		
	for (i = 0; i < num_points / 2; i++)
		sin_table[i] = -sin((i * 2.0 * M_PI) / (float) num_points);
		
	return sin_table;
}


/*
 * DSPAPCosTable
 *		Generate a 1/2 period (num_points / 2) negative cos table at
 *		frequency 1/num_points.
 */
float	*DSPAPCosTable(int num_points)
{
	float	*cos_table;		/* Pointer to generated cos table */
	int		i;
	
	if ((cos_table = (float *) malloc((num_points / 2) * sizeof(float)))
		 == NULL)
		_DSPErr("DSPAPCosTable: insufficient free storage");
		
	for (i = 0; i < num_points / 2; i++)
		cos_table[i] = -cos((i * 2.0 * M_PI) / (float) num_points);
		
	return cos_table;
}



