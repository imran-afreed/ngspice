/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1985 Thomas L. Quarles
Modified: 2000 AlansFixes
VDMOS: 2018 Holger Vogt
**********/

#include "ngspice/ngspice.h"
#include "ngspice/cktdefs.h"
#include "ngspice/devdefs.h"
#include "vdmosdefs.h"
#include "ngspice/trandefs.h"
#include "ngspice/const.h"
#include "ngspice/sperror.h"
#include "ngspice/suffix.h"

/* VDMOSlimitlog(vnew,vold)
 *  limits the per-iteration change of any absolute voltage value
 */
static double
VDMOSlimitlog(
    double deltemp,
    double deltemp_old,
    double LIM_TOL,
    int *check)
{
    if (isnan (deltemp) || isnan (deltemp_old))
    {
        fprintf(stderr, "Alberto says:  YOU TURKEY!  The limiting function received NaN.\n");
        fprintf(stderr, "New prediction returns to 0.0!\n");
        deltemp = 0.0;
        *check = 1;
    }
    /* Logarithmic damping of deltemp beyond LIM_TOL */
    if (deltemp > deltemp_old + LIM_TOL) {
        deltemp = deltemp_old + LIM_TOL + log10((deltemp-deltemp_old)/LIM_TOL);
        *check = 1;
    }
    else if (deltemp < deltemp_old - LIM_TOL) {
        deltemp = deltemp_old - LIM_TOL - log10((deltemp_old-deltemp)/LIM_TOL);
        *check = 1;
    }
    return deltemp;
}

int
VDMOSload(GENmodel *inModel, CKTcircuit *ckt)
/* actually load the current value into the
 * sparse matrix previously provided
 */
{
    VDMOSmodel *model = (VDMOSmodel *)inModel;
    VDMOSinstance *here;
    double Beta;
    double arg;
    double cdhat;
    double cdrain;
    double cdreq;
    double ceq;
    double ceqgd;
    double ceqgs;
    double delvds;
    double delvgd;
    double delvgs;
    double gcgd;
    double gcgs;
    double geq;
    double sarg;
    double vds;
    double vdsat;
    double vgd1;
    double vgd;
    double vgdo;
    double vgs1;
    double vgs;
    double von;
    double vt;
#ifndef PREDICTOR
    double xfact = 0.0;
#endif
    int xnrm;
    int xrev;
    double capgs = 0.0;   /* total gate-source capacitance */
    double capgd = 0.0;   /* total gate-drain capacitance */
    double capth = 0.0;   /* total thermal capacitance */
    int Check_mos, Check_diode;
    int error;

    register int selfheat;
    double BetaT, rd0T=0.0, rd1T=0.0, dBetaT_dT, drd0T_dT, drd1T_dT, dIds_dT;
    double deldelTemp, delTemp, delTemp1, Temp, Vds, Vgs;
    double ceqth;
    double GmT, gTtg, gTtdp, gTtt, gTtsp, gcTt;

    /*  loop through all the VDMOS device models */
    for (; model != NULL; model = VDMOSnextModel(model)) {
        /* VDMOS capacitance parameters */
        const double cgdmin = model->VDMOScgdmin;
        const double cgdmax = model->VDMOScgdmax;
        const double a = model->VDMOSa;
        const double cgs = model->VDMOScgs;

        /* loop through all the instances of the model */
        for (here = VDMOSinstances(model); here != NULL;
                here = VDMOSnextInstance(here)) {

            selfheat = (model->VDMOSshMod == 1) && (here->VDMOSrth0 != 0.0);

            vt = CONSTKoverQ * here->VDMOStemp;
            Check_mos = 0;

            /* first, we compute a few useful values - these could be
             * pre-computed, but for historical reasons are still done
             * here.  They may be moved at the expense of instance size
             */

            Beta = here->VDMOStTransconductance * here->VDMOSm *
                   here->VDMOSw / here->VDMOSl;

            delTemp = 0.0;
            if ((ckt->CKTmode & MODEINITSMSIG)) {
                vgs = *(ckt->CKTstate0 + here->VDMOSvgs);
                vds = *(ckt->CKTstate0 + here->VDMOSvds);
                delTemp = *(ckt->CKTstate0 + here->VDMOSdeltemp);
            } else if ((ckt->CKTmode & MODEINITTRAN)) {
                vgs = *(ckt->CKTstate1 + here->VDMOSvgs);
                vds = *(ckt->CKTstate1 + here->VDMOSvds);
                delTemp = *(ckt->CKTstate1 + here->VDMOSdeltemp);
            } else if ((ckt->CKTmode & MODEINITJCT) && !here->VDMOSoff) {
                /* ok - not one of the simple cases, so we have to
                 * look at all of the possibilities for why we were
                 * called.  We still just initialize the two voltages
                 */
                vds = model->VDMOStype * here->VDMOSicVDS;
                vgs = model->VDMOStype * here->VDMOSicVGS;
                delTemp = 0.0;
                if ((vds == 0.0) && (vgs == 0.0) &&
                        ((ckt->CKTmode & (MODETRAN | MODEAC|MODEDCOP |
                                          MODEDCTRANCURVE)) || (!(ckt->CKTmode & MODEUIC))))
                {
                    vgs = model->VDMOStype * model->VDMOSvth0 + 0.1;
                    vds = 0.0;
                }
            } else if ((ckt->CKTmode & (MODEINITJCT | MODEINITFIX)) && (here->VDMOSoff)) {
                delTemp = vgs = vds = 0.0;

            /*
             * ok - now to do the start-up operations
             *
             * we must get values for vds and vgs from somewhere
             * so we either predict them or recover them from last iteration
             * These are the two most common cases - either a prediction
             * step or the general iteration step and they
             * share some code, so we put them first - others later on
             */
            }
            else
            {
#ifndef PREDICTOR
                if (ckt->CKTmode & MODEINITPRED) {

                    /* predictor step */

                    xfact = ckt->CKTdelta / ckt->CKTdeltaOld[1];
                    *(ckt->CKTstate0 + here->VDMOSvgs) =
                        *(ckt->CKTstate1 + here->VDMOSvgs);
                    vgs = (1 + xfact)* (*(ckt->CKTstate1 + here->VDMOSvgs))
                          - (xfact * (*(ckt->CKTstate2 + here->VDMOSvgs)));
                    *(ckt->CKTstate0 + here->VDMOSvds) =
                        *(ckt->CKTstate1 + here->VDMOSvds);
                    vds = (1 + xfact)* (*(ckt->CKTstate1 + here->VDMOSvds))
                          - (xfact * (*(ckt->CKTstate2 + here->VDMOSvds)));
                    *(ckt->CKTstate0 + here->VDMOSdeltemp) =
                        *(ckt->CKTstate1 + here->VDMOSdeltemp);
                    delTemp = (1 + xfact)* (*(ckt->CKTstate1 + here->VDMOSdeltemp))
                          - (xfact * (*(ckt->CKTstate2 + here->VDMOSdeltemp)));
                }
                else
                {
#endif /* PREDICTOR */

                    /* general iteration */

                    vgs = model->VDMOStype * (
                              *(ckt->CKTrhsOld + here->VDMOSgNodePrime) -
                              *(ckt->CKTrhsOld + here->VDMOSsNodePrime));
                    vds = model->VDMOStype * (
                              *(ckt->CKTrhsOld + here->VDMOSdNodePrime) -
                              *(ckt->CKTrhsOld + here->VDMOSsNodePrime));
                    if (selfheat)
                        delTemp = *(ckt->CKTrhsOld + here->VDMOStempNode);
                    else
                        delTemp = 0.0;
#ifndef PREDICTOR
                }
#endif /* PREDICTOR */

                /* now some common crunching for some more useful quantities */

                vgd = vgs - vds;
                vgdo = *(ckt->CKTstate0 + here->VDMOSvgs) -
                       *(ckt->CKTstate0 + here->VDMOSvds);
                delvgs = vgs - *(ckt->CKTstate0 + here->VDMOSvgs);
                delvds = vds - *(ckt->CKTstate0 + here->VDMOSvds);
                delvgd = vgd - vgdo;

                deldelTemp = delTemp - *(ckt->CKTstate0 + here->VDMOSdeltemp);

                /* these are needed for convergence testing */

                if (here->VDMOSmode >= 0) {
                    cdhat =
                        here->VDMOScd
                      + here->VDMOSgm * delvgs
                      + here->VDMOSgds * delvds
                      + here->VDMOSgmT * deldelTemp;
                } else {
                    cdhat =
                        here->VDMOScd
                      + here->VDMOSgm * delvgd
                      - here->VDMOSgds * delvds
                      + here->VDMOSgmT * deldelTemp;
                }

#ifndef NOBYPASS
                /* now lets see if we can bypass (ugh) */
                if ((!(ckt->CKTmode & MODEINITPRED)) &&
                        (ckt->CKTbypass) &&
                        (fabs(delvgs) < (ckt->CKTreltol *
                                         MAX(fabs(vgs),
                                             fabs(*(ckt->CKTstate0 +
                                                    here->VDMOSvgs))) +
                                         ckt->CKTvoltTol)) &&
                            (fabs(delvds) < (ckt->CKTreltol *
                                             MAX(fabs(vds),
                                                 fabs(*(ckt->CKTstate0 +
                                                        here->VDMOSvds))) +
                                             ckt->CKTvoltTol)) &&
                                (fabs(cdhat - here->VDMOScd) < (ckt->CKTreltol *
                                                                MAX(fabs(cdhat),
                                                                        fabs(here->VDMOScd)) +
                                                                ckt->CKTabstol)) &&
                                    ((here->VDMOStempNode == 0) ||
                                            (fabs(deldelTemp) < (ckt->CKTreltol * MAX(fabs(delTemp),
                                                                                      fabs(*(ckt->CKTstate0+here->VDMOSdeltemp)))
                                                                 + ckt->CKTvoltTol*1e4))))
                                        {
                                            /* bypass code */
                                            /* nothing interesting has changed since last
                                             * iteration on this device, so we just
                                             * copy all the values computed last iteration out
                                             * and keep going
                                             */
                                            vgs = *(ckt->CKTstate0 + here->VDMOSvgs);
                                            vds = *(ckt->CKTstate0 + here->VDMOSvds);
                                            vgd = vgs - vds;
                                            delTemp = *(ckt->CKTstate0 + here->VDMOSdeltemp);
                                            /*  calculate Vds for temperature conductance calculation
                                                in bypass (used later when filling Temp node matrix)  */
                                            Vds = here->VDMOSmode > 0 ? vds : -vds;
                                            cdrain = here->VDMOSmode * (here->VDMOScd);
                                            if (ckt->CKTmode & (MODETRAN | MODETRANOP)) {
                                                capgs = (*(ckt->CKTstate0 + here->VDMOScapgs) +
                                                         *(ckt->CKTstate1 + here->VDMOScapgs));
                                                capgd = (*(ckt->CKTstate0 + here->VDMOScapgd) +
                                                         *(ckt->CKTstate1 + here->VDMOScapgd));
                                                capth = (*(ckt->CKTstate0 + here->VDMOScapth) +
                                                         *(ckt->CKTstate1 + here->VDMOScapth));
                                            }
                                            goto bypass;
                                        }
#endif /*NOBYPASS*/

                /* ok - bypass is out, do it the hard way */

                von = model->VDMOStype * here->VDMOSvon;

#ifndef NODELIMITING
                /*
                 * limiting
                 *  we want to keep device voltages from changing
                 * so fast that the exponentials churn out overflows
                 * and similar rudeness
                 */

                if (*(ckt->CKTstate0 + here->VDMOSvds) >= 0) {
                    vgs = DEVfetlim(vgs, *(ckt->CKTstate0 + here->VDMOSvgs)
                                    , von);
                    vds = vgs - vgd;
                    vds = DEVlimvds(vds, *(ckt->CKTstate0 + here->VDMOSvds));
                    vgd = vgs - vds;
                } else {
                    vgd = DEVfetlim(vgd, vgdo, von);
                    vds = vgs - vgd;
                    if (!(ckt->CKTfixLimit)) {
                        vds = -DEVlimvds(-vds, -(*(ckt->CKTstate0 +
                                                   here->VDMOSvds)));
                    }
                    vgs = vgd + vds;
                }
                if (selfheat)
                    delTemp = VDMOSlimitlog(delTemp,
                          *(ckt->CKTstate0 + here->VDMOSdeltemp),1.0,&Check_mos);
                else
                    delTemp = 0.0;
#endif /*NODELIMITING*/

            }

            Temp = delTemp + ckt->CKTtemp;
            here->VDMOSTempSH = Temp; /* added for portability of SH Temp for noise analysis */

            /*  Calculate temperature dependent values for self-heating effect  */
            if (selfheat) {
                double TempRatio = Temp / model->VDMOStnom;
                BetaT = Beta * pow(TempRatio,-model->VDMOSmu);
                dBetaT_dT = -Beta * model->VDMOSmu / (model->VDMOStnom * pow(TempRatio,1+model->VDMOSmu));
                rd0T = 0.0;
                drd0T_dT = 0.0;
                rd1T = 0.0;
                drd1T_dT = 0.0;
                if ((model->VDMOSdrainResistanceGiven) && (model->VDMOSdrainResistance != 0)) {
                    rd0T =  model->VDMOSdrainResistance / here->VDMOSm * pow(TempRatio, model->VDMOStexp0);
                    drd0T_dT = rd0T * model->VDMOStexp0 / Temp;
                }
                if ((model->VDMOSqsResistanceGiven) && (model->VDMOSqsResistance != 0)) {
                    rd1T = model->VDMOSqsResistance * pow(TempRatio, model->VDMOStexp1);
                    drd1T_dT = rd1T * model->VDMOStexp1 / Temp;
                }
            } else {
                BetaT = Beta;
                dBetaT_dT = 0.0;
                rd0T = model->VDMOSdrainResistance / here->VDMOSm;
                drd0T_dT = 0.0;
                rd1T = model->VDMOSqsResistance;
                drd1T_dT = 0.0;
            }

            /*
             * now all the preliminaries are over - we can start doing the
             * real work
             */
            vgd = vgs - vds;


            /* now to determine whether the user was able to correctly
             * identify the source and drain of his device
             */
            if (vds >= 0) {
                /* normal mode */
                here->VDMOSmode = 1;
                Vds = vds;
                Vgs = vgs;
            } else {
                /* inverse mode */
                here->VDMOSmode = -1;
                Vds = -vds;
                Vgs = vgd;
            }

            {
                /*
                 *     this block of code evaluates the drain current and its
                 *     derivatives using the shichman-hodges model and the
                 *     charges associated with the gate, channel and bulk for
                 *     mosfets
                 *
                 */

                /* the following 2 variables are local to this code block until
                 * it is obvious that they can be made global
                 */
                double betap;
                double vgst;

                von = here->VDMOStVth * model->VDMOStype;
                vgst = (here->VDMOSmode == 1 ? vgs : vgd) - von;
                vdsat = MAX(vgst, 0);
                if (model->VDMOSksubthresGiven) {
                /* Simple weak inversion model, according to https://www.anasoft.co.uk/MOS1Model.htm
                 * Scale the voltage overdrive vgst logarithmically in weak inversion.
                 * Best fits LTSPICE curves with shift=0
                 */
                    double slope = model->VDMOSksubthres;
                    double lambda = model->VDMOSlambda;
                    double theta = model->VDMOStheta;
                    double shift = model->VDMOSsubshift;
                    double mtr = model->VDMOSmtr;

                    /* scale vds with mtr (except with lambda) */
                    double vdss = vds*mtr*here->VDMOSmode;
                    double t0 = 1 + lambda*vds;
                    double t1 = 1 + theta*vgs;
                    betap = BetaT*t0/t1;
                    double dbetapdvgs = -BetaT*theta*t0/(t1*t1);
                    double dbetapdvds = BetaT*lambda/t1;
                    double dbetapdT = dBetaT_dT*t0/t1;

                    double t2 = exp((vgst-shift)/slope);
                    vgst = slope * log(1 + t2);
                    double dvgstdvgs = t2/(t2+1);

                    if (vgst <= vdss) {
                        /* saturation region */
                        cdrain = betap * vgst*vgst * .5;
                        here->VDMOSgm = betap*vgst*dvgstdvgs + 0.5*dbetapdvgs*vgst*vgst;
                        here->VDMOSgds = .5*dbetapdvds*vgst*vgst;
                        dIds_dT = dbetapdT * vgst*vgst * .5;
                    }
                    else {
                        /* linear region */
                        cdrain = betap * vdss * (vgst - .5 * vdss);
                        here->VDMOSgm = betap*vdss*dvgstdvgs + vdss*dbetapdvgs*(vgst-.5*vdss);
                        here->VDMOSgds = vdss*dbetapdvds*(vgst-.5*vdss) + betap*mtr*(vgst-.5*vdss) - .5*vdss*betap*mtr;
                        dIds_dT = dbetapdT * vdss * (vgst - .5 * vdss);
                    }
                } else {
                    double onfg, fgate, Betam, dfgdvg;
                    onfg = 1.0+model->VDMOStheta*vgst;
                    fgate = 1.0/onfg;
                    Betam = BetaT * fgate;
                    dfgdvg = -model->VDMOStheta*fgate*fgate;
                    if (vgst <= 0) {
                        /*
                         *     cutoff region
                         */
                        cdrain = 0;
                        here->VDMOSgm = 0;
                        here->VDMOSgds = 0;
                    } else {
                        /* scale vds with mtr */
                        double mtr = model->VDMOSmtr;
                        betap = Betam*(1 + model->VDMOSlambda*(vds*here->VDMOSmode));
                        if (vgst <= (vds * here->VDMOSmode) * mtr) {
                            /*
                             *     saturation region
                             */
                            cdrain = betap*vgst*vgst*.5;
                            here->VDMOSgm = betap*vgst * fgate + dfgdvg * cdrain;
                            here->VDMOSgds = model->VDMOSlambda*Betam*vgst*vgst*.5;
                        } else {
                            /*
                             *     linear region
                             */
                            cdrain = betap * (vds * here->VDMOSmode) * mtr *
                                     (vgst - .5 * (vds*here->VDMOSmode) * mtr);
                            here->VDMOSgm = betap * (vds * here->VDMOSmode) * mtr  * fgate + dfgdvg * cdrain;
                            here->VDMOSgds = betap * (vgst - (vds * here->VDMOSmode) * mtr) +
                                             model->VDMOSlambda * Betam *
                                             (vds * here->VDMOSmode) * mtr *
                                             (vgst - .5 * (vds * here->VDMOSmode) * mtr);
                        }
                    }
                    dIds_dT = 0.0;
                }
            }


            /* now deal with n vs p polarity */

            here->VDMOSvon = model->VDMOStype * von;
            here->VDMOSvdsat = model->VDMOStype * vdsat;

            /*
             *  COMPUTE EQUIVALENT DRAIN CURRENT SOURCE
             */
            here->VDMOScd = here->VDMOSmode * cdrain;

            /* save things away for next time */

            *(ckt->CKTstate0 + here->VDMOSvgs) = vgs;
            *(ckt->CKTstate0 + here->VDMOSvds) = vds;
            *(ckt->CKTstate0 + here->VDMOSdeltemp) = delTemp;

            /* quasi saturation
             * according to Vincenzo d'Alessandro's Quasi-Saturation Model, simplified:
             V. D'Alessandro, F. Frisina, N. Rinaldi: A New SPICE Model of VDMOS Transistors
             Including Thermal and Quasi-saturation Effects, 9th European Conference on Power
             Electronics and applications (EPE), Graz, Austria, August 2001, pp. P.1 − P.10.
             */
            if (model->VDMOSqsGiven && (here->VDMOSmode == 1)) {
                double vdsn = model->VDMOStype * (
                    *(ckt->CKTrhsOld + here->VDMOSdNode) -
                    *(ckt->CKTrhsOld + here->VDMOSsNode));
                double rd = rd0T + rd1T * (vdsn / (vdsn + fabs(model->VDMOSqsVoltage)));
                if (rd > 0)
                    here->VDMOSdrainConductance = 1 / rd + ckt->CKTgmin;
                else
                    here->VDMOSdrainConductance = 0.0;
            } else {
                if ((selfheat) && (rd0T > 0))
                    here->VDMOSdrainConductance = 1 / rd0T;
            }

            if (selfheat) {
                GmT = dIds_dT;
                here->VDMOSgmT = GmT;
            } else {
                GmT = 0.0;
                here->VDMOSgmT = 0.0;
            }

            if (selfheat) {
                /*  note that sign is switched because power flows out
                    of device into the temperature node. */
                here->VDMOSgtempg = -model->VDMOStype*here->VDMOSgm * Vds;
                here->VDMOSgtempT = -GmT * Vds;
                here->VDMOSgtempd = -model->VDMOStype* (here->VDMOSgds * Vds + cdrain);
                here->VDMOScth = - cdrain * Vds
                                 - 1/here->VDMOSdrainConductance * cdrain*cdrain
                                 - model->VDMOStype * (here->VDMOSgtempg * Vgs + here->VDMOSgtempd * Vds)
                                 - here->VDMOSgtempT * delTemp;
            }

            /*
             * vdmos capacitor model
             */
            if (ckt->CKTmode & (MODETRAN | MODETRANOP | MODEINITSMSIG)) {
                /*
                 * calculate gate - drain, gate - source capacitors
                 * drain-source capacitor is evaluated with the bulk diode below
                 */
                /*
                 * this just evaluates at the current time,
                 * expects you to remember values from previous time
                 * returns 1/2 of non-constant portion of capacitance
                 * you must add in the other half from previous time
                 * and the constant part
                 */
                DevCapVDMOS(vgd, cgdmin, cgdmax, a, cgs,
                            (ckt->CKTstate0 + here->VDMOScapgs),
                            (ckt->CKTstate0 + here->VDMOScapgd));
                *(ckt->CKTstate0 + here->VDMOScapth) = here->VDMOScth0; /* always constant */

                vgs1 = *(ckt->CKTstate1 + here->VDMOSvgs);
                vgd1 = vgs1 - *(ckt->CKTstate1 + here->VDMOSvds);
                delTemp1 = *(ckt->CKTstate1 + here->VDMOSdeltemp);
                if (ckt->CKTmode & (MODETRANOP | MODEINITSMSIG)) {
                    capgs = 2 * *(ckt->CKTstate0 + here->VDMOScapgs);
                    capgd = 2 * *(ckt->CKTstate0 + here->VDMOScapgd);
                    capth = 2 * *(ckt->CKTstate0 + here->VDMOScapth);
                } else {
                    capgs = (*(ckt->CKTstate0 + here->VDMOScapgs) +
                             *(ckt->CKTstate1 + here->VDMOScapgs));
                    capgd = (*(ckt->CKTstate0 + here->VDMOScapgd) +
                             *(ckt->CKTstate1 + here->VDMOScapgd));
                    capth = (*(ckt->CKTstate0 + here->VDMOScapth) +
                             *(ckt->CKTstate1 + here->VDMOScapth));
                }

#ifndef PREDICTOR
                if (ckt->CKTmode & (MODEINITPRED | MODEINITTRAN)) {
                    *(ckt->CKTstate0 + here->VDMOSqgs) =
                        (1 + xfact) * *(ckt->CKTstate1 + here->VDMOSqgs)
                        - xfact * *(ckt->CKTstate2 + here->VDMOSqgs);
                    *(ckt->CKTstate0 + here->VDMOSqgd) =
                        (1 + xfact) * *(ckt->CKTstate1 + here->VDMOSqgd)
                        - xfact * *(ckt->CKTstate2 + here->VDMOSqgd);
                    *(ckt->CKTstate0 + here->VDMOSqth) =
                        (1 + xfact) * *(ckt->CKTstate1 + here->VDMOSqth)
                        - xfact * *(ckt->CKTstate2 + here->VDMOSqth);
                } else {
#endif /*PREDICTOR*/
                    if (ckt->CKTmode & MODETRAN) {
                        *(ckt->CKTstate0 + here->VDMOSqgs) = (vgs - vgs1)*capgs +
                                                             *(ckt->CKTstate1 + here->VDMOSqgs);
                        *(ckt->CKTstate0 + here->VDMOSqgd) = (vgd - vgd1)*capgd +
                                                             *(ckt->CKTstate1 + here->VDMOSqgd);
                        *(ckt->CKTstate0 + here->VDMOSqth) = (delTemp-delTemp1)*capth +
                                                             *(ckt->CKTstate1 + here->VDMOSqth);
                    } else {
                        /* TRANOP only */
                        *(ckt->CKTstate0 + here->VDMOSqgs) = vgs*capgs;
                        *(ckt->CKTstate0 + here->VDMOSqgd) = vgd*capgd;
                        *(ckt->CKTstate0 + here->VDMOSqth) = delTemp*capth;
                    }
#ifndef PREDICTOR
                }
#endif /*PREDICTOR*/
            }
#ifndef NOBYPASS
bypass:
#endif
            if ((ckt->CKTmode & (MODEINITTRAN)) ||
                    (!(ckt->CKTmode & (MODETRAN)))) {
                /*
                 *  initialize to zero charge conductances
                 *  and current
                 */
                gcgs = 0;
                ceqgs = 0;
                gcgd = 0;
                ceqgd = 0;
                gcTt = 0.0;
                ceqth = 0.0;
            } else {
                if (capgs == 0) *(ckt->CKTstate0 + here->VDMOScqgs) = 0;
                if (capgd == 0) *(ckt->CKTstate0 + here->VDMOScqgd) = 0;
                if (capth == 0) *(ckt->CKTstate0 + here->VDMOScqth) = 0;
                /*
                 *    calculate equivalent conductances and currents for
                 *    vdmos capacitors
                 */
                error = NIintegrate(ckt, &gcgs, &ceqgs, capgs, here->VDMOSqgs);
                if (error) return(error);
                error = NIintegrate(ckt, &gcgd, &ceqgd, capgd, here->VDMOSqgd);
                if (error) return(error);
                ceqgs = ceqgs - gcgs*vgs + ckt->CKTag[0] *
                        *(ckt->CKTstate0 + here->VDMOSqgs);
                ceqgd = ceqgd - gcgd*vgd + ckt->CKTag[0] *
                        *(ckt->CKTstate0 + here->VDMOSqgd);
                if (selfheat)
                {
                    error = NIintegrate(ckt, &gcTt, &ceqth, capth, here->VDMOSqth);
                    if (error) return (error);
                    ceqth = ceqth - gcTt*delTemp + ckt->CKTag[0] *
                            *(ckt->CKTstate0 + here->VDMOSqth);
                }
            }

            /*
             *  load current vector
             */

            if (here->VDMOSmode >= 0) {
                xnrm = 1;
                xrev = 0;
                cdreq = model->VDMOStype*(cdrain - here->VDMOSgds*vds
                                                 - here->VDMOSgm*vgs);
            } else {
                xnrm = 0;
                xrev = 1;
                cdreq = -(model->VDMOStype)*(cdrain - here->VDMOSgds*(-vds) -
                                             here->VDMOSgm*vgd);
            }
            *(ckt->CKTrhs + here->VDMOSgNodePrime) -=
                (model->VDMOStype * (ceqgs + ceqgd));
            *(ckt->CKTrhs + here->VDMOSdNodePrime) +=
                (-cdreq + model->VDMOStype * ceqgd);
            *(ckt->CKTrhs + here->VDMOSsNodePrime) +=
                cdreq + model->VDMOStype * ceqgs;
            if (selfheat) {
                *(ckt->CKTrhs + here->VDMOStempNode) -= here->VDMOScth + ceqth; /* dissipated power + Cth current*/
            }

            if (selfheat) {
                if (here->VDMOSmode >= 0) {
                    GmT = model->VDMOStype * here->VDMOSgmT;
                    gTtg  = here->VDMOSgtempg;
                    gTtdp = here->VDMOSgtempd;
                    gTtt  = here->VDMOSgtempT;
                    gTtsp = - (gTtg + gTtdp);
                } else {
                    GmT = -model->VDMOStype * here->VDMOSgmT;
                    gTtg  = here->VDMOSgtempg;
                    gTtsp = here->VDMOSgtempd;
                    gTtt  = here->VDMOSgtempT;
                    gTtdp = - (gTtg + gTtsp);
                }
            }

            /*
             *  load y matrix
             */
            *(here->VDMOSDdPtr) += (here->VDMOSdrainConductance + here->VDMOSdsConductance);
            *(here->VDMOSGgPtr) += (here->VDMOSgateConductance);
            *(here->VDMOSSsPtr) += (here->VDMOSsourceConductance + here->VDMOSdsConductance);
            *(here->VDMOSDPdpPtr) +=
                (here->VDMOSdrainConductance + here->VDMOSgds +
                 xrev*(here->VDMOSgm) + gcgd);
            *(here->VDMOSSPspPtr) +=
                (here->VDMOSsourceConductance + here->VDMOSgds +
                 xnrm*(here->VDMOSgm) + gcgs);
            *(here->VDMOSGPgpPtr) +=
                (here->VDMOSgateConductance) + (gcgd + gcgs);
            *(here->VDMOSGgpPtr) += (-here->VDMOSgateConductance);
            *(here->VDMOSDdpPtr) += (-here->VDMOSdrainConductance);
            *(here->VDMOSGPgPtr) += (-here->VDMOSgateConductance);
            *(here->VDMOSGPdpPtr) -= gcgd;
            *(here->VDMOSGPspPtr) -= gcgs;
            *(here->VDMOSSspPtr) += (-here->VDMOSsourceConductance);
            *(here->VDMOSDPdPtr) += (-here->VDMOSdrainConductance);
            *(here->VDMOSDPgpPtr) += ((xnrm - xrev)*here->VDMOSgm - gcgd);
            *(here->VDMOSDPspPtr) += (-here->VDMOSgds - xnrm*
                                      (here->VDMOSgm));
            *(here->VDMOSSPgpPtr) += (-(xnrm - xrev)*here->VDMOSgm - gcgs);
            *(here->VDMOSSPsPtr) += (-here->VDMOSsourceConductance);
            *(here->VDMOSSPdpPtr) += (-here->VDMOSgds - xrev*
                                      (here->VDMOSgm));

            *(here->VDMOSDsPtr) += (-here->VDMOSdsConductance);
            *(here->VDMOSSdPtr) += (-here->VDMOSdsConductance);

            if (selfheat)
            {
                (*(here->VDMOSDPtempPtr) += GmT);
                (*(here->VDMOSSPtempPtr) += -GmT);
                (*(here->VDMOSGtempPtr) += 0.0);
                (*(here->VDMOSTemptempPtr) += gTtt + 1/here->VDMOSrth0 + gcTt);
                (*(here->VDMOSTempgPtr) += gTtg);
                (*(here->VDMOSTempdpPtr) += gTtdp);
                (*(here->VDMOSTempspPtr) += gTtsp);
            }

            /* bulk diode model
             * Delivers reverse conduction and forward breakdown
             * of VDMOS transistor
             */

            double vd;      /* current diode voltage */
            double vdtemp;
            double vte;
            double vtebrk, vbrknp;
            double cd, cdb, csat, cdeq;
            double czero;
            double czof2;
            double capd;
            double gd, gdb, gspr;
            double delvd;   /* change in diode voltage temporary */
            double diffcharge, deplcharge, diffcap, deplcap;
            double evd, evrev;
#ifndef NOBYPASS
            double tol;     /* temporary for tolerance calculations */
#endif

            cd = 0.0;
            cdb = 0.0;
            gd = 0.0;
            gdb = 0.0;
            csat = here->VDIOtSatCur;
            gspr = here->VDIOtConductance;
            vte = model->VDMOSn * vt;
            vtebrk = model->VDIObrkdEmissionCoeff * vt;
            vbrknp = here->VDIOtBrkdwnV;

            Check_diode = 1;
            if (ckt->CKTmode & MODEINITSMSIG) {
                vd = *(ckt->CKTstate0 + here->VDIOvoltage);
            } else if (ckt->CKTmode & MODEINITTRAN) {
                vd = *(ckt->CKTstate1 + here->VDIOvoltage);
            } else if ((ckt->CKTmode & MODEINITJCT) &&
                       (ckt->CKTmode & MODETRANOP) && (ckt->CKTmode & MODEUIC)) {
                vd = here->VDIOinitCond;
            } else if (ckt->CKTmode & MODEINITJCT) {
                vd = here->VDIOtVcrit;
            } else {
#ifndef PREDICTOR
                if (ckt->CKTmode & MODEINITPRED) {
                    *(ckt->CKTstate0 + here->VDIOvoltage) =
                        *(ckt->CKTstate1 + here->VDIOvoltage);
                    vd = DEVpred(ckt, here->VDIOvoltage);
                    *(ckt->CKTstate0 + here->VDIOcurrent) =
                        *(ckt->CKTstate1 + here->VDIOcurrent);
                    *(ckt->CKTstate0 + here->VDIOconduct) =
                        *(ckt->CKTstate1 + here->VDIOconduct);
                } else {
#endif /* PREDICTOR */
                    vd = model->VDMOStype * (*(ckt->CKTrhsOld + here->VDIOposPrimeNode) -
                                             *(ckt->CKTrhsOld + here->VDMOSdNode));
#ifndef PREDICTOR
                }
#endif /* PREDICTOR */
                delvd = vd - *(ckt->CKTstate0 + here->VDIOvoltage);
                cdhat = *(ckt->CKTstate0 + here->VDIOcurrent) +
                        *(ckt->CKTstate0 + here->VDIOconduct) * delvd;
                /*
                *   bypass if solution has not changed
                */
#ifndef NOBYPASS
                if ((!(ckt->CKTmode & MODEINITPRED)) && (ckt->CKTbypass)) {
                    tol = ckt->CKTvoltTol + ckt->CKTreltol*
                          MAX(fabs(vd), fabs(*(ckt->CKTstate0 + here->VDIOvoltage)));
                    if (fabs(delvd) < tol) {
                        tol = ckt->CKTreltol* MAX(fabs(cdhat),
                                                  fabs(*(ckt->CKTstate0 + here->VDIOcurrent))) +
                              ckt->CKTabstol;
                        if (fabs(cdhat - *(ckt->CKTstate0 + here->VDIOcurrent))
                                < tol) {
                            vd = *(ckt->CKTstate0 + here->VDIOvoltage);
                            cd = *(ckt->CKTstate0 + here->VDIOcurrent);
                            gd = *(ckt->CKTstate0 + here->VDIOconduct);
                            goto load;
                        }
                    }
                }
#endif /* NOBYPASS */
                /*
                *   limit new junction voltage
                */
                if ((model->VDMOSbvGiven) &&
                        (vd < MIN(0, -vbrknp + 10 * vtebrk))) {
                    vdtemp = -(vd + vbrknp);
                    vdtemp = DEVpnjlim(vdtemp,
                                       -(*(ckt->CKTstate0 + here->VDIOvoltage) +
                                         vbrknp), vtebrk,
                                       here->VDIOtVcrit, &Check_diode);
                    vd = -(vdtemp + vbrknp);
                } else {
                    vd = DEVpnjlim(vd, *(ckt->CKTstate0 + here->VDIOvoltage),
                                   vte, here->VDIOtVcrit, &Check_diode);
                }
            }
            /*
            *   compute dc current and derivatives
            */
            if (vd >= -3 * vte) {                 /* bottom current forward */

                evd = exp(vd / vte);
                cdb = csat*(evd - 1);
                gdb = csat*evd / vte;

            } else if ((!(model->VDMOSbvGiven)) ||
                       vd >= -vbrknp) { /* reverse */

                arg = 3 * vte / (vd*CONSTe);
                arg = arg * arg * arg;
                cdb = -csat*(1 + arg);
                gdb = csat * 3 * arg / vd;

            } else {                          /* breakdown */

                evrev = exp(-(vbrknp + vd) / vtebrk);
                cdb = -csat*evrev;
                gdb = csat*evrev / vtebrk;

            }


            cd = cdb;
            gd = gdb;

            gd = gd + ckt->CKTgmin;
            cd = cd + ckt->CKTgmin*vd;

            if ((ckt->CKTmode & (MODEDCTRANCURVE | MODETRAN | MODEAC | MODEINITSMSIG)) ||
                    ((ckt->CKTmode & MODETRANOP) && (ckt->CKTmode & MODEUIC))) {
                /*
                *   charge storage elements
                */
                czero = here->VDIOtJctCap;
                if (vd < here->VDIOtDepCap) {
                    arg = 1 - vd / here->VDIOtJctPot;
                    sarg = exp(-here->VDIOtGradingCoeff*log(arg));
                    deplcharge = here->VDIOtJctPot*czero*(1 - arg*sarg) / (1 - here->VDIOtGradingCoeff);
                    deplcap = czero*sarg;
                } else {
                    czof2 = czero / here->VDIOtF2;
                    deplcharge = czero*here->VDIOtF1 + czof2*(here->VDIOtF3*(vd - here->VDIOtDepCap) +
                                 (here->VDIOtGradingCoeff / (here->VDIOtJctPot + here->VDIOtJctPot))*(vd*vd - here->VDIOtDepCap*here->VDIOtDepCap));
                    deplcap = czof2*(here->VDIOtF3 + here->VDIOtGradingCoeff*vd / here->VDIOtJctPot);
                }
                diffcharge = here->VDIOtTransitTime*cdb;
                *(ckt->CKTstate0 + here->VDIOcapCharge) =
                    diffcharge +  deplcharge;

                diffcap = here->VDIOtTransitTime*gdb;
                capd = diffcap + deplcap;

                here->VDIOcap = capd;

                /*
                *   store small-signal parameters
                */
                if ((!(ckt->CKTmode & MODETRANOP)) ||
                        (!(ckt->CKTmode & MODEUIC))) {
                    if (ckt->CKTmode & MODEINITSMSIG) {
                        *(ckt->CKTstate0 + here->VDIOcapCurrent) = capd;

                        continue;
                    }

                    /*
                    *   transient analysis
                    */

                    if (ckt->CKTmode & MODEINITTRAN) {
                        *(ckt->CKTstate1 + here->VDIOcapCharge) =
                            *(ckt->CKTstate0 + here->VDIOcapCharge);
                    }
                    error = NIintegrate(ckt, &geq, &ceq, capd, here->VDIOcapCharge);
                    if (error) return(error);
                    gd = gd + geq;
                    cd = cd + *(ckt->CKTstate0 + here->VDIOcapCurrent);
                    if (ckt->CKTmode & MODEINITTRAN) {
                        *(ckt->CKTstate1 + here->VDIOcapCurrent) =
                            *(ckt->CKTstate0 + here->VDIOcapCurrent);
                    }
                }
            }

            /*
            *   check convergence
            */

            if ((Check_mos == 1) || (Check_diode == 1)) {
                ckt->CKTnoncon++;
                ckt->CKTtroubleElt = (GENinstance *)here;
            }

            *(ckt->CKTstate0 + here->VDIOvoltage) = vd;
            *(ckt->CKTstate0 + here->VDIOcurrent) = cd;
            *(ckt->CKTstate0 + here->VDIOconduct) = gd;

#ifndef NOBYPASS
load:
#endif
            /*
            *   load current vector
            */
            cdeq = cd - gd*vd;
            if (model->VDMOStype == 1) {
                *(ckt->CKTrhs + here->VDMOSdNode) += cdeq;
                *(ckt->CKTrhs + here->VDIOposPrimeNode) -= cdeq;
            } else {
                *(ckt->CKTrhs + here->VDMOSdNode) -= cdeq;
                *(ckt->CKTrhs + here->VDIOposPrimeNode) += cdeq;
            }

            /*
            *   load matrix
            */
            *(here->VDMOSSsPtr) += gspr;
            *(here->VDMOSDdPtr) += gd;
            *(here->VDIORPrpPtr) += (gd + gspr);
            *(here->VDIOSrpPtr) -= gspr;
            *(here->VDIODrpPtr) -= gd;
            *(here->VDIORPsPtr) -= gspr;
            *(here->VDIORPdPtr) -= gd;
        }
    }
    return(OK);
}
