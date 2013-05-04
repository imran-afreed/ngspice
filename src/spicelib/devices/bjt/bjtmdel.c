/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Thomas L. Quarles
**********/

/*
 * This routine deletes a BJT model from the circuit and frees
 * the storage it was using.
 * returns an error if the model has instances
 */

#include "ngspice/ngspice.h"
#include "bjtdefs.h"
#include "ngspice/sperror.h"
#include "ngspice/suffix.h"


int
BJTmDelete(GENmodel *gen_model)
{
    NG_IGNORE(gen_model);
    return OK;
}