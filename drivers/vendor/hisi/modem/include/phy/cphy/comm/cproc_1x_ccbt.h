/** ****************************************************************************

                    Huawei Technologies Sweden AB (C), 2001-2015

 ********************************************************************************
 * @author    Automatically generated by DAISY
 * @version
 * @date      2015-09-15
 * @file
 * @brief
 * @copyright Huawei Technologies Sweden AB
 *******************************************************************************/
#ifndef CPROC_1X_CCBT_H
#define CPROC_1X_CCBT_H

/*******************************************************************************
 1. Other files included
*******************************************************************************/

#include "vos.h"

#ifdef __cplusplus
#if __cplusplus
    extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */
#pragma pack(4)

/*******************************************************************************
 2. Macro definitions
*******************************************************************************/


/*******************************************************************************
 3. Enumerations declarations
*******************************************************************************/

/** ****************************************************************************
 * Name        : CPROC_1X_CCBT_MSG_TYPE_ENUM_UINT16
 *
 * Description :
 *******************************************************************************/
enum CPROC_1X_CCBT_MSG_TYPE_ENUM
{
    ID_CCBT_CPROC_1X_SET_POWER_REQ      = 0x2FF0, /**< @sa CCBT_CPROC_1X_SET_POWER_REQ_STRU */
    ID_CPROC_CCBT_1X_SET_POWER_CNF      = 0x2FF1, /**< @sa CPROC_CCBT_1X_SET_POWER_CNF_STRU */
    ID_CPROC_CSDR_1X_SET_POWER_REQ      = 0x2FF2, /**< @sa CPROC_CSDR_1X_SET_POWER_REQ_STRU */
    ID_CSDR_CPROC_1X_SET_POWER_CNF      = 0x2FF3, /**< @sa CSDR_CPROC_1X_SET_POWER_CNF_STRU */
    ID_CCBT_CPROC_1X_STATUS_REQ         = 0x2FF4, /**< @sa CCBT_CPROC_1X_STATUS_REQ_STRU */
    ID_CPROC_CCBT_1X_STATUS_CNF         = 0xF2F0, /**< @sa CPROC_CCBT_1X_STATUS_CNF_STRU */
    ID_CBT_CPROC_1X_NOTIFY_NOSIG_REQ    = 0xF2F1, /**< @sa CBT_CPROC_1X_NOTIFY_NOSIG_REQ_STRU */
    ID_CPROC_1X_CCBT_MSG_TYPE_ENUM_BUTT = 0xF2F2
};
typedef VOS_UINT16 CPROC_1X_CCBT_MSG_TYPE_ENUM_UINT16;

/** ****************************************************************************
 * Name        : CPROC_1X_STATE_ENUM_UINT16
 *
 * Description : This enum tells CCBT what state CPROC is in.
 *******************************************************************************/
enum CPROC_1X_STATE_ENUM
{
    CPROC_1X_STATE_NW_SEARCH            = 0x0000,
    CPROC_1X_STATE_IDLE_NON_SLOTTED     = 0x0001,
    CPROC_1X_STATE_ACCESS               = 0x0002,
    CPROC_1X_STATE_SLOTTED              = 0x0003,
    CPROC_1X_STATE_TCH                  = 0x0004,
    CPROC_1X_STATE_SLAVE                = 0x0005,
    CPROC_1X_STATE_BUTT                 = 0x0006
};
typedef VOS_UINT16 CPROC_1X_STATE_ENUM_UINT16;

/*******************************************************************************
 4. Message Header declaration
*******************************************************************************/

/*******************************************************************************
 5. Message declaration
*******************************************************************************/

/*******************************************************************************
 6. STRUCT and UNION declaration
*******************************************************************************/

/*******************************************************************************
 7. OTHER declarations
*******************************************************************************/

/** ****************************************************************************
 * Name        : CCBT_CPROC_1X_SET_POWER_REQ_STRU
 *
 * Description :
 *******************************************************************************/
typedef struct
{
    VOS_MSG_HEADER
    CPROC_1X_CCBT_MSG_TYPE_ENUM_UINT16  enMsgId;
    VOS_UINT16                          usOpId;
    VOS_UINT16                          usEnableTpc;    /**< 1 is enable TPC,  0 disable tpc */
    VOS_INT16                           sDedicatePower; /**< Unit is 0.125 dBm */
} CCBT_CPROC_1X_SET_POWER_REQ_STRU;

/** ****************************************************************************
 * Name        : CPROC_CCBT_1X_SET_POWER_CNF_STRU
 *
 * Description :
 *******************************************************************************/
typedef struct
{
    VOS_MSG_HEADER
    CPROC_1X_CCBT_MSG_TYPE_ENUM_UINT16  enMsgId;
    VOS_UINT16                          usOpId;
    VOS_UINT32                          ulResult; /**< 0 is success,  else error code */
} CPROC_CCBT_1X_SET_POWER_CNF_STRU;

/** ****************************************************************************
 * Name        : CPROC_CSDR_1X_SET_POWER_REQ_STRU
 *
 * Description :
 *******************************************************************************/
typedef struct
{
    VOS_MSG_HEADER
    CPROC_1X_CCBT_MSG_TYPE_ENUM_UINT16  enMsgId;
    VOS_UINT16                          usOpId;
    VOS_UINT16                          usEnableTpc;    /**< 1 is enable TPC,  0 disable tpc */
    VOS_INT16                           sDedicatePower; /**< Unit is 0.125 dBm */
} CPROC_CSDR_1X_SET_POWER_REQ_STRU;

/** ****************************************************************************
 * Name        : CSDR_CPROC_1X_SET_POWER_CNF_STRU
 *
 * Description :
 *******************************************************************************/
typedef struct
{
    VOS_MSG_HEADER
    CPROC_1X_CCBT_MSG_TYPE_ENUM_UINT16  enMsgId;
    VOS_UINT16                          usOpId;
    VOS_UINT32                          ulResult; /**< 0 is success,  else error code */
} CSDR_CPROC_1X_SET_POWER_CNF_STRU;

/** ****************************************************************************
 * Name        : CCBT_CPROC_1X_STATUS_REQ_STRU
 *
 * Description :
 *******************************************************************************/
typedef struct
{
    VOS_MSG_HEADER
    CPROC_1X_CCBT_MSG_TYPE_ENUM_UINT16  enMsgId;
    VOS_UINT16                          usOpId;
    VOS_UINT8                           aucData[4]; /**< not used, just for expand in future. */
} CCBT_CPROC_1X_STATUS_REQ_STRU;

/** ****************************************************************************
 * Name        : CPROC_CCBT_1X_STATUS_CNF_STRU
 *
 * Description :
 *******************************************************************************/
typedef struct
{
    VOS_MSG_HEADER
    CPROC_1X_CCBT_MSG_TYPE_ENUM_UINT16  enMsgId;
    VOS_UINT16                          usOpId;
    CPROC_1X_STATE_ENUM_UINT16          enStatus;
    VOS_UINT16                          uhwReserved; /**< 32 bit alignment */
} CPROC_CCBT_1X_STATUS_CNF_STRU;

/** ****************************************************************************
 * Name        : CBT_CPROC_1X_NOTIFY_NOSIG_REQ_STRU
 *
 * Description :
 *******************************************************************************/
typedef struct
{
    VOS_MSG_HEADER
    CPROC_1X_CCBT_MSG_TYPE_ENUM_UINT16  enMsgId;
    VOS_UINT16                          usOpId;
} CBT_CPROC_1X_NOTIFY_NOSIG_REQ_STRU;

/*******************************************************************************
 8. Global  declaration
*******************************************************************************/

/*******************************************************************************
 9. Function declarations
*******************************************************************************/

#if ((VOS_OS_VER == VOS_WIN32) || (VOS_OS_VER == VOS_NUCLEUS))
#pragma pack()
#else
#pragma pack(0)
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */

#endif
