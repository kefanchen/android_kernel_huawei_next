

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "TafMtaCtx.h"
#include "TafMtaComm.h"
#include "TafMtaMain.h"
#include "AtMtaInterface.h"
#include "TafMtaTimerMgmt.h"
#include "TafMtaAgps.h"
#include "TcNasinterface.h"
#include "TafMtaSecure.h"
#include "TafMtaMntn.h"
#include "MtaPhyInterface.h"
#include "TafMtaPhy.h"
#include "NasMtaInterface.h"
#include "TafMtaUnSolicitedReport.h"

#if (FEATURE_ON == FEATURE_PTM)
#include "NasErrorLog.h"
#endif

#include "MtaMtcInterface.h"
#include "TafMtaModemControl.h"

/* Added by w00316404 for eMBMS project, 2015-5-22, begin */
#include "TafMtaMbms.h"
/* Added by w00316404 for eMBMS project, 2015-5-22, end */

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#define    THIS_FILE_ID        PS_FILE_ID_TAF_MTA_MAIN_C

/*****************************************************************************
  2 全局变量声明
*****************************************************************************/
/* MTA模块处理来自AT消息函数对应表*/
const MTA_MSG_PROC_STRU g_astMtaMsgProcAtTab[]=
{
    /* 消息ID */                                /* 消息处理函数 */
    {ID_AT_MTA_CPOS_SET_REQ,                    TAF_MTA_RcvAtCposSetReq},
    {ID_AT_MTA_CGPSCLOCK_SET_REQ,               TAF_MTA_RcvAtCgpsClockSetReq},

    {ID_AT_MTA_APSEC_SET_REQ,                   TAF_MTA_RcvAtApSecSetReq},

    {ID_AT_MTA_SIMLOCKUNLOCK_SET_REQ,           TAF_MTA_RcvAtSimlockUnlockSetReq},

    {ID_AT_MTA_QRY_NMR_REQ,                     TAF_MTA_RcvQryNmrReq},

    {ID_AT_MTA_RESEL_OFFSET_CFG_SET_NTF,        TAF_MTA_RcvReselOffsetCfgSetReq},

    {ID_AT_MTA_WRR_AUTOTEST_QRY_REQ,            TAF_MTA_RcvQryWrrAutotestReq},
    {ID_AT_MTA_WRR_CELLINFO_QRY_REQ,            TAF_MTA_RcvQryWrrCellInfoReq},
    {ID_AT_MTA_WRR_MEANRPT_QRY_REQ,             TAF_MTA_RcvQryWrrMeanrptReq},
    {ID_AT_MTA_WRR_FREQLOCK_SET_REQ,            TAF_MTA_RcvSetWrrFreqlockReq},
    {ID_AT_MTA_WRR_RRC_VERSION_SET_REQ,         TAF_MTA_RcvSetWrrRrcVersionReq},
    {ID_AT_MTA_WRR_CELLSRH_SET_REQ,             TAF_MTA_RcvSetWrrCellsrhReq},
    {ID_AT_MTA_WRR_FREQLOCK_QRY_REQ,            TAF_MTA_RcvQryWrrFreqlockReq},
    {ID_AT_MTA_WRR_RRC_VERSION_QRY_REQ,         TAF_MTA_RcvQryWrrRrcVersionReq},
    {ID_AT_MTA_WRR_CELLSRH_QRY_REQ,             TAF_MTA_RcvQryWrrCellsrhReq},

    {ID_AT_MTA_BODY_SAR_SET_REQ,                TAF_MTA_RcvAtBodySarSetReq},

    {ID_AT_MTA_CURC_SET_NOTIFY,                 TAF_MTA_RcvAtSetCurcNotify},
    {ID_AT_MTA_CURC_QRY_REQ,                    TAF_MTA_RcvAtQryCurcReq},
    {ID_AT_MTA_UNSOLICITED_RPT_SET_REQ,         TAF_MTA_RcvAtSetUnsolicitedRptReq},
    {ID_AT_MTA_UNSOLICITED_RPT_QRY_REQ,         TAF_MTA_RcvAtQryUnsolicitedRptReq},
    {ID_TAFAGENT_MTA_GET_SYSMODE_REQ,           TAF_MTA_RcvTafAgentGetSysModeReq},

    {ID_AT_MTA_IMEI_VERIFY_QRY_REQ,             TAF_MTA_RcvQryImeiVerifyReq},
    {ID_AT_MTA_CGSN_QRY_REQ,                    TAF_MTA_RcvQryCgsnReq},

    {ID_AT_MTA_NCELL_MONITOR_SET_REQ,           TAF_MTA_RcvAtSetNCellMonitorReq},
    {ID_AT_MTA_NCELL_MONITOR_QRY_REQ,           TAF_MTA_RcvAtQryNCellMonitorReq},

    {ID_AT_MTA_REFCLKFREQ_SET_REQ,              TAF_MTA_RcvAtRefclkfreqSetReq},
    {ID_AT_MTA_REFCLKFREQ_QRY_REQ,              TAF_MTA_RcvAtRefclkfreqQryReq},

    {ID_TAFAGENT_MTA_GET_ANT_STATE_REQ,         TAF_MTA_RcvTafAgentGetAntStateReq},

    {ID_AT_MTA_HANDLEDECT_SET_REQ,              TAF_MTA_RcvAtSetHandleDectReq},
    {ID_AT_MTA_HANDLEDECT_QRY_REQ,              TAF_MTA_RcvAtQryHandleDectReq},

#if (FEATURE_ON == FEATURE_DSDS)
    { ID_AT_MTA_RRC_PROTECT_PS_REQ,             TAF_MTA_RcvAtPsProtectSetReq},
#endif
    { ID_AT_MTA_PHY_INIT_REQ,                   TAF_MTA_RcvAtPhyInitReq},


    {ID_AT_MTA_ECID_SET_REQ,                    TAF_MTA_RcvAtEcidSetReq},

    {ID_AT_MTA_MIPICLK_QRY_REQ,                TAF_MTA_RcvAtQryMipiClkReq},

    {ID_AT_MTA_SET_DPDTTEST_FLAG_REQ,           TAF_MTA_RcvAtSetDpdtTestFlagReq},
    {ID_AT_MTA_SET_DPDT_VALUE_REQ,              TAF_MTA_RcvAtSetDpdtValueReq},
    {ID_AT_MTA_QRY_DPDT_VALUE_REQ,              TAF_MTA_RcvAtQryDpdtValueReq},

    {ID_AT_MTA_SET_JAM_DETECT_REQ,              TAF_MTA_RcvAtSetJamDetectReq},

    {ID_AT_MTA_SET_FREQ_LOCK_REQ,               TAF_MTA_RcvAtSetFreqLockReq},

    {ID_AT_MTA_SET_GSM_FREQLOCK_REQ,            TAF_MTA_RcvAtSetGFreqLockReq},

    {ID_AT_MTA_SET_FEMCTRL_REQ,                 TAF_MTA_RcvAtSetFemctrlReq},

    {ID_AT_MTA_NVWRSECCTRL_SET_REQ,             TAF_MTA_RcvAtNvwrSecCtrlSetReq},
    /* Added by w00316404 for eMBMS project, 2015-5-22, begin */
#if(FEATURE_ON == FEATURE_LTE)
#if(FEATURE_ON == FEATURE_LTE_MBMS)
    {ID_AT_MTA_MBMS_SERVICE_OPTION_SET_REQ,     TAF_MTA_RcvAtMBMSServiceOptionSetReq},
    {ID_AT_MTA_MBMS_SERVICE_STATE_SET_REQ,      TAF_MTA_RcvAtMBMSServiceStateSetReq},
    {ID_AT_MTA_MBMS_PREFERENCE_SET_REQ,         TAF_MTA_RcvAtMBMSPreferenceSetReq},
    {ID_AT_MTA_SIB16_NETWORK_TIME_QRY_REQ,      TAF_MTA_RcvAtSIB16NetworkTimeQryReq},
    {ID_AT_MTA_BSSI_SIGNAL_LEVEL_QRY_REQ,       TAF_MTA_RcvAtBSSISignalLevelQryReq},
    {ID_AT_MTA_NETWORK_INFO_QRY_REQ,            TAF_MTA_RcvAtNetworkInfoQryReq},
    {ID_AT_MTA_EMBMS_STATUS_QRY_REQ,            TAF_MTA_RcvAtEMBMSStatusQryReq},
    {ID_AT_MTA_MBMS_UNSOLICITED_CFG_SET_REQ,    TAF_MTA_RcvAtMBMSUnsolicitedCfgSetReq},
    {ID_AT_MTA_INTEREST_LIST_SET_REQ,           TAF_MTA_RcvAtMBMSInterestListSetReq},
    {ID_AT_MTA_MBMS_AVL_SERVICE_LIST_QRY_REQ,   TAF_MTA_RcvAtMBMSAvlServiceListQryReq},
#endif
    {ID_AT_MTA_LTE_LOW_POWER_SET_REQ,           TAF_MTA_RcvAtLteLowPowerConsumptionSetReq},
    {ID_AT_MTA_LTE_WIFI_COEX_SET_REQ,           TAF_MTA_RcvAtIsmCoexSetReq},
    {ID_AT_MTA_LTE_WIFI_COEX_QRY_REQ,           TAF_MTA_RcvAtIsmCoexQryReq},
#endif
    /* Added by w00316404 for eMBMS project, 2015-5-22, end */
    {ID_AT_MTA_SET_FR_REQ,                      TAF_MTA_RcvAtSetFrReq},

#if (FEATURE_ON == FEATURE_UE_MODE_CDMA)
    /* Added by z00316370 for Nv_Esn_Meid 2015-7-24 begin */
    {ID_AT_MTA_MEID_SET_REQ,                    TAF_MTA_RcvMeidSetReq},
    /* Added by z00316370 for Nv_Esn_Meid 2015-7-24 end */
#endif

    /* Added by lwx277467 for Transmission Mode, 2015-07-30, begin */
    {ID_AT_MTA_TRANSMODE_QRY_REQ,               TAF_MTA_RcvAtTransModeQryReq},
    /* Added by lwx277467 for Transmission Mode, 2015-07-30, end */

    /* Added by lwx277467 for UE Mode, 2015-09-07, begin */
    {ID_AT_MTA_UE_CENTER_SET_REQ,               TAF_MTA_RcvAtUECenterSetReq},
    {ID_AT_MTA_UE_CENTER_QRY_REQ,               TAF_MTA_RcvAtUECenterQryReq},
    /* Added by lwx277467 for UE Mode, 2015-09-07, end */

};

/* MTA模块处理来自PHY APM模块的消息函数对应表*/
const MTA_MSG_PROC_STRU g_astMtaMsgProcApmTab[]=
{
    /* 消息ID */                                /* 消息处理函数 */
    {ID_APM_MTA_BODY_SAR_SET_CNF,               TAF_MTA_RcvPhyBodySarSetCnf},

    {ID_APM_MTA_REFCLOCK_STATUS_IND,            TAF_MTA_RcvPhyRefClockStatusInd},

    {ID_APM_MTA_HANDLE_DETECT_SET_CNF,          TAF_MTA_RcvPhyHandleDectSetCnf},
    {ID_APM_MTA_HANDLE_DETECT_QRY_CNF,          TAF_MTA_RcvPhyHandleDectQryCnf},

    /* Added by z00328328 for TX_POWER_CHR, 2015-10-20, begin */
#if(FEATURE_ON == FEATURE_PTM)
    {ID_APM_MTA_GET_TX_POWER_INFO_CNF,          TAF_MTA_RcvPhyTxPwrQryCnf},
#endif
    /* Added by z00328328 for TX_POWER_CHR, 2015-10-20, end */
};

/* Added by zwx247453 for Refclkfreq, 2015-06-17, begin */
/* MTA模块处理来自TLPHY模块的消息函数对应表 */
const MTA_MSG_PROC_STRU g_astMtaMsgProcRttAgentTab[]=
{
    /* 消息ID */                                /* 消息处理函数 */
    {ID_APM_MTA_REFCLOCK_STATUS_IND,       TAF_MTA_RcvTlPhyAfclockStatusInd},
    /* Added by z00328328 for TX_POWER_CHR, 2015-10-20, begin */
#if(FEATURE_ON == FEATURE_PTM)
    {ID_TLPHY_MTA_GET_TX_POWER_INFO_CNF,   TAF_MTA_RcvPhyTxPwrQryCnf},
#endif
    /* Added by z00328328 for TX_POWER_CHR, 2015-10-20, end */
};
/* Added by zwx247453 for Refclkfreq, 2015-6-17, end */

/* MTA模块处理来自接入层G模下消息函数对应表*/
const MTA_MSG_PROC_STRU g_astMtaMsgProcGasTab[]=
{
    /* 消息ID */                                /* 消息处理函数 */
    {ID_RRC_MTA_MSG_POSITION_REQ,               TAF_MTA_RcvRrcMsgPositionReq},

    {ID_RRC_MTA_QRY_NMR_CNF,                    TAF_MTA_RcvGUAsEcellInfoQryCnf},

    {ID_GRR_MTA_NCELL_MONITOR_SET_CNF,          TAF_MTA_RcvRrcSetNCellMonitorCnf},
    {ID_GRR_MTA_NCELL_MONITOR_QRY_CNF,          TAF_MTA_RcvRrcQryNCellMonitorCnf},
    {ID_GRR_MTA_NCELL_MONITOR_IND,              TAF_MTA_RcvRrcNCellMonitorInd},

    {ID_RRC_MTA_JAM_DETECT_CNF,                 TAF_MTA_RcvRrcJamDetectCnf},
    {ID_RRC_MTA_JAM_DETECT_IND,                 TAF_MTA_RcvRrcJamDetectInd},

    {ID_RRC_MTA_CHECK_FREQ_VALIDITY_CNF,        TAF_MTA_RcvRrcCheckFreqValidityCnf},

    {ID_GRR_MTA_FREQLOCK_SET_CNF,               TAF_MTA_RcvGasSetFreqLockCnf},

#if(FEATURE_ON == FEATURE_PTM)
    {ID_RRC_MTA_PLMN_FREQ_QRY_CNF,              TAF_MTA_RcvGasPlmnFreqQryCnf},
#endif

};
/* MTA模块处理来自接入层W模下消息函数对应表*/
const MTA_MSG_PROC_STRU g_astMtaMsgProcWcomTab[]=
{
    /* 消息ID */                                /* 消息处理函数 */
    {ID_RRC_MTA_MSG_POSITION_REQ,               TAF_MTA_RcvRrcMsgPositionReq},

};


/* MTA模块处理来自接入层Wrr模下消息函数对应表*/
const MTA_MSG_PROC_STRU g_astMtaMsgProcWrrTab[]=
{
    {ID_RRC_MTA_QRY_NMR_CNF,                    TAF_MTA_RcvGUAsEcellInfoQryCnf},
    {ID_WRR_MTA_AUTOTEST_QRY_CNF,               TAF_MTA_RcvWrrAutotestQryCnf},
    {ID_WRR_MTA_CELLINFO_QRY_CNF,               TAF_MTA_RcvWrrCellInfoQryCnf},
    {ID_WRR_MTA_MEASRPT_QRY_CNF,                TAF_MTA_RcvWrrMeanrptQryCnf},
    {ID_WRR_MTA_FREQLOCK_SET_CNF,               TAF_MTA_RcvWrrFreqlockSetCnf},
    {ID_WRR_MTA_RRC_VERSION_SET_CNF,            TAF_MTA_RcvWrrVersionSetCnf},
    {ID_WRR_MTA_CELLSRH_SET_CNF,                TAF_MTA_RcvWrrCellsrhSetCnf},
    {ID_WRR_MTA_FREQLOCK_QRY_CNF,               TAF_MTA_RcvWrrFreqlockQryCnf},
    {ID_WRR_MTA_RRC_VERSION_QRY_CNF,            TAF_MTA_RcvWrrVersionQryCnf},
    {ID_WRR_MTA_CELLSRH_QRY_CNF,                TAF_MTA_RcvWrrCellsrhQryCnf},

    {ID_RRC_MTA_CHECK_FREQ_VALIDITY_CNF,        TAF_MTA_RcvRrcCheckFreqValidityCnf},
#if(FEATURE_ON == FEATURE_PTM)
    {ID_RRC_MTA_PLMN_FREQ_QRY_CNF,              TAF_MTA_RcvWrrPlmnFreqQryCnf},
#endif
};


/* MTA模块处理来自物理层G模下消息函数对应表*/
const MTA_MSG_PROC_STRU g_astMtaMsgProcGphyTab[]=
{
    /* 消息ID */                                /* 消息处理函数 */
    {ID_GPHY_MTA_SET_GPS_RF_CLOCK_CNF,          TAF_MTA_RcvPhyCgpsClockSetCnf},
    {ID_GUPHY_MTA_QRY_DPDT_VALUE_CNF,           TAF_MTA_RcvGuPhyQryDpdtValueCnf},
    {ID_GPHY_MTA_XPASS_INFO_IND,                TAF_MTA_RcvGphyXpassInfoInd},

};
/* MTA模块处理来自物理层W模下消息函数对应表*/
const MTA_MSG_PROC_STRU g_astMtaMsgProcWphyTab[]=
{
    /* 消息ID */                                /* 消息处理函数 */
    {ID_WPHY_MTA_SET_GPS_RF_CLOCK_CNF,          TAF_MTA_RcvPhyCgpsClockSetCnf},
    {ID_GUPHY_MTA_QRY_DPDT_VALUE_CNF,           TAF_MTA_RcvGuPhyQryDpdtValueCnf},
    {ID_WPHY_MTA_XPASS_INFO_IND,                TAF_MTA_RcvWphyXpassInfoInd},

};
/* MTA模块处理来自TC消息函数对应表*/
const MTA_MSG_PROC_STRU g_astMtaMsgProcTcTab[]=
{
    /* 消息ID */                                /* 消息处理函数 */
    {TCMTA_RESET_UE_POS_STORED_INFO_IND,        TAF_MTA_RcvTcResetUePosStoredInfoInd},

};
/* MTA模块处理来自定时器消息函数对应表*/
const MTA_MSG_PROC_STRU g_astMtaMsgProcTimerTab[]=
{
    /* 消息ID */                                /* 消息处理函数 */
    {TI_TAF_MTA_WAIT_AT_SET_CPOS_CNF,           TAF_MTA_RcvTiCposSetReqExpired},
    {TI_TAF_MTA_WAIT_GPHY_SET_CGPSCLOCK_CNF,    TAF_MTA_RcvTiCgpsClockSetReqExpired},
    {TI_TAF_MTA_WAIT_WPHY_SET_CGPSCLOCK_CNF,    TAF_MTA_RcvTiCgpsClockSetReqExpired},

    {TI_TAF_MTA_WAIT_AS_QRY_NMR_CNF,            TAF_MTA_RcvTiWaitAsQryNmrCnfExpired},

    {TI_TAF_MTA_WAIT_WRR_AUTOTEST_QRY_CNF,      TAF_MTA_RcvTiReqAutotestQryExpired},
    {TI_TAF_MTA_WAIT_WRR_CELLINFO_QRY_CNF,      TAF_MTA_RcvTiReqCellInfoQryExpired},
    {TI_TAF_MTA_WAIT_WRR_MEANRPT_QRY_CNF,       TAF_MTA_RcvTiReqMeanrptQryExpired},
    {TI_TAF_MTA_WAIT_WRR_FREQLOCK_SET_CNF,      TAF_MTA_RcvTiReqFreqlockSetExpired},
    {TI_TAF_MTA_WAIT_WRR_RRC_VERSION_SET_CNF,   TAF_MTA_RcvTiReqRrcVersionSetExpired},
    {TI_TAF_MTA_WAIT_WRR_CELLSRH_SET_CNF,       TAF_MTA_RcvTiReqCellsrhSetExpired},
    {TI_TAF_MTA_WAIT_WRR_FREQLOCK_QRY_CNF,      TAF_MTA_RcvTiReqFreqlockQryExpired},
    {TI_TAF_MTA_WAIT_WRR_RRC_VERSION_QRY_CNF,   TAF_MTA_RcvTiReqRrcVersionQryExpired},
    {TI_TAF_MTA_WAIT_WRR_CELLSRH_QRY_CNF,       TAF_MTA_RcvTiReqCellsrhQryExpired},

    {TI_TAF_MTA_WAIT_PHY_SET_BODYSAR_CNF,       TAF_MTA_RcvTiWaitPhySetBodySarExpired},

    {TI_TAF_MTA_WAIT_GRR_NCELLMONITOR_SET_CNF,  TAF_MTA_RcvTiWaitGrrSetNCellMonitorExpired},
    {TI_TAF_MTA_WAIT_GRR_NCELLMONITOR_QRY_CNF,  TAF_MTA_RcvTiWaitGrrQryNCellMonitorExpired},

    {TI_TAF_MTA_WAIT_PHY_HANDLEDECT_SET_CNF,    TAF_MTA_RcvTiWaitPhySetHandleDectExpired},
    {TI_TAF_MTA_WAIT_PHY_HANDLEDECT_QRY_CNF,    TAF_MTA_RcvTiWaitPhyQryHandleDectExpired},

    {TI_TAF_MTA_WAIT_AS_ECID_SET_CNF,           TAF_MTA_RcvTiEcidSetExpired},


    {TI_TAF_MTA_RF_LCD_MIPI_CLK_REPORT,         TAF_MTA_RcvTiMipiClkExpired},

    {TI_TAF_MTA_WAIT_DPDT_VALUE_QRY_CNF,        TAF_MTA_RcvTiReqDpdtValueQryExpired},

    {TI_TAF_MTA_WAIT_DSP_INIT_CNF,              TAF_MTA_RcvTiPhyInitExpired},

    {TI_TAF_MTA_WAIT_JAM_DETECT_SET_CNF,        TAF_MTA_RcvTiWaitGrrSetJamDetectExpired},

    {TI_TAF_MTA_WAIT_CHECK_FREQ_VALIDITY_CNF,   TAF_MTA_RcvTiWaitRrcCheckFreqValidityExpired},

    {TI_TAF_MTA_WAIT_SET_GSM_FREQLOCK_CNF,      TAF_MTA_RcvTiWaitGasSetGFreqLockExpired},

    /* Added by w00316404 for eMBMS project, 2015-5-22, begin */
#if(FEATURE_ON == FEATURE_LTE)
#if(FEATURE_ON == FEATURE_LTE_MBMS)
    {TI_TAF_MTA_WAIT_LRRC_MBMS_SERVICE_OPTION_SET_CNF,          TAF_MTA_RcvTiWaitLrrcSetMBMSServiceOption},
    {TI_TAF_MTA_WAIT_LRRC_MBMS_SERVICE_STATE_SET_CNF,           TAF_MTA_RcvTiWaitLrrcSetMBMSServiceState},
    {TI_TAF_MTA_WAIT_LRRC_MBMS_PREFERENCE_SET_CNF,              TAF_MTA_RcvTiWaitLrrcSetMBMSPreference},
    {TI_TAF_MTA_WAIT_LRRC_MBMS_SIB16_NETWORK_TIME_QRY_CNF,      TAF_MTA_RcvTiWaitLrrcQrySIB16NetworkTime},
    {TI_TAF_MTA_WAIT_LRRC_MBMS_BSSI_SIGNAL_LEVEL_QRY_CNF,       TAF_MTA_RcvTiWaitLrrcQryBSSISignalLevel},
    {TI_TAF_MTA_WAIT_LRRC_MBMS_NETWORK_INFO_QRY_CNF,            TAF_MTA_RcvTiWaitLrrcQryNetworkInfo},
    {TI_TAF_MTA_WAIT_LRRC_EMBMS_STATUS_QRY_CNF,                 TAF_MTA_RcvTiWaitLrrcQryEMBMSStatus},
    {TI_TAF_MTA_WAIT_LRRC_MBMS_UNSOLICITED_CFG_SET_CNF,         TAF_MTA_RcvTiWaitLrrcSetMBMSUnsolicitedCfg},
    {TI_TAF_MTA_WAIT_LRRC_MBMS_INTERESTLIST_SET_CNF,            TAF_MTA_RcvTiWaitLrrcSetInterestList},
    {TI_TAF_MTA_WAIT_LRRC_MBMS_AVL_SERVICE_LIST_QRY_CNF,        TAF_MTA_RcvTiWaitLrrcQryAvlServiceList},
#endif
    {TI_TAF_MTA_WAIT_LRRC_LOW_POWER_CONSUMPTION_SET_CNF,        TAF_MTA_RcvTiWaitLrrcSetLowPowerConsumption},
    {TI_TAF_MTA_WAIT_LTE_WIFI_COEX_SET_CNF,                     TAF_MTA_RcvTiWaitSetIsmCoexList},
#endif
    /* Added by w00316404 for eMBMS project, 2015-5-22, end */

    {TI_TAF_MTA_WAIT_SET_FR_CNF,                TAF_MTA_RcvTiWaitLrrcSetFrCnfExpired},

    /* Added by zwx247453 for Refclkfreq, 2015-06-17, begin */
    {TI_TAF_MTA_WAIT_REFCLOCK_STATUS_IND,       TAF_MTA_RcvTiWaitAgentRefclockIndExpired},
    {TI_TAF_MTA_WAIT_QRY_AFCLOCK_STATUS_CNF,    TAF_MTA_RcvTiWaitAgentQryAfclockExpired},
    /* Added by zwx247453 for Refclkfreq, 2015-06-17, end */
#if (FEATURE_ON == FEATURE_PTM)
    {TI_TAF_MTA_WAIT_FREQ_CLT_QRY_CNF,                    TAF_MTA_RcvTiWaitFreqCltQryCnfExpired},
    /* Added by z00328328 for TX_POWER_CHR, 2015-10-23, begin */
    {TI_TAF_MTA_WAIT_TX_PWR_CLT_QRY_CNF,        TAF_MTA_RcvTiWaitTxPwrCltQryCnfExpired},
    /* Added by z00328328 for TX_POWER_CHR, 2015-10-23, end */
#endif
    /* Added by lwx277467 for Transmission Mode, 2015-08-04, Begin */
    {TI_TAF_MTA_WAIT_TRANSMODE_QRY_CNF,         TAF_MTA_RcvTiWaitLrrcTransModeQryExpired},
    /* Added by lwx277467 for Transmission Mode, 2015-08-04, End */

    /* Added by lwx277467 for UE Mode, 2015-09-07, Begin */
    {TI_TAF_MTA_WAIT_UE_CENTER_SET_CNF,         TAF_MTA_RcvTiWaitLnasUECenterSetExpired},
    {TI_TAF_MTA_WAIT_UE_CENTER_QRY_CNF,         TAF_MTA_RcvTiWaitLnasUECenterQryExpired},
    /* Added by lwx277467 for UE Mode, 2015-09-07, End */
};

/* MTA模块处理来自MMA模块的消息函数对应表*/
const MTA_MSG_PROC_STRU g_astMtaMsgProcNasTab[]=
{
    /* 消息ID */                                /* 消息处理函数 */
    {ID_NAS_MTA_AREA_LOST_IND,                  TAF_MTA_RcvNasAreaLostInd},

    /* Added by lwx277467 for UE Mode, 2015-09-07, begin */
    { ID_LNAS_MTA_SET_UE_CENTER_CNF,            TAF_MTA_RcvLnasUECenterSetCnf},
    { ID_LNAS_MTA_QRY_UE_CENTER_CNF,            TAF_MTA_RcvLnasUECenterQryCnf},
    /* Added by lwx277467 for UE Mode, 2015-09-07, end */
};

/* MTA模块处理来自AT消息函数对应表*/
const MTA_MSG_PROC_STRU g_astMtaMsgProcTafAgentTab[]=
{
    {ID_TAFAGENT_MTA_GET_SYSMODE_REQ,           TAF_MTA_RcvTafAgentGetSysModeReq},

    {ID_TAFAGENT_MTA_GET_ANT_STATE_REQ,         TAF_MTA_RcvTafAgentGetAntStateReq},
};


#if (FEATURE_ON == FEATURE_PTM)
/* MTA模块处理来自Acpu OM模块的消息函数对应表*/
const MTA_MSG_PROC_STRU g_astMtaMsgProcAcpuOmTab[]=
{
    /* 消息ID */                                /* 消息处理函数 */
    {ID_OM_ERR_LOG_CTRL_IND,                    TAF_MTA_RcvAcpuOmErrLogCtrlInd},
    {ID_OM_ERR_LOG_REPORT_REQ,                  TAF_MTA_RcvAcpuOmErrLogRptReq},
    {ID_OM_INFO_CLT_REPORT_REQ,                 TAF_MTA_RcvAcpuOmInfoCltRptReq},
};
#endif

/* MTA模块处理来自MTC模块的消息函数对应表*/
const MTA_MSG_PROC_STRU g_astMtaMsgProcMtcTab[]=
{
    /* 消息ID */                                /* 消息处理函数 */
#if (FEATURE_ON == FEATURE_MULTI_MODEM)
    {ID_MTC_MTA_PS_TRANSFER_IND,                TAF_MTA_RcvMtcPsTransferInd},
#endif
    {ID_MTC_MTA_MIPICLK_INFO_IND,               TAF_MTA_RcvMtcMipiClkInfoInd},
    {ID_MTC_MTA_GSM_CELLINFO_QRY_CNF,           TAF_MTA_RcvMtcGsmCellInfoQryCnf},
};

#if (FEATURE_ON == FEATURE_LTE)
const MTA_MSG_PROC_STRU g_astMtaMsgProcLrrcTab[]=
{
    /* 消息ID */                                /* 消息处理函数 */
    {ID_LRRC_MTA_CELLINFO_QRY_CNF,              TAF_MTA_RcvLrrcCellinfoQryCnf},

    {ID_TLRRC_MTA_QRY_DPDT_VALUE_CNF,           TAF_MTA_RcvTLrrcDpdtValueQryCnf},

    {ID_RRC_MTA_CHECK_FREQ_VALIDITY_CNF,        TAF_MTA_RcvRrcCheckFreqValidityCnf},

    /* Added by w00316404 for eMBMS project, 2015-5-22, begin */
#if(FEATURE_ON == FEATURE_LTE)
#if(FEATURE_ON == FEATURE_LTE_MBMS)
    {ID_LRRC_MTA_MBMS_SERVICE_OPTION_CNF,       TAF_MTA_RcvLrrcMBMSServiceOptionSetCnf},
    {ID_LRRC_MTA_MBMS_SERVICE_STATE_SET_CNF,    TAF_MTA_RcvLrrcMBMSServiceStateSetCnf},
    {ID_LRRC_MTA_MBMS_PREFERENCE_SET_CNF,       TAF_MTA_RcvLrrcMBMSPreferenceSetCnf},
    {ID_LRRC_MTA_SIB16_NETWORK_TIME_QRY_CNF,    TAF_MTA_RcvLrrcMBMSSIB16NetworkTimeQryCnf},
    {ID_LRRC_MTA_BSSI_SIGNAL_LEVEL_QRY_CNF,     TAF_MTA_RcvLrrcMBMSBSSISignalLevelQryCnf},
    {ID_LRRC_MTA_NETWORK_INFO_QRY_CNF,          TAF_MTA_RcvLrrcMBMSNetworkInfoQryCnf},
    {ID_LRRC_MTA_EMBMS_STATUS_QRY_CNF,          TAF_MTA_RcvLrrcEMBMSStatusQryCnf},
    {ID_LRRC_MTA_MBMS_UNSOLICITED_CFG_SET_CNF,  TAF_MTA_RcvLrrcMBMSUnsolicitedCfgSetCnf},
    {ID_LRRC_MTA_MBMS_SERVICE_EVENT_IND,        TAF_MTA_RcvLrrcMBMSServiceEventInd},
    {ID_LRRC_MTA_INTEREST_LIST_SET_CNF,         TAF_MTA_RcvLrrcInterestListSetCnf},
    {ID_LRRC_MTA_MBMS_AVL_SERVICE_LIST_QRY_CNF, TAF_MTA_RcvLrrcMBMSAvlServiceListQryCnf},
#endif
    {ID_LRRC_MTA_LOW_POWER_CONSUMPTION_SET_CNF, TAF_MTA_RcvLrrcLowPowerConsumptionSetCnf},
    {ID_LRRC_MTA_LTE_WIFI_COEX_SET_CNF,         TAF_MTA_RcvLrrcIsmCoexSetCnf},
#endif
    /* Added by w00316404 for eMBMS project, 2015-5-22, end */
    {ID_LRRC_MTA_SET_FR_CNF,                    TAF_MTA_RcvLrrcSetFrCnf},
#if(FEATURE_ON == FEATURE_PTM)
    {ID_RRC_MTA_PLMN_FREQ_QRY_CNF,              TAF_MTA_RcvLrrcPlmnFreqQryCnf},
#endif

    /* Added by lwx277467 for Transmission Mode, 2015-07-30, begin */
    { ID_LRRC_MTA_TRANSMODE_QRY_CNF,            TAF_MTA_RcvLrrcTransModeQryCnf},
    /* Added by lwx277467 for Transmission Mode, 2015-07-30, end */
};
#endif

/* MTA模块处理来自物理层初始化消息函数对应表*/
const MTA_MSG_PROC_STRU g_astMtaMsgProcDspStartTab[]=
{
    /* 消息ID */                                /* 消息处理函数 */
    {ID_UPHY_MTA_START_UPHY_CNF,          TAF_MTA_RcvPhyInitCnf},

};

#if (FEATURE_ON == FEATURE_UE_MODE_TDS)
const MTA_MSG_PROC_STRU g_astMtaMsgProcTdsTab[]=
{
    {ID_RRC_MTA_CHECK_FREQ_VALIDITY_CNF,        TAF_MTA_RcvRrcCheckFreqValidityCnf},
	{ID_TLRRC_MTA_QRY_DPDT_VALUE_CNF,           TAF_MTA_RcvTLrrcDpdtValueQryCnf},
};
#endif

/* Added by w00316404 for eMBMS project, 2015-5-22, begin */
#if 0
/* MTA模块处理来自物理层TLPHY代理层消息函数对应表*/
const MTA_MSG_PROC_STRU g_astMtaMsgProcTLphyAgentTab[]=
{
    /* 消息ID */                                /* 消息处理函数 */
#if(FEATURE_ON == FEATURE_LTE)
    {ID_LPHY_MTA_LTE_WIFI_COEX_SET_CNF,         TAF_MTA_RcvLphyIsmCoexSetCnf},
#endif
};
#endif
/* Added by w00316404 for eMBMS project, 2015-5-22, end */

/*****************************************************************************
  3 函数申明
*****************************************************************************/


VOS_VOID TAF_MTA_SndAtMsg(
    AT_APPCTRL_STRU                    *pAppCtrl,
    VOS_UINT32                          ulMsgId,
    VOS_UINT32                          ulLen,
    VOS_UINT8                          *pData
)
{
    AT_MTA_MSG_STRU                    *pstMtaMsgStr;

    /* 判断输入参数 */
    if ((VOS_NULL_PTR == pData) || (0 == ulLen))
    {
        MTA_ERROR_LOG("TAF_MTA_SndAtMsg: para is invalid!");
        return;
    }

    /* 消息空间申请 */
    pstMtaMsgStr = (AT_MTA_MSG_STRU *)PS_ALLOC_MSG( UEPS_PID_MTA,
                                                    sizeof(AT_APPCTRL_STRU) + sizeof(ulMsgId) + ulLen);
    if (VOS_NULL_PTR == pstMtaMsgStr)
    {
        MTA_ERROR_LOG("TAF_MTA_SndAtMsg: alloc msg fail!");
        return;
    }

    /* 填消息包 */
    pstMtaMsgStr->ulReceiverCpuId       = VOS_LOCAL_CPUID;
    pstMtaMsgStr->ulSenderCpuId         = VOS_LOCAL_CPUID;
    pstMtaMsgStr->ulSenderPid           = UEPS_PID_MTA;
    pstMtaMsgStr->ulReceiverPid         = WUEPS_PID_AT;
    pstMtaMsgStr->ulMsgId               = ulMsgId;
    pstMtaMsgStr->stAppCtrl.usClientId  = MN_GetRealClientId(pAppCtrl->usClientId, UEPS_PID_MTA);
    pstMtaMsgStr->stAppCtrl.ucOpId      = pAppCtrl->ucOpId;
    pstMtaMsgStr->stAppCtrl.aucReserved[0] = 0;
    PS_MEM_CPY(pstMtaMsgStr->aucContent, pData, ulLen);

    /* 发送消息到AT模块 */
    if (VOS_OK != PS_SEND_MSG(UEPS_PID_MTA, pstMtaMsgStr))
    {
        MTA_ERROR_LOG("TAF_MTA_SndAtMsg():WARNING:SEND MSG FIAL");
    }
    return;
}


VOS_UINT32 TAF_MTA_SearchMsgProcTab(
    VOS_UINT32                          ulMsgCnt,
    VOS_VOID                           *pstMsg,
    const MTA_MSG_PROC_STRU            *pstMsgProcTab
)
{
    VOS_UINT32                          ulLoop;
    VOS_UINT32                          ulMsgName;

    /* 从消息包中获取MSG ID */
    ulMsgName  = ((MSG_HEADER_STRU*)pstMsg)->ulMsgName;

    /* 查表，进行消息分发 */
    for (ulLoop = 0; ulLoop < ulMsgCnt; ulLoop++)
    {
        if (pstMsgProcTab[ulLoop].ulMsgType == ulMsgName)
        {
            pstMsgProcTab[ulLoop].pProcMsgFunc(pstMsg);
            break;
        }
    }

    /* 没有找到匹配的消息 */
    if (ulMsgCnt == ulLoop)
    {
        return VOS_ERR;
    }

    return VOS_OK;
}
VOS_VOID TAF_MTA_RcvAtMsg(struct MsgCB *pstMsg)
{
    VOS_UINT32                          ulMsgCnt;
    VOS_UINT32                          ulRst;

    /* 从g_astMtaMsgProcAtTab中获取消息个数 */
    ulMsgCnt = sizeof(g_astMtaMsgProcAtTab)/sizeof(MTA_MSG_PROC_STRU);

    /*g_astMtaMsgProcAtTab查表，进行消息分发*/
    ulRst = TAF_MTA_SearchMsgProcTab(ulMsgCnt, pstMsg, g_astMtaMsgProcAtTab);

    /* 没有找到匹配的消息 */
    if (VOS_ERR == ulRst)
    {
        MTA_ERROR_LOG("TAF_MTA_RcvAtMsg: Msg Id is invalid!");
    }

    return;
}
VOS_VOID TAF_MTA_RcvApmMsg(struct MsgCB *pstMsg)
{
    VOS_UINT32                          ulMsgCnt;
    VOS_UINT32                          ulRst;

    /* 从g_astMtaMsgProcApmTab中获取消息个数 */
    ulMsgCnt = sizeof(g_astMtaMsgProcApmTab)/sizeof(MTA_MSG_PROC_STRU);

    /* g_astMtaMsgProcApmTab查表，进行消息分发 */
    ulRst = TAF_MTA_SearchMsgProcTab(ulMsgCnt, pstMsg, g_astMtaMsgProcApmTab);

    /* 没有找到匹配的消息 */
    if (VOS_ERR == ulRst)
    {
        MTA_ERROR_LOG("TAF_MTA_RcvApmMsg: Msg Id is invalid!");
    }

    return;
}

/* Added by zwx247453 for Refclkfreq, 2015-06-17, begin */
/*****************************************************************************
 函 数 名  : TAF_MTA_RcvRttAgentMsg
 功能描述  : 处理来自RTTAGENT模块的消息
 输入参数  : struct MsgCB *pstMsg
 输出参数  : 无
 返 回 值  : VOS_VOID
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年06月17日
    作    者   : zwx247453
    修改内容   : Refclkfreq项目新增函数

*****************************************************************************/
VOS_VOID TAF_MTA_RcvRttAgentMsg(struct MsgCB *pstMsg)
{
    VOS_UINT32                          ulMsgCnt;
    VOS_UINT32                          ulRst;

    /* 从g_astMtaMsgProcRttAgentTab中获取消息个数 */
    ulMsgCnt = sizeof(g_astMtaMsgProcRttAgentTab)/sizeof(MTA_MSG_PROC_STRU);

    /* g_astMtaMsgProcRttAgentTab查表，进行消息分发 */
    ulRst = TAF_MTA_SearchMsgProcTab(ulMsgCnt, pstMsg, g_astMtaMsgProcRttAgentTab);

    /* 没有找到匹配的消息 */
    if (VOS_ERR == ulRst)
    {
        MTA_ERROR_LOG("TAF_MTA_RcvRttAgentMsg: Msg Id is invalid!");
    }

    return;
}
/* Added by zwx247453 for Refclkfreq, 2015-06-17, end */


VOS_VOID TAF_MTA_RcvGasMsg(struct MsgCB *pstMsg)
{
    VOS_UINT32                          ulMsgCnt;
    VOS_UINT32                          ulRst;

    /* 从g_astMtaMsgProcGasTab中获取消息个数 */
    ulMsgCnt = sizeof(g_astMtaMsgProcGasTab)/sizeof(MTA_MSG_PROC_STRU);

    /* g_astMtaMsgProcGasTab查表，进行消息分发 */
    ulRst = TAF_MTA_SearchMsgProcTab(ulMsgCnt, pstMsg, g_astMtaMsgProcGasTab);

    /* 没有找到匹配的消息 */
    if (VOS_ERR == ulRst)
    {
        MTA_ERROR_LOG("TAF_MTA_RcvGasMsg: Msg Id is invalid!");
    }

    return;
}
VOS_VOID TAF_MTA_RcvWcomMsg(struct MsgCB *pstMsg)

{
    VOS_UINT32                          ulMsgCnt;
    VOS_UINT32                          ulRst;

    /* 从g_astMtaMsgProcWcomTab中获取消息个数 */
    ulMsgCnt = sizeof(g_astMtaMsgProcWcomTab)/sizeof(MTA_MSG_PROC_STRU);

    /* g_astMtaMsgProcWasTab查表，进行消息分发 */
    ulRst = TAF_MTA_SearchMsgProcTab(ulMsgCnt, pstMsg, g_astMtaMsgProcWcomTab);

    /* 没有找到匹配的消息 */
    if (VOS_ERR == ulRst)
    {
        MTA_ERROR_LOG("TAF_MTA_RcvWcomMsg: Msg Id is invalid!");
    }

    return;
}
VOS_VOID TAF_MTA_RcvWrrMsg(struct MsgCB *pstMsg)
{
    VOS_UINT32                          ulMsgCnt;
    VOS_UINT32                          ulRst;

    /* 从g_astMtaMsgProcWrrTab中获取消息个数 */
    ulMsgCnt = sizeof(g_astMtaMsgProcWrrTab)/sizeof(MTA_MSG_PROC_STRU);

    /* g_astMtaMsgProcWrrTab查表，进行消息分发 */
    ulRst = TAF_MTA_SearchMsgProcTab(ulMsgCnt, pstMsg, g_astMtaMsgProcWrrTab);

    /* 没有找到匹配的消息 */
    if (VOS_ERR == ulRst)
    {
        MTA_ERROR_LOG("TAF_MTA_RcvWrrMsg: Msg Id is invalid!");
    }

    return;
}
VOS_VOID TAF_MTA_RcvGphyMsg(struct MsgCB *pstMsg)
{
    VOS_UINT32                          ulMsgCnt;
    VOS_UINT32                          ulLoop;
    VOS_UINT32                          ulMsgName;

    /* 从g_astMtaMsgProcGphyTab中获取消息个数 */
    ulMsgCnt = sizeof(g_astMtaMsgProcGphyTab)/sizeof(MTA_MSG_PROC_STRU);

    /* 从消息包中获取MSG ID */
    ulMsgName = (VOS_UINT32)((MTA_PHY_MSG_HEADER_STRU*)pstMsg)->usMsgID;

    /* g_astMtaMsgProcGphyTab查表，进行消息分发 */
    for (ulLoop = 0; ulLoop < ulMsgCnt; ulLoop++)
    {
        if (g_astMtaMsgProcGphyTab[ulLoop].ulMsgType == ulMsgName)
        {
            g_astMtaMsgProcGphyTab[ulLoop].pProcMsgFunc(pstMsg);
            break;
        }
    }

    /* 没有找到匹配的消息 */
    if (ulMsgCnt == ulLoop)
    {
        MTA_ERROR_LOG("TAF_MTA_RcvGphyMsg: Msg Id is invalid!");
    }

    return;
}



VOS_VOID TAF_MTA_RcvWphyMsg(struct MsgCB *pstMsg)
{
    VOS_UINT32                          ulMsgCnt;
    VOS_UINT32                          ulLoop;
    VOS_UINT32                          ulMsgName;

    /* 从g_astMtaMsgProcWphyTab中获取消息个数 */
    ulMsgCnt = sizeof(g_astMtaMsgProcWphyTab)/sizeof(MTA_MSG_PROC_STRU);

    /* 从消息包中获取MSG ID */
    ulMsgName = (VOS_UINT32)((MTA_PHY_MSG_HEADER_STRU*)pstMsg)->usMsgID;

    /* g_astMtaMsgProcWphyTab查表，进行消息分发 */
    for (ulLoop = 0; ulLoop < ulMsgCnt; ulLoop++)
    {
        if (g_astMtaMsgProcWphyTab[ulLoop].ulMsgType == ulMsgName)
        {
            g_astMtaMsgProcWphyTab[ulLoop].pProcMsgFunc(pstMsg);
            break;
        }
    }

    /* 没有找到匹配的消息 */
    if (ulMsgCnt == ulLoop)

    {
        MTA_ERROR_LOG("TAF_MTA_RcvWphyMsg: Msg Id is invalid!");
    }

    return;
}


VOS_VOID TAF_MTA_RcvTcMsg(struct MsgCB *pstMsg)
{
    VOS_UINT32                          ulMsgCnt;
    VOS_UINT32                          ulRst;

    /* 从g_astMtaMsgProcTcTab中获取消息个数 */
    ulMsgCnt = sizeof(g_astMtaMsgProcTcTab)/sizeof(MTA_MSG_PROC_STRU);

    /* g_astMtaMsgProcTcTab查表，进行消息分发 */
    ulRst = TAF_MTA_SearchMsgProcTab(ulMsgCnt, pstMsg, g_astMtaMsgProcTcTab);

    /* 没有找到匹配的消息 */
    if (VOS_ERR == ulRst)
    {
        MTA_ERROR_LOG("TAF_MTA_RcvTcMsg: Msg Id is invalid!");
    }

    return;
}
VOS_VOID TAF_MTA_RcvTimerMsg(VOS_VOID *pstMsg)
{
    VOS_UINT32                          ulLoop;
    VOS_UINT32                          ulMsgCnt;
    VOS_UINT32                          ulTimerId;
    REL_TIMER_MSG                      *pstRcvTimerMsg;

    /* 从g_astMtaMsgProcTimerTab中获取消息个数 */
    ulMsgCnt = sizeof(g_astMtaMsgProcTimerTab)/sizeof(MTA_MSG_PROC_STRU);

    /* 获取TIMER ID，把定时器状态置为STOP */
    pstRcvTimerMsg = (REL_TIMER_MSG *)pstMsg;
    ulTimerId      = pstRcvTimerMsg->ulName;
    TAF_MTA_StopTimer(ulTimerId);

    /* g_astMtaMsgProcTimerTab查表，进行消息分发 */
    for (ulLoop = 0; ulLoop < ulMsgCnt; ulLoop++)
    {
        if (g_astMtaMsgProcTimerTab[ulLoop].ulMsgType == ulTimerId)
        {
            g_astMtaMsgProcTimerTab[ulLoop].pProcMsgFunc((VOS_VOID *)pstMsg);
            return;
        }
    }

    /* 没有找到匹配的消息 */
    if (ulMsgCnt == ulLoop)
    {
        MTA_ERROR_LOG("TAF_MTA_RcvTimerMsg: Msg Id is invalid!");
    }

    return;
}


VOS_VOID TAF_MTA_RcvNasMsg(struct MsgCB *pstMsg)
{
    VOS_UINT32                          ulMsgCnt;
    VOS_UINT32                          ulRst;

    /* 从g_astMtaMsgProcMmaTab中获取消息个数 */
    ulMsgCnt = sizeof(g_astMtaMsgProcNasTab)/sizeof(MTA_MSG_PROC_STRU);

    /*g_astMtaMsgProcMmaTab查表，进行消息分发*/
    ulRst = TAF_MTA_SearchMsgProcTab(ulMsgCnt, pstMsg, g_astMtaMsgProcNasTab);

    /* 没有找到匹配的消息 */
    if (VOS_ERR == ulRst)
    {
        MTA_ERROR_LOG("TAF_MTA_RcvNasMsg: Msg Id is invalid!");
    }

    return;
}
VOS_VOID TAF_MTA_RcvTafAgentMsg(struct MsgCB *pstMsg)
{
    VOS_UINT32                          ulMsgCnt;
    VOS_UINT32                          ulRst;

    /* 从g_astMtaMsgProcTafAgentTab中获取消息个数 */
    ulMsgCnt = sizeof(g_astMtaMsgProcTafAgentTab)/sizeof(MTA_MSG_PROC_STRU);

    /*g_astMtaMsgProcTafAgentTab查表，进行消息分发*/
    ulRst = TAF_MTA_SearchMsgProcTab(ulMsgCnt, pstMsg, g_astMtaMsgProcTafAgentTab);

    /* 没有找到匹配的消息 */
    if (VOS_ERR == ulRst)
    {
        MTA_ERROR_LOG("TAF_MTA_RcvTafAgentMsg: Msg Id is invalid!");
    }

    return;
}



#if (FEATURE_ON == FEATURE_PTM)
VOS_VOID TAF_MTA_RcvAcpuOmMsg(struct MsgCB *pstMsg)
{
    VOS_UINT32                          ulMsgCnt;
    VOS_UINT32                          ulRst;

    /* 从g_astMtaMsgProcAcpuOmTab中获取消息个数 */
    ulMsgCnt = sizeof(g_astMtaMsgProcAcpuOmTab)/sizeof(MTA_MSG_PROC_STRU);

    /*g_astMtaMsgProcAcpuOmTab查表，进行消息分发*/
    ulRst = TAF_MTA_SearchMsgProcTab(ulMsgCnt, pstMsg, g_astMtaMsgProcAcpuOmTab);

    /* 没有找到匹配的消息 */
    if (VOS_ERR == ulRst)
    {
        MTA_ERROR_LOG("TAF_MTA_RcvAcpuOmMsg: Msg Id is invalid!");
    }

    return;
}
#endif

#if (FEATURE_ON == FEATURE_MULTI_MODEM)
VOS_VOID TAF_MTA_RcvMtcMsg(struct MsgCB *pstMsg)
{
    VOS_UINT32                          ulMsgCnt;
    VOS_UINT32                          ulRst;

    /* 从g_astMtaMsgProcMtcTab中获取消息个数 */
    ulMsgCnt = sizeof(g_astMtaMsgProcMtcTab)/sizeof(MTA_MSG_PROC_STRU);

    /*g_astMtaMsgProcMtcTab查表，进行消息分发*/
    ulRst = TAF_MTA_SearchMsgProcTab(ulMsgCnt, pstMsg, g_astMtaMsgProcMtcTab);

    /* 没有找到匹配的消息 */
    if (VOS_ERR == ulRst)
    {
        MTA_ERROR_LOG("TAF_MTA_RcvMtcMsg: Msg Id is invalid!");
    }

    return;
}
#endif

#if (FEATURE_ON == FEATURE_LTE)
VOS_VOID TAF_MTA_RcvLrrcMsg(struct MsgCB *pstMsg)
{
    VOS_UINT32                          ulMsgCnt;
    VOS_UINT32                          ulRst;

    /* 从g_astMtaMsgProcLrrcTab中获取消息个数 */
    ulMsgCnt = sizeof(g_astMtaMsgProcLrrcTab)/sizeof(MTA_MSG_PROC_STRU);

    /*g_astMtaMsgProcLrrcTab查表，进行消息分发*/
    ulRst = TAF_MTA_SearchMsgProcTab(ulMsgCnt, pstMsg, g_astMtaMsgProcLrrcTab);

    /* 没有找到匹配的消息 */
    if (VOS_ERR == ulRst)
    {
        MTA_ERROR_LOG("TAF_MTA_RcvLrrcMsg: Msg Id is invalid!");
    }

    return;
}
#endif
VOS_VOID TAF_MTA_RcvDspStartMsg(struct MsgCB *pstMsg)
{
    VOS_UINT32                          ulMsgCnt;
    VOS_UINT32                          ulRst;
    MSG_HEADER_STRU                    *pstMsgHeader    = VOS_NULL_PTR;

    pstMsgHeader = (MSG_HEADER_STRU*)pstMsg;

    pstMsgHeader->ulMsgName = pstMsgHeader->ulMsgName & 0xffff;

    /* 从g_astMtaMsgProcDspStartTab中获取消息个数 */
    ulMsgCnt = sizeof(g_astMtaMsgProcDspStartTab)/sizeof(MTA_MSG_PROC_STRU);

    /*g_astMtaMsgProcDspStartTab查表，进行消息分发*/
    ulRst = TAF_MTA_SearchMsgProcTab(ulMsgCnt, pstMsg, g_astMtaMsgProcDspStartTab);

    /* 没有找到匹配的消息 */
    if (VOS_ERR == ulRst)
    {
        MTA_ERROR_LOG("TAF_MTA_RcvDspStartMsg: Msg Id is invalid!");
    }

    return;
}


#if (FEATURE_ON == FEATURE_UE_MODE_TDS)
VOS_VOID TAF_MTA_RcvTdsMsg(struct MsgCB *pstMsg)
{
    VOS_UINT32                          ulMsgCnt;
    VOS_UINT32                          ulRst;

    /* 从g_astMtaMsgProcTdsTab中获取消息个数 */
    ulMsgCnt = sizeof(g_astMtaMsgProcTdsTab)/sizeof(MTA_MSG_PROC_STRU);

    /* g_astMtaMsgProcTdsTab查表，进行消息分发 */
    ulRst = TAF_MTA_SearchMsgProcTab(ulMsgCnt, pstMsg, g_astMtaMsgProcTdsTab);

    /* 没有找到匹配的消息 */
    if (VOS_ERR == ulRst)
    {
        MTA_ERROR_LOG("TAF_MTA_RcvTdsMsg: Msg Id is invalid!");
    }

    return;
}
#endif

/* Added by w00316404 for eMBMS project, 2015-5-22, begin */
#if 0
/*****************************************************************************
 函 数 名  : TAF_MTA_RcvTLphyAgentMsg
 功能描述  : 处理来自TLPHY代理模块的消息
 输入参数  : struct MsgCB *pstMsg
 输出参数  : 无
 返 回 值  : VOS_VOID
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2015年5月22日
    作    者   : w00316404
    修改内容   : 新增
*****************************************************************************/
VOS_VOID TAF_MTA_RcvTLphyAgentMsg(struct MsgCB *pstMsg)
{
    VOS_UINT32                          ulMsgCnt;
    VOS_UINT32                          ulRst;

    /* 从g_astMtaMsgProcTLphyAgentTab中获取消息个数 */
    ulMsgCnt = sizeof(g_astMtaMsgProcTLphyAgentTab)/sizeof(MTA_MSG_PROC_STRU);

    /*g_astMtaMsgProcTLphyAgentTab查表，进行消息分发*/
    ulRst = TAF_MTA_SearchMsgProcTab(ulMsgCnt, pstMsg, g_astMtaMsgProcTLphyAgentTab);

    /* 没有找到匹配的消息 */
    if (VOS_ERR == ulRst)
    {
        MTA_ERROR_LOG("TAF_MTA_RcvLphyMsg: Msg Id is invalid!");
    }

    return;
}
#endif
/* Added by w00316404 for eMBMS project, 2015-5-22, end */


VOS_VOID  TAF_MTA_ProcMsg (struct MsgCB *pstMsg)
{
    /* 消息为空，直接返回 */
    if (VOS_NULL_PTR == pstMsg)
    {
        MN_ERR_LOG("TAF_MTA_ProcMsg: PstMsg is NULL!");
        return;
    }

    /* 根据发送方PID来分发消息 */
    switch (pstMsg->ulSenderPid)
    {
        case WUEPS_PID_AT:
            TAF_MTA_RcvAtMsg(pstMsg);
            break;
        case WUEPS_PID_WCOM:
            TAF_MTA_RcvWcomMsg(pstMsg);
            break;
        case UEPS_PID_GAS:
            TAF_MTA_RcvGasMsg(pstMsg);
            break;
        case DSP_PID_GPHY:
            TAF_MTA_RcvGphyMsg(pstMsg);
            break;
        case DSP_PID_WPHY:
            TAF_MTA_RcvWphyMsg(pstMsg);
            break;
        case WUEPS_PID_TC:
            TAF_MTA_RcvTcMsg(pstMsg);
            break;
        case VOS_PID_TIMER:
            TAF_MTA_RcvTimerMsg(pstMsg);
            break;

        case WUEPS_PID_WRR:
            TAF_MTA_RcvWrrMsg(pstMsg);
            break;

        case DSP_PID_APM:
            TAF_MTA_RcvApmMsg(pstMsg);
            break;

        case WUEPS_PID_MMA:
            TAF_MTA_RcvNasMsg(pstMsg);
            break;
        case ACPU_PID_TAFAGENT:
            TAF_MTA_RcvTafAgentMsg(pstMsg);
            break;
#if (FEATURE_ON == FEATURE_PTM)
        case MSP_PID_DIAG_APP_AGENT:
            TAF_MTA_RcvAcpuOmMsg(pstMsg);
            break;
#endif
#if (FEATURE_ON == FEATURE_MULTI_MODEM)
        case UEPS_PID_MTC:
            TAF_MTA_RcvMtcMsg(pstMsg);
            break;
#endif
#if (FEATURE_ON == FEATURE_LTE)
        case PS_PID_ERRC:
            TAF_MTA_RcvLrrcMsg(pstMsg);
            break;
#endif

        case DSP_PID_STARTUP:
            TAF_MTA_RcvDspStartMsg(pstMsg);
            break;

#if (FEATURE_ON == FEATURE_UE_MODE_TDS)
        case TPS_PID_RRC:
            TAF_MTA_RcvTdsMsg(pstMsg);
            break;
#endif

        /* Added by w00316404 for eMBMS project, 2015-5-22, begin */
#if 0
        case TLPHY_PID_RTTAGENT:
            TAF_MTA_RcvTLphyAgentMsg(pstMsg);
            break;
#endif
        /* Added by w00316404 for eMBMS project, 2015-5-22, end */

        /* Added by zwx247453 for Refclkfreq, 2015-06-17, begin */
        case TLPHY_PID_RTTAGENT:
            TAF_MTA_RcvRttAgentMsg(pstMsg);
            break;
        /* Added by zwx247453 for Refclkfreq, 2015-06-17, end */

        /* Added by lwx277467 for 新增+CEUS命令, 2015-09-10, begin */
#if (FEATURE_ON == FEATURE_LTE)
        case PS_PID_MM:
            TAF_MTA_RcvNasMsg(pstMsg);
            break;
#endif
        /* Added by lwx277467 for 新增+CEUS命令, 2015-09-10, end */
		
        default:
            MTA_ERROR_LOG("TAF_MTA_ProcMsg: No opposite MsgPro Function!");
            break;
    }

    return;
}


VOS_VOID  TAF_MTA_InitCtx(TAF_MTA_CONTEXT_STRU *pstMtaCtx)
{
    /* 初始化化记录XML码流资源的结构 */
    pstMtaCtx->stAgpsCtx.stXmlText.pcBufHead   = VOS_NULL_PTR;
    pstMtaCtx->stAgpsCtx.stXmlText.pcBufCur    = VOS_NULL_PTR;

    /* 初始化化所有定时器消息 */
    TAF_MTA_InitAllTimers(pstMtaCtx->astTimerCtx);

    /* 初始化AT命令缓存队列 */
    TAF_MTA_InitCmdBufferQueue(TAF_MTA_GetCmdBufferQueueAddr());

    /* 初始化GPS参考时钟状态信息 */
    TAF_MTA_InitRefClockInfo(&pstMtaCtx->stAgpsCtx.stRefClockInfo);

    /* 初始化RF&Lcd相关上下文件 */
    TAF_MTA_InitRfLcdIntrusionCtx(&pstMtaCtx->stMtaRfLcdCtx);

    TAF_MTA_InitEcidCfg(&pstMtaCtx->stAgpsCtx.stEcidCfg);

    /* Added by w00316404 for eMBMS project, 2015-5-22, begin */
#if(FEATURE_ON == FEATURE_LTE)
    TAF_MTA_InitIsmCoex(pstMtaCtx->astIsmCoex);
#endif
    /* Added by w00316404 for eMBMS project, 2015-5-22, end */

#if (FEATURE_ON == FEATURE_PTM)
    TAF_MMA_InitInfoCltCtx(&pstMtaCtx->stInfoCltCtx);
#endif

    return;
}
VOS_UINT32  TAF_MTA_InitTask( VOS_VOID )
{
    TAF_MTA_CONTEXT_STRU                *pstMtaCtx;

    /* 初始化MTA模块CTX */
    pstMtaCtx   = TAF_MTA_GetMtaCtxAddr();
    TAF_MTA_InitCtx(pstMtaCtx);

    return VOS_OK;
}
VOS_UINT32 TAF_MTA_InitPid(enum VOS_INIT_PHASE_DEFINE ip)
{
    switch ( ip )
    {
        case VOS_IP_LOAD_CONFIG:
            {
                /* 调用MTA模块初始化函数 */
                (VOS_VOID)TAF_MTA_InitTask();
            }break;
        default:
            break;
    }

    return VOS_OK;
}









#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
