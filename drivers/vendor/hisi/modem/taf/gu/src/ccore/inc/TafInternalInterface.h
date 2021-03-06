

#ifndef __TAF_INTERNAL_INTERFACE_H__
#define __TAF_INTERNAL_INTERFACE_H__

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "vos.h"
#include "TafMntn.h"
#include "TafTypeDef.h"
#include "MnCallApi.h"
#include "MnMsgApi.h"


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#pragma pack(4)

/*****************************************************************************
  2 宏定义
*****************************************************************************/


/*****************************************************************************
  3 枚举定义
*****************************************************************************/
/*****************************************************************************
 枚举名    : TAF_INTERNAL_MSG_TYPE_ENUM
 结构说明  : TAF PID内部各个业务模块之间的消息接口枚举
*****************************************************************************/
enum TAF_INTERNAL_MSG_TYPE_ENUM
{
    TAF_CALL_TAF_REL_CALL_REQ                               = ID_TAF_CALL_INTERNAL_BASE, /* _H2ASN_MsgChoice TAF_CALL_TAF_REL_CALL_REQ_STRU */
    TAF_CALL_TAF_REL_CALL_CNF                               ,                            /* _H2ASN_MsgChoice TAF_CALL_TAF_REL_CALL_CNF_STRU */

    /* MSG<->SPM */
    ID_TAF_SPM_SMMA_IND                                     = ID_TAF_MSG_INTERNAL_BASE,       /* _H2ASN_MsgChoice TAF_SPM_SMMA_IND_STRU */
    ID_TAF_SPM_MSG_REPORT_IND                               ,                                 /* _H2ASN_MsgChoice TAF_SPM_MSG_REPORT_IND_STRU */
    ID_TAF_SPM_SMMA_RSP                                     ,                                 /* _H2ASN_MsgChoice TAF_SPM_SMMA_RSP_STRU */

    ID_TAF_SPM_MSG_CHECK_RESULT_IND,                                            /* _H2ASN_MsgChoice TAF_SPM_MSG_CHECK_RESULT_IND_STRU */

    ID_TAF_INTERNAL_MSG_TYPE_BUTT
};
typedef VOS_UINT32 TAF_INTERNAL_MSG_TYPE_ENUM_UINT32;

/*****************************************************************************
 枚举名    : TAF_CALL_REL_CALL_TYPE_ENUM_UINT8
 结构说明  :
             0: TAF_CALL_REL_CALL_ALL_NORMAL    释放所有正常呼叫
*****************************************************************************/
enum TAF_CALL_REL_CALL_TYPE_ENUM
{
    TAF_CALL_REL_CALL_ALL_NORMAL,

    TAF_CALL_REL_CALL_BUTT
};
typedef VOS_UINT8 TAF_CALL_REL_CALL_TYPE_ENUM_UINT8;

/*****************************************************************************
 枚举名    : TAF_SMS_MO_TYPE_ENUM_UINT8
 结构说明  :
             0: TAF_SMS_MO_TYPE_MO          AT发起的短信
             1: TAF_SMS_MO_TYPE_SMMA        SMMA
             2: TAF_SMS_MO_TYPE_BUFFER_STK  STK发起的短信
*****************************************************************************/
enum TAF_SMS_MO_TYPE_ENUM
{
    TAF_SMS_MO_TYPE_MO,
    TAF_SMS_MO_TYPE_SMMA,
    TAF_SMS_MO_TYPE_BUFFER_STK,
    TAF_SMS_MO_TYPE_MAX
};
typedef VOS_UINT8 TAF_SMS_MO_TYPE_ENUM_UINT8;

/*****************************************************************************
  4 全局变量声明
*****************************************************************************/


/*****************************************************************************
  5 消息头定义
*****************************************************************************/


/*****************************************************************************
  6 消息定义
*****************************************************************************/


/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/

typedef struct
{
    VOS_MSG_HEADER
    VOS_UINT32                          ulMsgId;
    MN_CLIENT_ID_T                      usClientId;                             /* Client ID */
    MN_OPERATION_ID_T                   ucOpId;                                 /* Operation ID */
    TAF_CALL_REL_CALL_TYPE_ENUM_UINT8   enRelType;                              /* 释放呼叫类型 */
}TAF_CALL_TAF_REL_CALL_REQ_STRU;
typedef struct
{
    VOS_MSG_HEADER
    VOS_UINT32                          ulMsgId;
    MN_CLIENT_ID_T                      usClientId;                             /* Client ID */
    MN_OPERATION_ID_T                   ucOpId;                                 /* Operation ID */
    VOS_UINT8                           aucReserve[1];                          /* 保留 */
    TAF_CS_CAUSE_ENUM_UINT32            enCause;                                /* 呼叫释放原因值 */
}TAF_CALL_TAF_REL_CALL_CNF_STRU;


typedef struct
{
    VOS_MSG_HEADER
    VOS_UINT32                          ulMsgId;
    MN_CLIENT_ID_T                      clientId;                               /* Client ID */
    MN_OPERATION_ID_T                   opId;                                   /* Operation ID */
    VOS_UINT8                           aucReserve[1];                         /* 保留 */
    VOS_UINT8                           aucContent[4];                          /* 消息内容 */
}TAF_SPM_SMMA_IND_STRU;


typedef struct
{
    VOS_MSG_HEADER
    VOS_UINT32                          ulMsgId;
    MN_CLIENT_ID_T                      clientId;                               /* Client ID */
    MN_OPERATION_ID_T                   opId;                                   /* Operation ID */
    VOS_UINT8                           aucReserve[1];                          /* 保留 */
    TAF_MSG_SIGNALLING_TYPE_ENUM_UINT32 enMsgSignallingType;
}TAF_SPM_SMMA_RSP_STRU;
typedef struct
{
    VOS_MSG_HEADER
    VOS_UINT32                          ulMsgId;
    MN_CLIENT_ID_T                      usClientId;                            /* Client ID */
    MN_OPERATION_ID_T                   ucOpId;                                /* Operation ID */
    TAF_SMS_MO_TYPE_ENUM_UINT8          enSmsMoType;                           /* 短信MO type */
    VOS_UINT8                           ucReportFlag;                          /* SMMA report标志 */
    MN_OPERATION_ID_T                   ucBcOpId;                              /* 广播OPID */
    VOS_UINT8                           aucReserve[2];                         /* 保留位 */
    TAF_MSG_SIGNALLING_TYPE_ENUM_UINT32 enSignallingType;                      /* report消息信令类型 */
    MN_MSG_SUBMIT_RPT_EVT_INFO_STRU     stRptEvtInfo;                          /* 短信report事件信息 */
}TAF_SPM_MSG_REPORT_IND_STRU;


typedef struct
{
    VOS_MSG_HEADER
    VOS_UINT32                          ulMsgId;
    TAF_MSG_ERROR_ENUM_UINT32           enRslt;                                 /* 检查结果 */
    MN_CLIENT_ID_T                      usClientId;                             /* Client ID */
    MN_OPERATION_ID_T                   ucOpId;                                 /* Operation ID */
    VOS_UINT8                           aucReserve[1];                          /* 保留位 */
}TAF_SPM_MSG_CHECK_RESULT_IND_STRU;


typedef struct
{
    VOS_MSG_HEADER
    VOS_UINT32                          ulMsgId;
    MN_CLIENT_ID_T                      clientId;                               /* Client ID */
    MN_OPERATION_ID_T                   opId;                                   /* Operation ID */
    VOS_UINT8                           aucReserve[1];                         /* 保留 */
    VOS_UINT8                           aucContent[4];                          /* 消息内容 */
}TAF_INTERNAL_MSG_STRU;

/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/

/* ASN解析结构 */
typedef struct
{
    VOS_UINT32                          ulMsgId;                                /*_H2ASN_MsgChoice_Export TAF_INTERNAL_MSG_TYPE_ENUM_UINT32 */
    VOS_UINT8                           aucMsg[4];
    /***************************************************************************
        _H2ASN_MsgChoice_When_Comment          TAF_INTERNAL_MSG_TYPE_ENUM_UINT32
    ****************************************************************************/
}TAF_INTERNAL_MSG_DATA;
/*_H2ASN_Length UINT32*/


typedef struct
{
    VOS_MSG_HEADER
    TAF_INTERNAL_MSG_DATA               stMsgData;
}TafInternal_MSG;

/*****************************************************************************
  10 函数声明
*****************************************************************************/






#if (VOS_OS_VER == VOS_WIN32)
#pragma pack()
#else
#pragma pack(0)
#endif




#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of TafInternalInterface.h */



