

#ifndef __IMSANVINTERFACE_H__
#define __IMSANVINTERFACE_H__

/*****************************************************************************
  1 Include Headfile
*****************************************************************************/

#include    "vos.h"

/*****************************************************************************
  1.1 Cplusplus Announce
*****************************************************************************/
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/*****************************************************************************
#pragma pack(*)    �����ֽڶ��뷽ʽ
*****************************************************************************/
#if (VOS_OS_VER != VOS_WIN32)
#pragma pack(4)
#else
#pragma pack(push, 4)
#endif

/*****************************************************************************
  2 macro
*****************************************************************************/
#define IMSA_PASSWORD_LEN               (129)
#define IMSA_IMPI_LEN                   (129)
#define IMSA_IMPU_LEN                   (129)
#define IMSA_HOME_NET_DOMAIN_NAME_LEN   (129)

#define IMSA_IMS_MAX_REDIAL_CAUSE_NUM   (16)

#define IMSA_NV_IPV4_ADDR_LEN           (4)
#define IMSA_NV_IPV6_ADDR_LEN           (16)

/*zhaochen 00308719 begin for card lock 2015-10-31*/
#define IMSA_NV_PLMN_LIST_LEN           (128)
#define IMSA_NV_PLMN_LEN                (3)
/*zhaochen 00308719 end for card lock 2015-10-31*/


/*****************************************************************************
  3 Massage Declare
*****************************************************************************/



/*****************************************************************************
  4 Enum
*****************************************************************************/
enum IMSA_PCSCF_DISCOVERY_POLICY_ENUM
{
    IMSA_PCSCF_DISCOVERY_POLICY_PCO           = 0,      /**< PCO��ʽ��ȡP-CSCF */
    IMSA_PCSCF_DISCOVERY_POLICY_NV            = 1,      /**< NV����P-CSCF */
    IMSA_PCSCF_DISCOVERY_POLICY_BUTT
};
typedef VOS_UINT8 IMSA_PCSCF_DISCOVERY_POLICY_ENUM_UINT8;


/*****************************************************************************
  5 STRUCT
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucGsmImsSupportFlag;    /**< GSM IMSʹ����,VOS_TRUE :֧�֣�VOS_FALSE :��֧�� */
    VOS_UINT8                           ucUtranImsSupportFlag; /**< UNTRAN IMSʹ����,VOS_TRUE :֧�֣�VOS_FALSE :��֧�� */
    VOS_UINT8                           ucLteImsSupportFlag;    /**< LTE IMSʹ����,VOS_TRUE :֧�֣�VOS_FALSE :��֧�� */
    VOS_UINT8                           ucGsmEmsSupportFlag;    /**< GSM EMSʹ����,VOS_TRUE :֧�֣�VOS_FALSE :��֧�� */
    VOS_UINT8                           ucUtranEmsSupportFlag; /**< UNTRAN EMSʹ����,VOS_TRUE :֧�֣�VOS_FALSE :��֧�� */
    VOS_UINT8                           ucLteEmsSupportFlag;    /**< LTE EMSʹ����,VOS_TRUE :֧�֣�VOS_FALSE :��֧�� */
    VOS_UINT8                           ucRoamingImsNotSupportFlag; /**< ���������²�����IMSע��ʹ���VOS_TRUE:֧�� VOS_FALSE:��֧�� */
    VOS_UINT8                           ucReserved2;

}IMSA_NV_IMS_RAT_SUPPORT_STRU;

typedef struct
{
    VOS_UINT16                          usImsMinPort;           /* IMS�˿ںŷ�Χ���� */
    VOS_UINT16                          usImsMaxPort;           /* IMS�˿ںŷ�Χ���� */

}IMSA_NV_SIP_PORT_CONFIG_STRU;

typedef struct
{
    VOS_UINT8                           ucIsCallRedialSupportFlg;   /* �Ƿ�֧��IMS�����ز����ܣ�VOS_TRUE:֧�֣�VOS_FALSE:��֧��*/
    VOS_UINT8                           ucRedialCauseNum;           /* �ز�ԭ��ֵ���� */
    VOS_UINT8                           aucReserved1[2];
    VOS_UINT32                          ulCallRedialMaxTime;        /* IMS�����ز����ܴ�ʱ���ز����ʱ������λ���� */
    VOS_UINT32                          ulCallRedialInterval;       /* IMS�����ز����ܴ�ʱ���ز��������λ���� */
    VOS_UINT32                          ulCallRedialMaxNum;         /* IMS�����ز����ܴ�ʱ���ز�������  */
    VOS_UINT16                          ausRedialCauseList[IMSA_IMS_MAX_REDIAL_CAUSE_NUM];       /* �ز�ԭ��ֵ�б� */
}IMSA_NV_IMS_REDIAL_CFG_STRU;

typedef struct
{
    VOS_UINT8                           ucCMCCCustomDeregFlag;    /**< �й��ƶ���������, ��������ʱȥע��ʹ����*/
    VOS_UINT8                           ucCMCCCustomTcallFlag;    /**< �й��ƶ���������, Tcall SilentRedialʹ����*/
    VOS_UINT8                           ucCMCCCustomTqosFlag;     /**< �й��ƶ���������, Tqos SilentRedialʹ����*/
    VOS_UINT8                           aucReserved[1];

    VOS_UINT32                          ulTcallTimerLen;            /**< �й��ƶ���������, Tcall timer��ʱ��ʱ��, ��λΪms*/
    VOS_UINT32                          ulTqosTimerLen;             /**< �й��ƶ���������, Tqos timer��ʱ��ʱ��, ��λΪms*/
} IMSA_NV_CMCC_CUSTOM_REQ_STRU;

typedef struct
{
    VOS_UINT8                           ucNoCardEmcCallSupportFlag;/**< �޿�������ʹ����,VOS_TRUE :֧�֣�VOS_FALSE :��֧�� */
    VOS_UINT8                           ucUeRelMediaPdpFLag;    /**< UE�����ͷ�ý����ر�ʶ */
    VOS_UINT8                           ucImsLocDeregSigPdpRelFlag;/**< IMS��ͨ������ͷź󱾵�IMSȥע���ʶ */
    VOS_UINT8                           ucRetryTimes;              /**< IMSע��ʧ���ٳ���ע�������0ΪĬ��ֵ��Ϊ���޴� */
    VOS_UINT32                          ulMaxTime;              /**< max time����λΪs */
    VOS_UINT32                          ulBaseTime;             /**< base time����λΪs */
    VOS_UINT32                          ulRegFailRetryIntervel; /**< ע��ʧ�����Լ������λΪs��0��ʾ�ǹ̶���� */
    VOS_UINT32                          ulRegTimerLen;
    IMSA_NV_IMS_REDIAL_CFG_STRU         stImsRedialCfg;         /**< IMS�����ز����� */
    IMSA_NV_CMCC_CUSTOM_REQ_STRU        stCMCCCustomReq;        /**< �й��ƶ��������� */
}IMSA_NV_IMSA_CONFIG_STRU;

typedef struct
{
    VOS_UINT8                           ucVoiceCallOnImsSupportFlag;        /**< IMS����ʹ����,VOS_TRUE :֧�֣�VOS_FALSE :��֧�� */
    VOS_UINT8                           ucVideoCallOnImsSupportFlag;        /**< IMS��Ƶʹ����,VOS_TRUE :֧�֣�VOS_FALSE :��֧�� */
    VOS_UINT8                           ucSmsOnImsSupportFlag;              /**< IMS����ʹ����,VOS_TRUE :֧�֣�VOS_FALSE :��֧�� */
    VOS_UINT8                           ucSrvccOnImsSupportFlag;            /**< IMS Srvccʹ����,VOS_TRUE :֧�֣�VOS_FALSE :��֧�� */
    VOS_UINT8                           ucSrvccMidCallOnImsSupportFlag;     /**< IMS SrvccMidCallʹ����,VOS_TRUE :֧�֣�VOS_FALSE :��֧�� */
    VOS_UINT8                           ucSrvccAlertingOnImsSupportFlag;    /**< IMS SrvccAlertingʹ����,VOS_TRUE :֧�֣�VOS_FALSE :��֧�� */
    VOS_UINT8                           ucSrvccPreAlertingOnImsSupportFlag; /**< IMS SrvccPreAlertingʹ����,VOS_TRUE :֧�֣�VOS_FALSE :��֧�� */
    VOS_UINT8                           ucSrvccTiFlag;                      /**< IMS SrvccPreAlerting,VOS_TRUE :0 R10�汾,VOS_FALSE R11֮��汾 */
}IMS_NV_IMS_CAP_STRU;


typedef struct
{
    VOS_UINT8                           ucUserInfoFlag;   /**< 0:��USIM��IMPI��IMPU,DOMAIN   1:��NV��ȡIMPI��IMPU,DOMAIN */
    VOS_UINT8                           ucAuthType;                         /**< IMS ��Ȩ��ʽ  0:AKA  1:DIGIST 2:AKA_IPSEC */
    VOS_UINT8                           aucReserved[2];
    VOS_INT8                            acImpi[IMSA_IMPI_LEN]; /**< Ĭ��49�ֽ� */
    VOS_INT8                            acTImpu[IMSA_IMPU_LEN]; /**< Ĭ��53�ֽ� */
    VOS_INT8                            acHomeNetDomainName[IMSA_HOME_NET_DOMAIN_NAME_LEN]; /**< Ĭ��33�ֽ� */
    VOS_INT8                            aucPassWord[IMSA_PASSWORD_LEN];     /**< ��Ȩ��ʽΪDIGISTʱ:�������� */
}IMSA_NV_IMS_CONFIG_STRU;

typedef struct
{
    VOS_UINT32                          bitOpPrimPcscfAddr  : 1;
    VOS_UINT32                          bitOpSecPcscfAddr   : 1; /**< ���bitOpSecPcscfAddrΪ1��bitOpPrimPcscfAddrҲ����Ϊ1*/
    VOS_UINT32                          bitOpSpare          : 30;

    VOS_UINT8                           aucPrimPcscfAddr[IMSA_NV_IPV4_ADDR_LEN];
    VOS_UINT8                           aucSecPcscfAddr[IMSA_NV_IPV4_ADDR_LEN];
} IMSA_NV_PDP_IPV4_PCSCF_STRU;

typedef struct
{
    VOS_UINT32                          bitOpPrimPcscfAddr  : 1;
    VOS_UINT32                          bitOpSecPcscfAddr   : 1; /**< ���bitOpSecPcscfAddrΪ1��bitOpPrimPcscfAddrҲ����Ϊ1*/
    VOS_UINT32                          bitOpSpare          : 30;

    VOS_UINT8                           aucPrimPcscfAddr[IMSA_NV_IPV6_ADDR_LEN];
    VOS_UINT8                           aucSecPcscfAddr[IMSA_NV_IPV6_ADDR_LEN];
} IMSA_NV_PDP_IPV6_PCSCF_STRU;


typedef struct
{
    IMSA_PCSCF_DISCOVERY_POLICY_ENUM_UINT8  enPcscfDiscoverPolicy;  /**< P-CSCF��ȡ���� */
    VOS_UINT8                               aucReserved[3];
    IMSA_NV_PDP_IPV4_PCSCF_STRU             stIpv4Pcscf;            /**< ����IPv4 P-CSCF��Ϣ */
    IMSA_NV_PDP_IPV6_PCSCF_STRU             stIpv6Pcscf;            /**< ����IPv6 P-CSCF��Ϣ */
}IMSA_NV_PCSCF_DISCOVERY_POLICY_STRU;

/*zhaochen 00308719 begin for card lock 2015-10-31*/
/*****************************************************************************
�ṹ����    :IMSA_NV_PLMN_ID_STRU��
ʹ��˵��    :
    MCC, Mobile country code (aucPlmnId[0], aucPlmnId[1] bits 1 to 4)
    MNC, Mobile network code (aucPlmnId[2], aucPlmnId[1] bits 5 to 8).

    The coding of this field is the responsibility of each administration but BCD
    coding shall be used. The MNC shall consist of 2 or 3 digits. For PCS 1900 for NA,
    Federal regulation mandates that a 3-digit MNC shall be used. However a network
    operator may decide to use only two digits in the MNC over the radio interface.
    In this case, bits 5 to 8 of octet 4 shall be coded as "1111". Mobile equipment
    shall accept MNC coded in such a way.

    ---------------------------------------------------------------------------
                 ||(BIT8)|(BIT7)|(BIT6)|(BIT5)|(BIT4)|(BIT3)|(BIT2)|(BIT1)
    ---------------------------------------------------------------------------
    aucPlmnId[0] ||    MCC digit 2            |           MCC digit 1
    ---------------------------------------------------------------------------
    aucPlmnId[1] ||    MNC digit 3            |           MCC digit 3
    ---------------------------------------------------------------------------
    aucPlmnId[2] ||    MNC digit 2            |           MNC digit 1
    ---------------------------------------------------------------------------
    e.g.
    PLMN 460 10 תΪΪ 0x64 0xF0 0x01
    PLMN 123 01 תΪΪ 0x21 0xF3 0x10
    PLMN 123 456תΪΪ 0x21 0x63 0x54

*****************************************************************************/
typedef struct
{
    VOS_UINT8                           aucPlmnId[IMSA_NV_PLMN_LEN];
    VOS_UINT8                           ucRsv;
}IMSA_NV_PLMN_ID_STRU;

typedef struct
{
    VOS_UINT8                           ucEnableCardLockFlag;       /**< �Ƿ���������ܣ�0�رգ�1�� */
    VOS_UINT8                           ucPlmnNum;                  /**< ֧�ֵ�PLMN�б����� */
    VOS_UINT8                           ucReserved1;
    VOS_UINT8                           ucReserved2;
    IMSA_NV_PLMN_ID_STRU                astPlmnList[IMSA_NV_PLMN_LIST_LEN]; /**< ֧�ֵ�PLMN�б� */
}IMSA_NV_CARD_LOCK_STRU;
/*zhaochen 00308719 end for card lock 2015-10-31*/

/* zhaochen 00308719 begin for HIFI mailbox full reset 2015-11-09 */
typedef struct
{
    VOS_UINT8                           ucHifiDataControlFlag; /*�Ƿ�֧��HIFI������TRUE:֧�֣�FALSE����֧�� */
    VOS_UINT8                           ucHifiDataNeedAckNum;  /*�Ƿ�֧��ҵ��̬VoWifi -> VoLTE: TRUE:֧�֣�FALSE����֧�� */
    VOS_UINT8                           ucHifiDatMaxBufferNum; /*�Ƿ�֧��ҵ��̬VoLTE -> VoWifi: TRUE:֧�֣�FALSE����֧�� */
    VOS_UINT8                           ucRsv1;

    VOS_UINT8                           ucRsv2;
    VOS_UINT8                           ucRsv3;
    VOS_UINT8                           ucRsv4;
    VOS_UINT8                           ucRsv5;

    VOS_UINT8                           ucRsv6;
    VOS_UINT8                           ucRsv7;
    VOS_UINT8                           ucRsv8;
    VOS_UINT8                           ucRsv9;

    VOS_UINT8                           ucRsv10;
    VOS_UINT8                           ucRsv11;
    VOS_UINT8                           ucRsv12;
    VOS_UINT8                           ucRsv13;

    VOS_UINT8                           ucRsv14;
    VOS_UINT8                           ucRsv15;
    VOS_UINT8                           ucRsv16;
    VOS_UINT8                           ucRsv17;

    VOS_UINT16                          usRsv1;
    VOS_UINT16                          usRsv2;

    VOS_UINT16                          usRsv3;
    VOS_UINT16                          usRsv4;

    VOS_UINT16                          usRsv5;
    VOS_UINT16                          usRsv6;

    VOS_UINT16                          usRsv7;
    VOS_UINT16                          usRsv8;

    VOS_UINT16                          usRsv9;
    VOS_UINT16                          usRsv10;

    VOS_UINT32                          ulRsv1;
    VOS_UINT32                          ulRsv2;

    VOS_UINT32                          ulRsv3;
    VOS_UINT32                          ulRsv4;

    VOS_UINT32                          ulRsv5;
    VOS_UINT32                          ulRsv6;

    VOS_UINT32                          ulRsv7;
    VOS_UINT32                          ulRsv8;

    VOS_UINT32                          ulRsv9;
    VOS_UINT32                          ulRsv10;

    VOS_UINT32                          ulRsv11;
    VOS_UINT32                          ulRsv12;
} IMSA_COMM_PARA_CONFIG_STRU;

/* zhaochen 00308719 end for HIFI mailbox full reset 2015-11-09 */

/*****************************************************************************
  6 UNION
*****************************************************************************/


/*****************************************************************************
  7 Extern Global Variable
*****************************************************************************/

/*****************************************************************************
  8 Fuction Extern
*****************************************************************************/





/*****************************************************************************
  9 OTHERS
*****************************************************************************/


#if (VOS_OS_VER != VOS_WIN32)
#pragma pack()
#else
#pragma pack(pop)
#endif




#ifdef __cplusplus
    #if __cplusplus
            }
    #endif
#endif

#endif /* end of ImsaNvInterface.h */



