/* -----------------------------------------------------------
      Dolparameters.h - Reads and parses a set of possible 
			argv parameters and values so that they can be 
			displayed to the user later for selection.
	
	- by emu_kidid for Swiss 28/07/2015
   ----------------------------------------------------------- */

#ifndef __DOLPARAMETERS_H
#define __DOLPARAMETERS_H

#include <gccore.h>

/** 
ParameterValue is used for everything encapsulated by {}'s below.
It is {actual value, user displayed value}.

Example .dcp (DOL Config Parameters) file format:

Name={--videoMode, Video Mode}
Values={pal, PAL 50}, {ntsc, NTSC}

Name={,Interlace}
Values={--nointerlace, No}, {--interlace, Yes}

Name={--turbo, Turbo (default 2)}
Values={1, 1}, {2, 2}, {3, 3}
*/

#define MAX_PARAMS 16
#define MAX_VALUES_PER_PARAM 8
#define MAX_PARAM_STRING 32

typedef struct {
	char value[MAX_PARAM_STRING];	// Value passed to DOL
	char name[MAX_PARAM_STRING];	// User friendly name
} ParameterValue;

typedef struct {
	ParameterValue arg;	// The parameter itself
	int num_values;		// How many possible values this parameter has
	int enable;			// Whether this param is used
	int currentValueIdx;// Current value index
	ParameterValue values[MAX_VALUES_PER_PARAM];	// The values
} Parameter;

typedef struct {
	Parameter parameters[MAX_PARAMS];
	int num_params;
} Parameters;

void parseParameters(char *filecontents);
Parameters* getParameters();
void populateArgv(int *argc, char *argv[], char *filename);
#endif
