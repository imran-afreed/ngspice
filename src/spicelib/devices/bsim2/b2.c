/**********
Copyright 1990 Regents of the University of California.  All rights reserved.
Author: 1988 Min-Chie Jeng, Hong J. Park, Thomas L. Quarles
**********/

#include "ngspice/ngspice.h"
#include "ngspice/devdefs.h"
#include "bsim2def.h"
#include "ngspice/suffix.h"

IFparm B2pTable[] = { /* parameters */
 IOP( "l",   BSIM2_L,      IF_REAL   , "Length"),
 IOP( "w",   BSIM2_W,      IF_REAL   , "Width"),
 IOP( "m",   BSIM2_M,      IF_REAL   , "Parallel Multiplier"),
 IOP( "ad",  BSIM2_AD,     IF_REAL   , "Drain area"),
 IOP( "as",  BSIM2_AS,     IF_REAL   , "Source area"),
 IOP( "pd",  BSIM2_PD,     IF_REAL   , "Drain perimeter"),
 IOP( "ps",  BSIM2_PS,     IF_REAL   , "Source perimeter"),
 IOP( "nrd", BSIM2_NRD,    IF_REAL   , "Number of squares in drain"),
 IOP( "nrs", BSIM2_NRS,    IF_REAL   , "Number of squares in source"),
 IOP( "off", BSIM2_OFF,    IF_FLAG   , "Device is initially off"),
 IOP( "vds", BSIM2_IC_VDS, IF_REAL   , "Initial D-S voltage"),
 IOP( "vgs", BSIM2_IC_VGS, IF_REAL   , "Initial G-S voltage"),
 IOP( "vbs", BSIM2_IC_VBS, IF_REAL   , "Initial B-S voltage"),
 IP( "ic",  BSIM2_IC,     IF_VECTOR , "Vector of DS,GS,BS initial voltages")
};

IFparm B2mPTable[] = { /* model parameters */
 IOP( "vfb",   BSIM2_MOD_VFB0,      IF_REAL,"Flat band voltage"),
 IOP( "lvfb",  BSIM2_MOD_VFBL,      IF_REAL, "Length dependence of vfb"),
 IOP( "wvfb",  BSIM2_MOD_VFBW,      IF_REAL, "Width dependence of vfb"),
 IOP( "phi",   BSIM2_MOD_PHI0,      IF_REAL,
        "Strong inversion surface potential "),
 IOP( "lphi",  BSIM2_MOD_PHIL,      IF_REAL, "Length dependence of phi"),
 IOP( "wphi",  BSIM2_MOD_PHIW,      IF_REAL, "Width dependence of phi"),
 IOP( "k1",    BSIM2_MOD_K10,       IF_REAL, "Bulk effect coefficient 1"),
 IOP( "lk1",   BSIM2_MOD_K1L,       IF_REAL, "Length dependence of k1"),
 IOP( "wk1",   BSIM2_MOD_K1W,       IF_REAL, "Width dependence of k1"),
 IOP( "k2",    BSIM2_MOD_K20,       IF_REAL, "Bulk effect coefficient 2"),
 IOP( "lk2",   BSIM2_MOD_K2L,       IF_REAL, "Length dependence of k2"),
 IOP( "wk2",   BSIM2_MOD_K2W,       IF_REAL, "Width dependence of k2"),
 IOP( "eta0",   BSIM2_MOD_ETA00,      IF_REAL,
        "VDS dependence of threshold voltage at VDD=0"),
 IOP( "leta0",  BSIM2_MOD_ETA0L,      IF_REAL, "Length dependence of eta0"),
 IOP( "weta0",  BSIM2_MOD_ETA0W,      IF_REAL, "Width dependence of eta0"),
 IOP( "etab",   BSIM2_MOD_ETAB0,     IF_REAL, "VBS dependence of eta"),
 IOP( "letab",  BSIM2_MOD_ETABL,     IF_REAL, "Length dependence of etab"),
 IOP( "wetab",  BSIM2_MOD_ETABW,     IF_REAL, "Width dependence of etab"),
 IOP( "dl",    BSIM2_MOD_DELTAL,    IF_REAL, "Channel length reduction in um"),
 IOP( "dw",    BSIM2_MOD_DELTAW,    IF_REAL, "Channel width reduction in um"),
 IOP( "mu0",   BSIM2_MOD_MOB00,   IF_REAL,
        "Low-field mobility, at VDS=0 VGS=VTH"),
 IOP( "mu0b",  BSIM2_MOD_MOB0B0, IF_REAL,
        "VBS dependence of low-field mobility"),
 IOP( "lmu0b", BSIM2_MOD_MOB0BL, IF_REAL, "Length dependence of mu0b"),
 IOP( "wmu0b", BSIM2_MOD_MOB0BW, IF_REAL, "Width dependence of mu0b"),
 IOP( "mus0",   BSIM2_MOD_MOBS00,   IF_REAL, "Mobility at VDS=VDD VGS=VTH"),
 IOP( "lmus0",  BSIM2_MOD_MOBS0L,   IF_REAL, "Length dependence of mus0"),
 IOP( "wmus0",  BSIM2_MOD_MOBS0W,   IF_REAL, "Width dependence of mus"),
 IOP( "musb",  BSIM2_MOD_MOBSB0,  IF_REAL, "VBS dependence of mus"),
 IOP( "lmusb", BSIM2_MOD_MOBSBL,  IF_REAL, "Length dependence of musb"),
 IOP( "wmusb", BSIM2_MOD_MOBSBW,  IF_REAL, "Width dependence of musb"),
 IOP( "mu20",  BSIM2_MOD_MOB200,  IF_REAL,
        "VDS dependence of mu in tanh term"),
 IOP( "lmu20", BSIM2_MOD_MOB20L,  IF_REAL, "Length dependence of mu20"),
 IOP( "wmu20", BSIM2_MOD_MOB20W,  IF_REAL, "Width dependence of mu20"),
 IOP( "mu2b",  BSIM2_MOD_MOB2B0,  IF_REAL, "VBS dependence of mu2"),
 IOP( "lmu2b", BSIM2_MOD_MOB2BL,  IF_REAL, "Length dependence of mu2b"),
 IOP( "wmu2b", BSIM2_MOD_MOB2BW,  IF_REAL, "Width dependence of mu2b"),
 IOP( "mu2g",  BSIM2_MOD_MOB2G0,  IF_REAL, "VGS dependence of mu2"),
 IOP( "lmu2g", BSIM2_MOD_MOB2GL,  IF_REAL, "Length dependence of mu2g"),
 IOP( "wmu2g", BSIM2_MOD_MOB2GW,  IF_REAL, "Width dependence of mu2g"),
 IOP( "mu30",  BSIM2_MOD_MOB300,  IF_REAL,
        "VDS dependence of mu in linear term"),
 IOP( "lmu30", BSIM2_MOD_MOB30L,  IF_REAL, "Length dependence of mu30"),
 IOP( "wmu30", BSIM2_MOD_MOB30W,  IF_REAL, "Width dependence of mu30"),
 IOP( "mu3b",  BSIM2_MOD_MOB3B0,  IF_REAL, "VBS dependence of mu3"),
 IOP( "lmu3b", BSIM2_MOD_MOB3BL,  IF_REAL, "Length dependence of mu3b"),
 IOP( "wmu3b", BSIM2_MOD_MOB3BW,  IF_REAL, "Width dependence of mu3b"),
 IOP( "mu3g",  BSIM2_MOD_MOB3G0,  IF_REAL, "VGS dependence of mu3"),
 IOP( "lmu3g", BSIM2_MOD_MOB3GL,  IF_REAL, "Length dependence of mu3g"),
 IOP( "wmu3g", BSIM2_MOD_MOB3GW,  IF_REAL, "Width dependence of mu3g"),
 IOP( "mu40",  BSIM2_MOD_MOB400,  IF_REAL,
        "VDS dependence of mu in linear term"),
 IOP( "lmu40", BSIM2_MOD_MOB40L,  IF_REAL, "Length dependence of mu40"),
 IOP( "wmu40", BSIM2_MOD_MOB40W,  IF_REAL, "Width dependence of mu40"),
 IOP( "mu4b",  BSIM2_MOD_MOB4B0,  IF_REAL, "VBS dependence of mu4"),
 IOP( "lmu4b", BSIM2_MOD_MOB4BL,  IF_REAL, "Length dependence of mu4b"),
 IOP( "wmu4b", BSIM2_MOD_MOB4BW,  IF_REAL, "Width dependence of mu4b"),
 IOP( "mu4g",  BSIM2_MOD_MOB4G0,  IF_REAL, "VGS dependence of mu4"),
 IOP( "lmu4g", BSIM2_MOD_MOB4GL,  IF_REAL, "Length dependence of mu4g"),
 IOP( "wmu4g", BSIM2_MOD_MOB4GW,  IF_REAL, "Width dependence of mu4g"),
 IOP( "ua0",    BSIM2_MOD_UA00,      IF_REAL,
        "Linear VGS dependence of mobility"),
 IOP( "lua0",   BSIM2_MOD_UA0L,      IF_REAL, "Length dependence of ua0"),
 IOP( "wua0",   BSIM2_MOD_UA0W,      IF_REAL, "Width dependence of ua0"),
 IOP( "uab",  BSIM2_MOD_UAB0,     IF_REAL, "VBS dependence of ua"),
 IOP( "luab", BSIM2_MOD_UABL,     IF_REAL, "Length dependence of uab"),
 IOP( "wuab", BSIM2_MOD_UABW,     IF_REAL, "Width dependence of uab"),
 IOP( "ub0",    BSIM2_MOD_UB00,      IF_REAL,
        "Quadratic VGS dependence of mobility"),
 IOP( "lub0",   BSIM2_MOD_UB0L,      IF_REAL, "Length dependence of ub0"),
 IOP( "wub0",   BSIM2_MOD_UB0W,      IF_REAL, "Width dependence of ub0"),
 IOP( "ubb",  BSIM2_MOD_UBB0,     IF_REAL, "VBS dependence of ub"),
 IOP( "lubb", BSIM2_MOD_UBBL,     IF_REAL, "Length dependence of ubb"),
 IOP( "wubb", BSIM2_MOD_UBBW,     IF_REAL, "Width dependence of ubb"),
 IOP( "u10",    BSIM2_MOD_U100,      IF_REAL, "VDS depence of mobility"),
 IOP( "lu10",   BSIM2_MOD_U10L,      IF_REAL, "Length dependence of u10"),
 IOP( "wu10",   BSIM2_MOD_U10W,      IF_REAL, "Width dependence of u10"),
 IOP( "u1b",  BSIM2_MOD_U1B0,     IF_REAL, "VBS depence of u1"),
 IOP( "lu1b", BSIM2_MOD_U1BL,     IF_REAL, "Length depence of u1b"),
 IOP( "wu1b", BSIM2_MOD_U1BW,     IF_REAL, "Width depence of u1b"),
 IOP( "u1d",  BSIM2_MOD_U1D0,     IF_REAL, "VDS depence of u1"),
 IOP( "lu1d", BSIM2_MOD_U1DL,     IF_REAL, "Length depence of u1d"),
 IOP( "wu1d", BSIM2_MOD_U1DW,     IF_REAL, "Width depence of u1d"),
 IOP( "n0",    BSIM2_MOD_N00,       IF_REAL,
        "Subthreshold slope at VDS=0 VBS=0"),
 IOP( "ln0",   BSIM2_MOD_N0L,       IF_REAL, "Length dependence of n0"),
 IOP( "wn0",   BSIM2_MOD_N0W,       IF_REAL, "Width dependence of n0"),
 IOP( "nb",    BSIM2_MOD_NB0,       IF_REAL, "VBS dependence of n"),
 IOP( "lnb",   BSIM2_MOD_NBL,       IF_REAL, "Length dependence of nb"),
 IOP( "wnb",   BSIM2_MOD_NBW,       IF_REAL, "Width dependence of nb"),
 IOP( "nd",    BSIM2_MOD_ND0,       IF_REAL, "VDS dependence of n"),
 IOP( "lnd",   BSIM2_MOD_NDL,       IF_REAL, "Length dependence of nd"),
 IOP( "wnd",   BSIM2_MOD_NDW,       IF_REAL, "Width dependence of nd"),
 IOP( "vof0",    BSIM2_MOD_VOF00,       IF_REAL,
        "Threshold voltage offset AT VDS=0 VBS=0"),
 IOP( "lvof0",   BSIM2_MOD_VOF0L,       IF_REAL, "Length dependence of vof0"),
 IOP( "wvof0",   BSIM2_MOD_VOF0W,       IF_REAL, "Width dependence of vof0"),
 IOP( "vofb",    BSIM2_MOD_VOFB0,       IF_REAL, "VBS dependence of vof"),
 IOP( "lvofb",   BSIM2_MOD_VOFBL,       IF_REAL, "Length dependence of vofb"),
 IOP( "wvofb",   BSIM2_MOD_VOFBW,       IF_REAL, "Width dependence of vofb"),
 IOP( "vofd",    BSIM2_MOD_VOFD0,       IF_REAL, "VDS dependence of vof"),
 IOP( "lvofd",   BSIM2_MOD_VOFDL,       IF_REAL, "Length dependence of vofd"),
 IOP( "wvofd",   BSIM2_MOD_VOFDW,       IF_REAL, "Width dependence of vofd"),
 IOP( "ai0",    BSIM2_MOD_AI00,       IF_REAL,
        "Pre-factor of hot-electron effect."),
 IOP( "lai0",   BSIM2_MOD_AI0L,       IF_REAL, "Length dependence of ai0"),
 IOP( "wai0",   BSIM2_MOD_AI0W,       IF_REAL, "Width dependence of ai0"),
 IOP( "aib",    BSIM2_MOD_AIB0,       IF_REAL, "VBS dependence of ai"),
 IOP( "laib",   BSIM2_MOD_AIBL,       IF_REAL, "Length dependence of aib"),
 IOP( "waib",   BSIM2_MOD_AIBW,       IF_REAL, "Width dependence of aib"),
 IOP( "bi0",    BSIM2_MOD_BI00,       IF_REAL,
        "Exponential factor of hot-electron effect."),
 IOP( "lbi0",   BSIM2_MOD_BI0L,       IF_REAL, "Length dependence of bi0"),
 IOP( "wbi0",   BSIM2_MOD_BI0W,       IF_REAL, "Width dependence of bi0"),
 IOP( "bib",    BSIM2_MOD_BIB0,       IF_REAL, "VBS dependence of bi"),
 IOP( "lbib",   BSIM2_MOD_BIBL,       IF_REAL, "Length dependence of bib"),
 IOP( "wbib",   BSIM2_MOD_BIBW,       IF_REAL, "Width dependence of bib"),
 IOP( "vghigh",    BSIM2_MOD_VGHIGH0,       IF_REAL,
        "Upper bound of the cubic spline function."),
 IOP( "lvghigh",   BSIM2_MOD_VGHIGHL,       IF_REAL,
        "Length dependence of vghigh"),
 IOP( "wvghigh",   BSIM2_MOD_VGHIGHW,       IF_REAL,
        "Width dependence of vghigh"),
 IOP( "vglow",    BSIM2_MOD_VGLOW0,       IF_REAL,
        "Lower bound of the cubic spline function."),
 IOP( "lvglow",   BSIM2_MOD_VGLOWL,       IF_REAL,
        "Length dependence of vglow"),
 IOP( "wvglow",   BSIM2_MOD_VGLOWW,       IF_REAL,
        "Width dependence of vglow"),
 IOP( "tox",   BSIM2_MOD_TOX,       IF_REAL, "Gate oxide thickness in um"),
 IOP( "temp",  BSIM2_MOD_TEMP,      IF_REAL, "Temperature in degree Celcius"),
 IOP( "vdd",   BSIM2_MOD_VDD,       IF_REAL, "Maximum Vds "),
 IOP( "vgg",   BSIM2_MOD_VGG,       IF_REAL, "Maximum Vgs "),
 IOP( "vbb",   BSIM2_MOD_VBB,       IF_REAL, "Maximum Vbs "),
 IOPA( "cgso",  BSIM2_MOD_CGSO,      IF_REAL,
          "Gate source overlap capacitance per unit channel width(m)"),
 IOPA( "cgdo",  BSIM2_MOD_CGDO,      IF_REAL,
          "Gate drain overlap capacitance per unit channel width(m)"),
 IOPA( "cgbo",  BSIM2_MOD_CGBO,      IF_REAL,
          "Gate bulk overlap capacitance per unit channel length(m)"),
 IOP( "xpart", BSIM2_MOD_XPART,     IF_REAL,
      "Flag for channel charge partitioning"),
 IOP( "rsh",   BSIM2_MOD_RSH,       IF_REAL,
      "Source drain diffusion sheet resistance in ohm per square"),
 IOP( "js",    BSIM2_MOD_JS,        IF_REAL,
      "Source drain junction saturation current per unit area"),
 IOP( "pb",    BSIM2_MOD_PB,        IF_REAL,
      "Source drain junction built in potential"),
 IOPA( "mj",    BSIM2_MOD_MJ,        IF_REAL,
       "Source drain bottom junction capacitance grading coefficient"),
 IOPA( "pbsw",  BSIM2_MOD_PBSW,      IF_REAL,
       "Source drain side junction capacitance built in potential"),
 IOPA( "mjsw",  BSIM2_MOD_MJSW,      IF_REAL,
       "Source drain side junction capacitance grading coefficient"),
 IOPA( "cj",    BSIM2_MOD_CJ,        IF_REAL,
       "Source drain bottom junction capacitance per unit area"),
 IOPA( "cjsw",  BSIM2_MOD_CJSW,      IF_REAL,
       "Source drain side junction capacitance per unit area"),
 IOP( "wdf",   BSIM2_MOD_DEFWIDTH,  IF_REAL,
       "Default width of source drain diffusion in um"),
 IOP( "dell",  BSIM2_MOD_DELLENGTH, IF_REAL,
       "Length reduction of source drain diffusion"),
 IOP("kf",     BSIM2_MOD_KF,    IF_REAL ,"Flicker noise coefficient"),
 IOP("af",     BSIM2_MOD_AF,    IF_REAL ,"Flicker noise exponent"),
 IP( "nmos",  BSIM2_MOD_NMOS,      IF_FLAG,
       "Flag to indicate NMOS"),
 IP( "pmos",  BSIM2_MOD_PMOS,      IF_FLAG,
       "Flag to indicate PMOS"),
};

char *B2names[] = {
    "Drain",
    "Gate",
    "Source",
    "Bulk"
};

int	B2nSize = NUMELEMS(B2names);
int	B2pTSize = NUMELEMS(B2pTable);
int	B2mPTSize = NUMELEMS(B2mPTable);
int	B2iSize = sizeof(B2instance);
int	B2mSize = sizeof(B2model);
