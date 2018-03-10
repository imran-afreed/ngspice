/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Gordon Jacobs
Modified: 2001 Jon Engelbert
**********/

#include "ngspice/ngspice.h"
#include "ngspice/cktdefs.h"
#include "cswdefs.h"
#include "ngspice/fteext.h"
#include "ngspice/trandefs.h"
#include "ngspice/sperror.h"
#include "ngspice/suffix.h"

static void
verify(int state, char *msg)
{
    switch (state) {
    case REALLY_ON:
    case REALLY_OFF:
    case HYST_ON:
    case HYST_OFF:
        break;
    default:
        internalerror(msg);
    }
}


int
CSWload(GENmodel *inModel, CKTcircuit *ckt)
{
    CSWmodel *model = (CSWmodel *) inModel;
    CSWinstance *here;
    double g_now;
    double i_ctrl;
    double previous_state = -1;
    double current_state = -1, old_current_state = -1;

    for (; model; model = CSWnextModel(model))
        for (here = CSWinstances(model); here; here = CSWnextInstance(here)) {

            old_current_state = ckt->CKTstate0[here->CSWswitchstate];
            previous_state = ckt->CKTstate1[here->CSWswitchstate];
            i_ctrl = ckt->CKTrhsOld[here->CSWcontBranch];

            /* decide the state of the switch */

            if (ckt->CKTmode & (MODEINITFIX | MODEINITJCT)) {

                if (here->CSWzero_stateGiven) {
                    /* switch specified "on" */
                    if (i_ctrl > model->CSWiThreshold + fabs(model->CSWiHysteresis))
                        current_state = REALLY_ON;
                    else
                        current_state = HYST_ON;
                } else {
                    if (i_ctrl < model->CSWiThreshold - fabs(model->CSWiHysteresis))
                        current_state = REALLY_OFF;
                    else
                        current_state = HYST_OFF;
                }

            } else if (ckt->CKTmode & (MODEINITSMSIG)) {

                current_state = previous_state;

            } else if (ckt->CKTmode & (MODEINITFLOAT)) {
                /* fixme, missleading comment: */
                /* use state0 since INITTRAN or INITPRED already called */

                if (i_ctrl > (model->CSWiThreshold + fabs(model->CSWiHysteresis)))
                    current_state = REALLY_ON;
                else if (i_ctrl < (model->CSWiThreshold - fabs(model->CSWiHysteresis)))
                    current_state = REALLY_OFF;
                else
                    if (model->CSWiHysteresis > 0) {
                        current_state = previous_state;
                    } else {
                        /* in hysteresis... change value if going from low to hysteresis,
                         * or from hi to hysteresis. */

                        verify(previous_state, "bad value for previous_state in swload");

                        /* if previous state was in hysteresis, then don't change the state.. */
                        if (previous_state == REALLY_ON)
                            current_state = HYST_ON;
                        else if (previous_state == REALLY_OFF)
                            current_state = HYST_OFF;
                        else
                            current_state = previous_state;
                    }

                if ((current_state == REALLY_ON || current_state == HYST_ON) != (old_current_state == REALLY_ON || old_current_state == HYST_ON)) {
                    ckt->CKTnoncon++;    /* ensure one more iteration */
                    ckt->CKTtroubleElt = (GENinstance *) here;
                }

            } else if (ckt->CKTmode & (MODEINITTRAN | MODEINITPRED)) {

                if (i_ctrl > (model->CSWiThreshold + fabs(model->CSWiHysteresis)))
                    current_state = REALLY_ON;
                else if (i_ctrl < (model->CSWiThreshold - fabs(model->CSWiHysteresis)))
                    current_state = REALLY_OFF;
                else
                    if (model->CSWiHysteresis > 0) {
                        current_state = previous_state;
                    } else {
                        /* in hysteresis... change value if going from low to hysteresis,
                         * or from hi to hysteresis. */

                        verify(previous_state, "bad value for previous_state in cswload");

                        /* if previous state was in hysteresis, then don't change the state.. */
                        if (previous_state == REALLY_ON)
                            current_state = HYST_ON;
                        else if (previous_state == REALLY_OFF)
                            current_state = HYST_OFF;
                        else
                            current_state = previous_state;
                    }
            }

            ckt->CKTstate0[here->CSWswitchstate] = current_state;
            ckt->CKTstate1[here->CSWswitchstate] = previous_state;

            if (current_state == REALLY_ON || current_state == HYST_ON)
                g_now = model->CSWonConduct;
            else
                g_now = model->CSWoffConduct;

            here->CSWcond = g_now;

            *(here->CSWposPosPtr) += g_now;
            *(here->CSWposNegPtr) -= g_now;
            *(here->CSWnegPosPtr) -= g_now;
            *(here->CSWnegNegPtr) += g_now;
        }

    return OK;
}
