
#ifndef __NASCC_H__
#define __NASCC_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#pragma pack(4)

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "vos.h"
#include "AtMnInterface.h"

/*****************************************************************************
  2 枚举定义
*****************************************************************************/



/*****************************************************************************
  3 类型定义
*****************************************************************************/

/*****************************************************************************
 3 接口函数声明
*****************************************************************************/
/*****************************************************************************
 函 数 名  : NAS_CC_PidInit
 功能描述  : CC PID初始化函数，在FID初始化时被调用
 输入参数  : enInitPhase - 初始化的阶段
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2008年2月16日
    作    者   : 丁庆 49431
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32  NAS_CC_PidInit(
    enum VOS_INIT_PHASE_DEFINE          enInitPhase
);

/*****************************************************************************
 函 数 名  : WuepsSsPidInit
 功能描述  : SS PID初始化函数，在FID初始化时被调用
 输入参数  : enInitPhase - 初始化的阶段
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2008年2月16日
    作    者   : 丁庆 49431
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32  WuepsSsPidInit(
    enum VOS_INIT_PHASE_DEFINE          enInitPhase
);
/*****************************************************************************
 函 数 名  : WuepsTcPidInit
 功能描述  : TC PID初始化函数，在FID初始化时被调用
 输入参数  : enInitPhase - 初始化的阶段
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2008年2月16日
    作    者   : 丁庆 49431
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32  WuepsTcPidInit(
    enum VOS_INIT_PHASE_DEFINE          enInitPhase
);
/*****************************************************************************
 函 数 名  : WuepsSmsPidInit
 功能描述  : SMS PID初始化函数，在FID初始化时被调用
 输入参数  : enInitPhase - 初始化的阶段
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2008年2月16日
    作    者   : 丁庆 49431
    修改内容   : 新生成函数

*****************************************************************************/
VOS_UINT32  WuepsSmsPidInit(
    enum VOS_INIT_PHASE_DEFINE          enInitPhase
);


/*****************************************************************************
 函 数 名  : NAS_CC_MsgRoute
 功能描述  : CC PID的消息入口，在此进行消息的分发
 输入参数  : pMsg - 外部发给CC PID的消息
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2008年2月16日
    作    者   : 丁庆 49431
    修改内容   : 新生成函数

*****************************************************************************/
VOS_VOID  NAS_CC_MsgRoute(
    struct MsgCB* pMsg
);

VOS_VOID NAS_CC_ProcessSpecTypeBufferMsg(VOS_UINT32 ulEventType);

/*****************************************************************************
 函 数 名  : Ss_TaskEntry_T
 功能描述  : SS PID的消息入口，在此进行消息的分发
 输入参数  : pMsg - 外部发给SS PID的消息
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2008年2月16日
    作    者   : 丁庆 49431
    修改内容   : 新生成函数

*****************************************************************************/
extern VOS_VOID Ss_TaskEntry_T( struct MsgCB* pMsg );
/*****************************************************************************
 函 数 名  : Tc_TaskEntry
 功能描述  : TC PID的消息入口，在此进行消息的分发
 输入参数  : pMsg - 外部发给TC PID的消息
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2008年2月16日
    作    者   : 丁庆 49431
    修改内容   : 新生成函数

*****************************************************************************/
extern VOS_VOID Tc_TaskEntry  (struct MsgCB * pRcvMsg);
/*****************************************************************************
 函 数 名  : SMS_TaskEntry
 功能描述  : SMS PID的消息入口，在此进行消息的分发
 输入参数  : pMsg - 外部发给SMS PID的消息
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2008年2月16日
    作    者   : 丁庆 49431
    修改内容   : 新生成函数

*****************************************************************************/
extern VOS_VOID SMS_TaskEntry  (struct MsgCB * pRcvMsg);


#if ((VOS_OS_VER == VOS_WIN32) || (VOS_OS_VER == VOS_NUCLEUS))
#pragma pack()
#else
#pragma pack(0)
#endif


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* __NASCC_H__ */

