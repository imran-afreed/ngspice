/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1987 Gary W. Ng
Modified: 2000 AlansFixes 
**********/

#include "ngspice/ngspice.h"
#include "mos2defs.h"
#include "ngspice/cktdefs.h"
#include "ngspice/iferrmsg.h"
#include "ngspice/noisedef.h"
#include "ngspice/suffix.h"

/*
 * MOS2noise (mode, operation, firstModel, ckt, data, OnDens)
 *    This routine names and evaluates all of the noise sources
 *    associated with MOSFET's.  It starts with the model *firstModel and
 *    traverses all of its insts.  It then proceeds to any other models
 *    on the linked list.  The total output noise density generated by
 *    all of the MOSFET's is summed with the variable "OnDens".
 */


int
MOS2noise (int mode, int operation, GENmodel *genmodel, CKTcircuit *ckt,
           Ndata *data, double *OnDens)
{
    NOISEAN *job = (NOISEAN *) ckt->CKTcurJob;

    MOS2model *firstModel = (MOS2model *) genmodel;
    MOS2model *model;
    MOS2instance *inst;
    char name[N_MXVLNTH];
    double tempOnoise;
    double tempInoise;
    double noizDens[MOS2NSRCS];
    double lnNdens[MOS2NSRCS];
    int i;

    /* define the names of the noise sources */

    static char *MOS2nNames[MOS2NSRCS] = {       /* Note that we have to keep the order */
	"_rd",              /* noise due to rd */        /* consistent with thestrchr definitions */
	"_rs",              /* noise due to rs */        /* in MOS2defs.h */
	"_id",              /* noise due to id */
	"_1overf",          /* flicker (1/f) noise */
	""                  /* total transistor noise */
    };

    for (model=firstModel; model != NULL; model=model->MOS2nextModel) {
	for (inst=model->MOS2instances; inst != NULL; inst=inst->MOS2nextInstance) {
	    if (inst->MOS2owner != ARCHme) continue;
        
	    switch (operation) {

	    case N_OPEN:

		/* see if we have to to produce a summary report */
		/* if so, name all the noise generators */

		if (job->NStpsSm != 0) {
		    switch (mode) {

		    case N_DENS:
			for (i=0; i < MOS2NSRCS; i++) {
			    (void)sprintf(name,"onoise_%s%s",inst->MOS2name,MOS2nNames[i]);


data->namelist = TREALLOC(IFuid, data->namelist, data->numPlots + 1);
if (!data->namelist) return(E_NOMEM);
		SPfrontEnd->IFnewUid (ckt,
			&(data->namelist[data->numPlots++]),
			NULL, name, UID_OTHER, NULL);
				/* we've added one more plot */


			}
			break;

		    case INT_NOIZ:
			for (i=0; i < MOS2NSRCS; i++) {
			    (void)sprintf(name,"onoise_total_%s%s",inst->MOS2name,MOS2nNames[i]);


data->namelist = TREALLOC(IFuid, data->namelist, data->numPlots + 1);
if (!data->namelist) return(E_NOMEM);
		SPfrontEnd->IFnewUid (ckt,
			&(data->namelist[data->numPlots++]),
			NULL, name, UID_OTHER, NULL);
				/* we've added one more plot */


			    (void)sprintf(name,"inoise_total_%s%s",inst->MOS2name,MOS2nNames[i]);


data->namelist = TREALLOC(IFuid, data->namelist, data->numPlots + 1);
if (!data->namelist) return(E_NOMEM);
		SPfrontEnd->IFnewUid (ckt,
			&(data->namelist[data->numPlots++]),
			NULL, name, UID_OTHER, NULL);
				/* we've added one more plot */


			}
			break;
		    }
		}
		break;

	    case N_CALC:
		switch (mode) {

		case N_DENS:
		    NevalSrc(&noizDens[MOS2RDNOIZ],&lnNdens[MOS2RDNOIZ],
				 ckt,THERMNOISE,inst->MOS2dNodePrime,inst->MOS2dNode,
				 inst->MOS2drainConductance);

		    NevalSrc(&noizDens[MOS2RSNOIZ],&lnNdens[MOS2RSNOIZ],
				 ckt,THERMNOISE,inst->MOS2sNodePrime,inst->MOS2sNode,
				 inst->MOS2sourceConductance);

		    NevalSrc(&noizDens[MOS2IDNOIZ],&lnNdens[MOS2IDNOIZ],
				 ckt,THERMNOISE,inst->MOS2dNodePrime,inst->MOS2sNodePrime,
                                 (2.0/3.0 * fabs(inst->MOS2gm)));

		    NevalSrc(&noizDens[MOS2FLNOIZ], NULL, ckt,
				 N_GAIN,inst->MOS2dNodePrime, inst->MOS2sNodePrime,
				 (double)0.0);
		    noizDens[MOS2FLNOIZ] *= model->MOS2fNcoef * 
				 exp(model->MOS2fNexp *
				 log(MAX(fabs(inst->MOS2cd),N_MINLOG))) /
				 (data->freq * inst->MOS2w *
				 inst->MOS2m * 
				 (inst->MOS2l - 2*model->MOS2latDiff) *
				 model->MOS2oxideCapFactor * model->MOS2oxideCapFactor);
		    lnNdens[MOS2FLNOIZ] = 
				 log(MAX(noizDens[MOS2FLNOIZ],N_MINLOG));

		    noizDens[MOS2TOTNOIZ] = noizDens[MOS2RDNOIZ] +
						     noizDens[MOS2RSNOIZ] +
						     noizDens[MOS2IDNOIZ] +
						     noizDens[MOS2FLNOIZ];
		    lnNdens[MOS2TOTNOIZ] = 
				 log(MAX(noizDens[MOS2TOTNOIZ], N_MINLOG));

		    *OnDens += noizDens[MOS2TOTNOIZ];

		    if (data->delFreq == 0.0) { 

			/* if we haven't done any previous integration, we need to */
			/* initialize our "history" variables                      */

			for (i=0; i < MOS2NSRCS; i++) {
			    inst->MOS2nVar[LNLSTDENS][i] = lnNdens[i];
			}

			/* clear out our integration variables if it's the first pass */

			if (data->freq == job->NstartFreq) {
			    for (i=0; i < MOS2NSRCS; i++) {
				inst->MOS2nVar[OUTNOIZ][i] = 0.0;
				inst->MOS2nVar[INNOIZ][i] = 0.0;
			    }
			}
		    } else {   /* data->delFreq != 0.0 (we have to integrate) */
			for (i=0; i < MOS2NSRCS; i++) {
			    if (i != MOS2TOTNOIZ) {
				tempOnoise = Nintegrate(noizDens[i], lnNdens[i],
				      inst->MOS2nVar[LNLSTDENS][i], data);
				tempInoise = Nintegrate(noizDens[i] * data->GainSqInv ,
				      lnNdens[i] + data->lnGainInv,
				      inst->MOS2nVar[LNLSTDENS][i] + data->lnGainInv,
				      data);
				inst->MOS2nVar[LNLSTDENS][i] = lnNdens[i];
				data->outNoiz += tempOnoise;
				data->inNoise += tempInoise;
				if (job->NStpsSm != 0) {
				    inst->MOS2nVar[OUTNOIZ][i] += tempOnoise;
				    inst->MOS2nVar[OUTNOIZ][MOS2TOTNOIZ] += tempOnoise;
				    inst->MOS2nVar[INNOIZ][i] += tempInoise;
				    inst->MOS2nVar[INNOIZ][MOS2TOTNOIZ] += tempInoise;
                                }
			    }
			}
		    }
		    if (data->prtSummary) {
			for (i=0; i < MOS2NSRCS; i++) {     /* print a summary report */
			    data->outpVector[data->outNumber++] = noizDens[i];
			}
		    }
		    break;

		case INT_NOIZ:        /* already calculated, just output */
		    if (job->NStpsSm != 0) {
			for (i=0; i < MOS2NSRCS; i++) {
			    data->outpVector[data->outNumber++] = inst->MOS2nVar[OUTNOIZ][i];
			    data->outpVector[data->outNumber++] = inst->MOS2nVar[INNOIZ][i];
			}
		    }    /* if */
		    break;
		}    /* switch (mode) */
		break;

	    case N_CLOSE:
		return (OK);         /* do nothing, the main calling routine will close */
		break;               /* the plots */
	    }    /* switch (operation) */
	}    /* for inst */
    }    /* for model */

return(OK);
}
