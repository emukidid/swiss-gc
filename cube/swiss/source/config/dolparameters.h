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

typedef struct {
	char *value;	// Value passed to DOL
	char *name;		// User friendly name
} ParameterValue;

typedef struct {
	ParameterValue arg;		// The parameter itself
	int num_values;			// How many possible values this parameter has
	int enable;				// Whether this param is used
	int currentValueIdx;	// Current value index
	ParameterValue *values;	// The values
} Parameter;

typedef struct {
	Parameter *parameters;
	int num_params;
} Parameters;

void parseParameters(char *filecontents);
Parameters* getParameters();
void populateArgz(char **argz, size_t *argz_len);
#endif
