/*
 *  linux/drivers/mmc/core/mmc.c
 *
 *  Copyright (C) 2003-2004 Russell King, All Rights Reserved.
 *  Copyright (C) 2005-2007 Pierre Ossman, All Rights Reserved.
 *  MMCv4 support Copyright (C) 2006 Philip Langdale, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/err.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/pm_runtime.h>

#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/mmc.h>

#include <linux/hisi/mmc_trace.h>

#include "core.h"
#include "bus.h"
#include "mmc_ops.h"
#include "sd_ops.h"
#ifdef CONFIG_HUAWEI_EMMC_DSM
#include <linux/mmc/dsm_emmc.h>
#endif

static const unsigned int tran_exp[] = {
	10000,		100000,		1000000,	10000000,
	0,		0,		0,		0
};

static const unsigned char tran_mant[] = {
	0,	10,	12,	13,	15,	20,	25,	30,
	35,	40,	45,	50,	55,	60,	70,	80,
};

static const unsigned int tacc_exp[] = {
	1,	10,	100,	1000,	10000,	100000,	1000000, 10000000,
};

static const unsigned int tacc_mant[] = {
	0,	10,	12,	13,	15,	20,	25,	30,
	35,	40,	45,	50,	55,	60,	70,	80,
};

#define UNSTUFF_BITS(resp,start,size)					\
	({								\
		const int __size = size;				\
		const u32 __mask = (__size < 32 ? 1 << __size : 0) - 1;	\
		const int __off = 3 - ((start) / 32);			\
		const int __shft = (start) & 31;			\
		u32 __res;						\
									\
		__res = resp[__off] >> __shft;				\
		if (__size + __shft > 32)				\
			__res |= resp[__off-1] << ((32 - __shft) % 32);	\
		__res & __mask;						\
	})

/*
 * Given the decoded CSD structure, decode the raw CID to our CID structure.
 */
static int mmc_decode_cid(struct mmc_card *card)
{
	u32 *resp = card->raw_cid;

	/*
	 * The selection of the format here is based upon published
	 * specs from sandisk and from what people have reported.
	 */
	switch (card->csd.mmca_vsn) {
	case 0: /* MMC v1.0 - v1.2 */
	case 1: /* MMC v1.4 */
		card->cid.manfid	= UNSTUFF_BITS(resp, 104, 24);
		card->cid.prod_name[0]	= UNSTUFF_BITS(resp, 96, 8);
		card->cid.prod_name[1]	= UNSTUFF_BITS(resp, 88, 8);
		card->cid.prod_name[2]	= UNSTUFF_BITS(resp, 80, 8);
		card->cid.prod_name[3]	= UNSTUFF_BITS(resp, 72, 8);
		card->cid.prod_name[4]	= UNSTUFF_BITS(resp, 64, 8);
		card->cid.prod_name[5]	= UNSTUFF_BITS(resp, 56, 8);
		card->cid.prod_name[6]	= UNSTUFF_BITS(resp, 48, 8);
		card->cid.hwrev		= UNSTUFF_BITS(resp, 44, 4);
		card->cid.fwrev		= UNSTUFF_BITS(resp, 40, 4);
		card->cid.serial	= UNSTUFF_BITS(resp, 16, 24);
		card->cid.month		= UNSTUFF_BITS(resp, 12, 4);
		card->cid.year		= UNSTUFF_BITS(resp, 8, 4) + 1997;
		break;

	case 2: /* MMC v2.0 - v2.2 */
	case 3: /* MMC v3.1 - v3.3 */
	case 4: /* MMC v4 */
		card->cid.manfid	= UNSTUFF_BITS(resp, 120, 8);
		card->cid.oemid		= UNSTUFF_BITS(resp, 104, 16);
		card->cid.prod_name[0]	= UNSTUFF_BITS(resp, 96, 8);
		card->cid.prod_name[1]	= UNSTUFF_BITS(resp, 88, 8);
		card->cid.prod_name[2]	= UNSTUFF_BITS(resp, 80, 8);
		card->cid.prod_name[3]	= UNSTUFF_BITS(resp, 72, 8);
		card->cid.prod_name[4]	= UNSTUFF_BITS(resp, 64, 8);
		card->cid.prod_name[5]	= UNSTUFF_BITS(resp, 56, 8);
		card->cid.prv		= UNSTUFF_BITS(resp, 48, 8);
		card->cid.serial	= UNSTUFF_BITS(resp, 16, 32);
		card->cid.month		= UNSTUFF_BITS(resp, 12, 4);
		card->cid.year		= UNSTUFF_BITS(resp, 8, 4) + 1997;
		break;

	default:
		pr_err("%s: card has unknown MMCA version %d\n",
			mmc_hostname(card->host), card->csd.mmca_vsn);
		return -EINVAL;
	}

	return 0;
}

static void mmc_set_erase_size(struct mmc_card *card)
{
	if (card->ext_csd.erase_group_def & 1)
		card->erase_size = card->ext_csd.hc_erase_size;
	else
		card->erase_size = card->csd.erase_size;

	mmc_init_erase(card);
}

/*
 * Given a 128-bit response, decode to our card CSD structure.
 */
static int mmc_decode_csd(struct mmc_card *card)
{
	struct mmc_csd *csd = &card->csd;
	unsigned int e, m, a, b;
	u32 *resp = card->raw_csd;

	/*
	 * We only understand CSD structure v1.1 and v1.2.
	 * v1.2 has extra information in bits 15, 11 and 10.
	 * We also support eMMC v4.4 & v4.41.
	 */
	csd->structure = UNSTUFF_BITS(resp, 126, 2);
	if (csd->structure == 0) {
		pr_err("%s: unrecognised CSD structure version %d\n",
			mmc_hostname(card->host), csd->structure);
		return -EINVAL;
	}

	csd->mmca_vsn	 = UNSTUFF_BITS(resp, 122, 4);
	m = UNSTUFF_BITS(resp, 115, 4);
	e = UNSTUFF_BITS(resp, 112, 3);
	csd->tacc_ns	 = (tacc_exp[e] * tacc_mant[m] + 9) / 10;
	csd->tacc_clks	 = UNSTUFF_BITS(resp, 104, 8) * 100;

	m = UNSTUFF_BITS(resp, 99, 4);
	e = UNSTUFF_BITS(resp, 96, 3);
	csd->max_dtr	  = tran_exp[e] * tran_mant[m];
	csd->cmdclass	  = UNSTUFF_BITS(resp, 84, 12);

	e = UNSTUFF_BITS(resp, 47, 3);
	m = UNSTUFF_BITS(resp, 62, 12);
	csd->capacity	  = (1 + m) << (e + 2);

	csd->read_blkbits = UNSTUFF_BITS(resp, 80, 4);
	csd->read_partial = UNSTUFF_BITS(resp, 79, 1);
	csd->write_misalign = UNSTUFF_BITS(resp, 78, 1);
	csd->read_misalign = UNSTUFF_BITS(resp, 77, 1);
	csd->dsr_imp = UNSTUFF_BITS(resp, 76, 1);
	csd->r2w_factor = UNSTUFF_BITS(resp, 26, 3);
	csd->write_blkbits = UNSTUFF_BITS(resp, 22, 4);
	csd->write_partial = UNSTUFF_BITS(resp, 21, 1);

	if (csd->write_blkbits >= 9) {
		a = UNSTUFF_BITS(resp, 42, 5);
		b = UNSTUFF_BITS(resp, 37, 5);
		csd->erase_size = (a + 1) * (b + 1);
		csd->erase_size <<= csd->write_blkbits - 9;
	}

	return 0;
}

static void mmc_select_card_type(struct mmc_card *card)
{
	struct mmc_host *host = card->host;
	u8 card_type = card->ext_csd.raw_card_type;
	u32 caps = host->caps, caps2 = host->caps2;
	unsigned int hs_max_dtr = 0, hs200_max_dtr = 0;
	unsigned int avail_type = 0;

    if (caps & MMC_CAP_MMC_HIGHSPEED &&
        card_type & EXT_CSD_CARD_TYPE_HS_26) {
		hs_max_dtr = MMC_HIGH_26_MAX_DTR;
        avail_type |= EXT_CSD_CARD_TYPE_HS_26;
    }

	if (caps & MMC_CAP_MMC_HIGHSPEED &&
        card_type & EXT_CSD_CARD_TYPE_HS_52) {
		hs_max_dtr = MMC_HIGH_52_MAX_DTR;
        avail_type |= EXT_CSD_CARD_TYPE_HS_52;
    }

    if (caps & MMC_CAP_1_8V_DDR &&
        card_type & EXT_CSD_CARD_TYPE_DDR_1_8V) {
		hs_max_dtr = MMC_HIGH_DDR_MAX_DTR;
        avail_type |= EXT_CSD_CARD_TYPE_DDR_1_8V;
    }

    if (caps & MMC_CAP_1_2V_DDR &&
        card_type & EXT_CSD_CARD_TYPE_DDR_1_2V) {
        hs_max_dtr = MMC_HIGH_DDR_MAX_DTR;
        avail_type |= EXT_CSD_CARD_TYPE_DDR_1_2V;
    }

    if (caps2 & MMC_CAP2_HS200_1_8V_SDR &&
        card_type & EXT_CSD_CARD_TYPE_HS200_1_8V) {
		hs200_max_dtr = MMC_HS200_MAX_DTR;
        avail_type |= EXT_CSD_CARD_TYPE_HS200_1_8V;
    }

    if (caps2 & MMC_CAP2_HS200_1_2V_SDR &&
        card_type & EXT_CSD_CARD_TYPE_HS200_1_2V) {
        hs200_max_dtr = MMC_HS200_MAX_DTR;
        avail_type |= EXT_CSD_CARD_TYPE_HS200_1_2V;
    }

    if (caps2 & MMC_CAP2_HS400_1_8V &&
        card_type & EXT_CSD_CARD_TYPE_HS400_1_8V) {
        hs200_max_dtr = MMC_HS200_MAX_DTR;
        avail_type |= EXT_CSD_CARD_TYPE_HS400_1_8V;
    }

    if (caps2 & MMC_CAP2_HS400_1_2V &&
        card_type & EXT_CSD_CARD_TYPE_HS400_1_2V) {
        hs200_max_dtr = MMC_HS200_MAX_DTR;
        avail_type |= EXT_CSD_CARD_TYPE_HS400_1_2V;
    }

	card->ext_csd.hs_max_dtr = hs_max_dtr;
	card->ext_csd.hs200_max_dtr = hs200_max_dtr;
	card->mmc_avail_type = avail_type;

	pr_debug("%s: eMMC mmc_avail_type=0X%x, hs_max_dtr=%d\n", mmc_hostname(card->host), card->mmc_avail_type, card->ext_csd.hs_max_dtr);
}
#ifdef CONFIG_HW_SYSTEM_WR_PROTECT
static int mmc_system_partition_wr_prot_init(struct mmc_card *card, u8 *ext_csd)
{
	int err;
	unsigned char tmp;
/* defined by supplier
 *	err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
 *				 156, 0x01,
 *				 card->ext_csd.generic_cmd6_time);
 *	if (err){
 *		err = -EINVAL;
 *		pr_debug("[HW]:%s, %d: setting PARTITIONS_ATTRIBUTE [156] error \n", __func__, __LINE__);
 *	}
 *	else
 *		pr_debug("[HW]:%s, %d: setting PARTITIONS_ATTRIBUTE [156] = 0x%02x \n", __func__, __LINE__, 0x01);
*/

	if( (ext_csd[EXT_CSD_USER_WP]  & (1<<6)) == 0 ||
		(ext_csd[EXT_CSD_USER_WP]  & (1<<4)) == 0){
		tmp = ext_csd[EXT_CSD_USER_WP];
		tmp |= (1 << 4);
		tmp |= (1 << 6);
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				 EXT_CSD_USER_WP, tmp,
				 card->ext_csd.generic_cmd6_time);
		if (err){
			err = -EINVAL;
			pr_debug("[HW]:%s, %d: setting ext_csd[171] error \n", __func__, __LINE__);
		}else{
			pr_debug("[HW]:%s, %d: USER_WP[171] = 0x%02x \n", __func__, __LINE__, tmp);
		}
	}

	if((ext_csd[EXT_CSD_ERASE_GROUP_DEF] & (1<<0)) == 0){
		tmp = ext_csd[EXT_CSD_ERASE_GROUP_DEF];
		tmp |= (1<<0);
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				 EXT_CSD_ERASE_GROUP_DEF, tmp,
				 card->ext_csd.generic_cmd6_time);
		if (err){
			err = -EINVAL;
			pr_debug("[HW]:%s, %d: setting ext_csd[EXT_CSD_ERASE_GROUP_DEF] error \n", __func__, __LINE__);
		}else{
			pr_debug("[HW]:%s, %d: ERASE_GROUP_DEF [175] = 0x%02x \n", __func__, __LINE__, tmp);
		}

	}

	return 0;
}
#endif

static void mmc_manage_enhanced_area(struct mmc_card *card, u8 *ext_csd)
{
    u8 hc_erase_grp_sz, hc_wp_grp_sz;

    /*
    * Disable these attributes by default
    */
    card->ext_csd.enhanced_area_offset = -EINVAL;
    card->ext_csd.enhanced_area_size = -EINVAL;
   /*
    * Enhanced area feature support -- check whether the eMMC
    * card has the Enhanced area enabled.  If so, export enhanced
    * area offset and size to user by adding sysfs interface.
    */
    if ((ext_csd[EXT_CSD_PARTITION_SUPPORT] & 0x2) &&
        (ext_csd[EXT_CSD_PARTITION_ATTRIBUTE] & 0x1)) {
        if (card->ext_csd.partition_setting_completed) {
            hc_erase_grp_sz =
                ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE];
            hc_wp_grp_sz =
                ext_csd[EXT_CSD_HC_WP_GRP_SIZE];
            /*
            * calculate the enhanced data area offset, in bytes
            */
            card->ext_csd.enhanced_area_offset =
                (ext_csd[139] << 24) + (ext_csd[138] << 16) +
                (ext_csd[137] << 8) + ext_csd[136];
            if (mmc_card_blockaddr(card))
                card->ext_csd.enhanced_area_offset <<= 9;
            /*
            * calculate the enhanced data area size, in kilobytes
            */
            card->ext_csd.enhanced_area_size =
                (ext_csd[142] << 16) + (ext_csd[141] << 8) +
                    ext_csd[140];
            card->ext_csd.enhanced_area_size *=
                (size_t)(hc_erase_grp_sz * hc_wp_grp_sz);
            card->ext_csd.enhanced_area_size <<= 9;
            } else {
                pr_warn("%s: defines enhanced area without partition setting complete\n",
                mmc_hostname(card->host));
            }

    }
}

static void mmc_manage_gp_partitions(struct mmc_card *card, u8 *ext_csd)
{
    int idx;
    u8 hc_erase_grp_sz, hc_wp_grp_sz;
    unsigned int part_size;

   /*
    * General purpose partition feature support --
    * If ext_csd has the size of general purpose partitions,
    * set size, part_cfg, partition name in mmc_part.
    */
    if (ext_csd[EXT_CSD_PARTITION_SUPPORT] &
        EXT_CSD_PART_SUPPORT_PART_EN) {
        hc_erase_grp_sz =
            ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE];
        hc_wp_grp_sz =
            ext_csd[EXT_CSD_HC_WP_GRP_SIZE];

        for (idx = 0; idx < MMC_NUM_GP_PARTITION; idx++) {
            if (!ext_csd[EXT_CSD_GP_SIZE_MULT + idx * 3] &&
                !ext_csd[EXT_CSD_GP_SIZE_MULT + idx * 3 + 1] &&
                !ext_csd[EXT_CSD_GP_SIZE_MULT + idx * 3 + 2])
                    continue;

                if (card->ext_csd.partition_setting_completed == 0) {
                    pr_warn("%s: has partition size defined without partition complete\n",
                    mmc_hostname(card->host));
                    break;
                }
                part_size =
                    (ext_csd[EXT_CSD_GP_SIZE_MULT + idx * 3 + 2]
                    << 16) +
                    (ext_csd[EXT_CSD_GP_SIZE_MULT + idx * 3 + 1]
                    << 8) +
                    ext_csd[EXT_CSD_GP_SIZE_MULT + idx * 3];
                part_size *= (size_t)(hc_erase_grp_sz *
                    hc_wp_grp_sz);
                mmc_part_add(card, part_size << 19,
                    EXT_CSD_PART_CONFIG_ACC_GP0 + idx,
                    "gp%d", idx, false,
                    MMC_BLK_DATA_AREA_GP);
        }
    }
}

/*
 * Decode extended CSD.
 */
static int mmc_decode_ext_csd(struct mmc_card *card, u8 *ext_csd)
{
	int err = 0;
	unsigned int part_size;

	/* Version is coded in the CSD_STRUCTURE byte in the EXT_CSD register */
	card->ext_csd.raw_ext_csd_structure = ext_csd[EXT_CSD_STRUCTURE];
	if (card->csd.structure == 3) {
		if (card->ext_csd.raw_ext_csd_structure > 2) {
			pr_err("%s: unrecognised EXT_CSD structure "
				"version %d\n", mmc_hostname(card->host),
					card->ext_csd.raw_ext_csd_structure);
			err = -EINVAL;
			goto out;
		}
	}

	card->ext_csd.rev = ext_csd[EXT_CSD_REV];
	pr_debug("%s: support eMMC card version=%d\n", mmc_hostname(card->host), card->ext_csd.rev);
	if (card->ext_csd.rev > 8) {
	    /* for support emmc 5.1*/
		pr_err("%s: unrecognised EXT_CSD revision %d\n",
			mmc_hostname(card->host), card->ext_csd.rev);
		/*
		err = -EINVAL;
		goto out;
		*/
	}

	card->ext_csd.raw_sectors[0] = ext_csd[EXT_CSD_SEC_CNT + 0];
	card->ext_csd.raw_sectors[1] = ext_csd[EXT_CSD_SEC_CNT + 1];
	card->ext_csd.raw_sectors[2] = ext_csd[EXT_CSD_SEC_CNT + 2];
	card->ext_csd.raw_sectors[3] = ext_csd[EXT_CSD_SEC_CNT + 3];
	if (card->ext_csd.rev >= 2) {
		card->ext_csd.sectors =
			ext_csd[EXT_CSD_SEC_CNT + 0] << 0 |
			ext_csd[EXT_CSD_SEC_CNT + 1] << 8 |
			ext_csd[EXT_CSD_SEC_CNT + 2] << 16 |
			ext_csd[EXT_CSD_SEC_CNT + 3] << 24;

		/* Cards with density > 2GiB are sector addressed */
		if (card->ext_csd.sectors > (2u * 1024 * 1024 * 1024) / 512)
			mmc_card_set_blockaddr(card);
	}

	card->ext_csd.raw_card_type = ext_csd[EXT_CSD_CARD_TYPE];
	mmc_select_card_type(card);

	card->ext_csd.raw_driver_strength = ext_csd[EXT_CSD_DRIVER_STRENGTH];

	card->ext_csd.raw_s_a_timeout = ext_csd[EXT_CSD_S_A_TIMEOUT];
	card->ext_csd.raw_erase_timeout_mult =
		ext_csd[EXT_CSD_ERASE_TIMEOUT_MULT];
	card->ext_csd.raw_hc_erase_grp_size =
		ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE];
	if (card->ext_csd.rev >= 3) {
		u8 sa_shift = ext_csd[EXT_CSD_S_A_TIMEOUT];
		card->ext_csd.part_config = ext_csd[EXT_CSD_PART_CONFIG];

		/* EXT_CSD value is in units of 10ms, but we store in ms */
		card->ext_csd.part_time = 10 * ext_csd[EXT_CSD_PART_SWITCH_TIME];

		/* Sleep / awake timeout in 100ns units */
		if (sa_shift > 0 && sa_shift <= 0x17)
			card->ext_csd.sa_timeout =
					1 << ext_csd[EXT_CSD_S_A_TIMEOUT];
		card->ext_csd.erase_group_def =
			ext_csd[EXT_CSD_ERASE_GROUP_DEF];
		card->ext_csd.hc_erase_timeout = 300 *
			ext_csd[EXT_CSD_ERASE_TIMEOUT_MULT];
		card->ext_csd.hc_erase_size =
			ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE] << 10;

		card->ext_csd.rel_sectors = ext_csd[EXT_CSD_REL_WR_SEC_C];

		/*
		 * There are two boot regions of equal size, defined in
		 * multiples of 128K.
		 */
		if (ext_csd[EXT_CSD_BOOT_MULT]) {
			part_size = ext_csd[EXT_CSD_BOOT_MULT] << 17;
			mmc_part_add(card, part_size,
				EXT_CSD_PART_CONFIG_ACC_BOOT0,
				"boot%d", 0, true,
				MMC_BLK_DATA_AREA_BOOT);
		}
	}

	card->ext_csd.raw_hc_erase_gap_size =
		ext_csd[EXT_CSD_HC_WP_GRP_SIZE];
	card->ext_csd.raw_sec_trim_mult =
		ext_csd[EXT_CSD_SEC_TRIM_MULT];
	card->ext_csd.raw_sec_erase_mult =
		ext_csd[EXT_CSD_SEC_ERASE_MULT];
	card->ext_csd.raw_sec_feature_support =
		ext_csd[EXT_CSD_SEC_FEATURE_SUPPORT];
	card->ext_csd.raw_trim_mult =
		ext_csd[EXT_CSD_TRIM_MULT];
	card->ext_csd.raw_partition_support = ext_csd[EXT_CSD_PARTITION_SUPPORT];
	if (card->ext_csd.rev >= 4) {
		mmc_manage_enhanced_area(card, ext_csd);

		mmc_manage_gp_partitions(card, ext_csd);

		card->ext_csd.sec_trim_mult =
			ext_csd[EXT_CSD_SEC_TRIM_MULT];
		card->ext_csd.sec_erase_mult =
			ext_csd[EXT_CSD_SEC_ERASE_MULT];
		card->ext_csd.sec_feature_support =
			ext_csd[EXT_CSD_SEC_FEATURE_SUPPORT];
		card->ext_csd.trim_timeout = 300 *
			ext_csd[EXT_CSD_TRIM_MULT];

		/*
		 * Note that the call to mmc_part_add above defaults to read
		 * only. If this default assumption is changed, the call must
		 * take into account the value of boot_locked below.
		 */
		card->ext_csd.boot_ro_lock = ext_csd[EXT_CSD_BOOT_WP];
		card->ext_csd.boot_ro_lockable = true;

		/* Save power class values */
		card->ext_csd.raw_pwr_cl_52_195 =
			ext_csd[EXT_CSD_PWR_CL_52_195];
		card->ext_csd.raw_pwr_cl_26_195 =
			ext_csd[EXT_CSD_PWR_CL_26_195];
		card->ext_csd.raw_pwr_cl_52_360 =
			ext_csd[EXT_CSD_PWR_CL_52_360];
		card->ext_csd.raw_pwr_cl_26_360 =
			ext_csd[EXT_CSD_PWR_CL_26_360];
		card->ext_csd.raw_pwr_cl_200_195 =
			ext_csd[EXT_CSD_PWR_CL_200_195];
		card->ext_csd.raw_pwr_cl_200_360 =
			ext_csd[EXT_CSD_PWR_CL_200_360];
		card->ext_csd.raw_pwr_cl_ddr_200_360 =
			ext_csd[EXT_CSD_PWR_CL_DDR_200_360];
		card->ext_csd.raw_pwr_cl_ddr_52_195 =
			ext_csd[EXT_CSD_PWR_CL_DDR_52_195];
		card->ext_csd.raw_pwr_cl_ddr_52_360 =
			ext_csd[EXT_CSD_PWR_CL_DDR_52_360];
		card->ext_csd.raw_pwr_cl_ddr_200_360 =
			ext_csd[EXT_CSD_PWR_CL_DDR_200_360];
	}

	if (card->ext_csd.rev >= 5) {
		/* check whether the eMMC card supports BKOPS */
		if (ext_csd[EXT_CSD_BKOPS_SUPPORT] & 0x1) {
			card->ext_csd.bkops = 1;
			card->ext_csd.bkops_en = ext_csd[EXT_CSD_BKOPS_EN] & EXT_CSD_BKOPS_MANUAL_EN;
			card->ext_csd.raw_bkops_status =
				ext_csd[EXT_CSD_BKOPS_STATUS];
			if (!card->ext_csd.bkops_en)
				pr_debug("%s: BKOPS_EN bit is not set\n",
					mmc_hostname(card->host));
		}

		/* check whether the eMMC card supports HPI */
		if (ext_csd[EXT_CSD_HPI_FEATURES] & 0x1) {
			card->ext_csd.hpi = 1;
			if (ext_csd[EXT_CSD_HPI_FEATURES] & 0x2)
				card->ext_csd.hpi_cmd =	MMC_STOP_TRANSMISSION;
			else
				card->ext_csd.hpi_cmd = MMC_SEND_STATUS;
			/*
			 * Indicate the maximum timeout to close
			 * a command interrupted by HPI
			 */
			card->ext_csd.out_of_int_time =
				ext_csd[EXT_CSD_OUT_OF_INTERRUPT_TIME] * 10;
			pr_debug("%s: support HPI. ext_csd.hpi=%d\n", mmc_hostname(card->host), card->ext_csd.hpi);
		}

		card->ext_csd.rel_param = ext_csd[EXT_CSD_WR_REL_PARAM];
		card->ext_csd.rst_n_function = ext_csd[EXT_CSD_RST_N_FUNCTION];

		/*
		 * RPMB regions are defined in multiples of 128K.
		 */
		card->ext_csd.raw_rpmb_size_mult = ext_csd[EXT_CSD_RPMB_MULT];
		if (ext_csd[EXT_CSD_RPMB_MULT] && mmc_host_cmd23(card->host)) {
			mmc_part_add(card, ext_csd[EXT_CSD_RPMB_MULT] << 17,
				EXT_CSD_PART_CONFIG_ACC_RPMB,
				"rpmb", 0, false,
				MMC_BLK_DATA_AREA_RPMB);
			pr_debug("%s: support RPMB raw_rpmb_size_mult=%d\n", mmc_hostname(card->host), card->ext_csd.raw_rpmb_size_mult);
		}
	}

	card->ext_csd.raw_erased_mem_count = ext_csd[EXT_CSD_ERASED_MEM_CONT];
	if (ext_csd[EXT_CSD_ERASED_MEM_CONT])
		card->erased_byte = 0xFF;
	else
		card->erased_byte = 0x0;

	/* eMMC v4.5 or later */
	if (card->ext_csd.rev >= 6) {
		card->ext_csd.feature_support |= MMC_DISCARD_FEATURE;

		card->ext_csd.generic_cmd6_time = 10 *
			ext_csd[EXT_CSD_GENERIC_CMD6_TIME];
		card->ext_csd.power_off_longtime = 10 *
			ext_csd[EXT_CSD_POWER_OFF_LONG_TIME];

		card->ext_csd.cache_size =
			ext_csd[EXT_CSD_CACHE_SIZE + 0] << 0 |
			ext_csd[EXT_CSD_CACHE_SIZE + 1] << 8 |
			ext_csd[EXT_CSD_CACHE_SIZE + 2] << 16 |
			ext_csd[EXT_CSD_CACHE_SIZE + 3] << 24;

		if (ext_csd[EXT_CSD_DATA_SECTOR_SIZE] == 1)
			card->ext_csd.data_sector_size = 4096;
		else
			card->ext_csd.data_sector_size = 512;

		if ((ext_csd[EXT_CSD_DATA_TAG_SUPPORT] & 1) &&
		    (ext_csd[EXT_CSD_TAG_UNIT_SIZE] <= 8)) {
			card->ext_csd.data_tag_unit_size =
			((unsigned int) 1 << ext_csd[EXT_CSD_TAG_UNIT_SIZE]) *
			(card->ext_csd.data_sector_size);
		} else {
			card->ext_csd.data_tag_unit_size = 0;
		}

		card->ext_csd.max_packed_writes =
			ext_csd[EXT_CSD_MAX_PACKED_WRITES];
		card->ext_csd.max_packed_reads =
			ext_csd[EXT_CSD_MAX_PACKED_READS];
	} else {
		card->ext_csd.data_sector_size = 512;
	}

	/* eMMC v5.0 or later */
	if (card->ext_csd.rev >= 7) {
		card->ext_csd.pre_eol_info = ext_csd[EXT_CSD_PRE_EOL_INFO];
		card->ext_csd.device_life_time_est_typ_a = ext_csd[EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_A];
		card->ext_csd.device_life_time_est_typ_b = ext_csd[EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_B];
		/*
		 * Vendor proprietary health report, EXT_CSD[301:270].
		 */
		/*for (idx = 0; idx < 32; idx++) {
			pr_debug("%s: support Device Health Report. VENDOR_PROPRIETARY_HEALTH_REPORT[%d]=0X%x\n", mmc_hostname(card->host), idx, ext_csd[EXT_CSD_HEALTH_REPORT + idx]);
		}*/
		/*
		 * SLEEP_NOTIFICATION of EXT_CSD_POWER_OFF_NOTIFICATION, EXT_CSD[301:270].
		 */

		card->ext_csd.raw_sleep_noti_time = ext_csd[EXT_CSD_SLEEP_NOTIFICATION_TIME];
		if (card->ext_csd.raw_sleep_noti_time > 0 && card->ext_csd.raw_sleep_noti_time <= 0x17)
		{
			card->ext_csd.sleep_notification_time = ((unsigned int)1 << card->ext_csd.raw_sleep_noti_time) / 100;/*ms, raw_sleep_noti_time Units: 10us */
			pr_debug("%s: support SLEEP_NOTIFICATION. sleep_notification_time=%d ms\n", mmc_hostname(card->host), card->ext_csd.sleep_notification_time);
		}

		card->ext_csd.cmdq_support = ext_csd[EXT_CSD_CMDQ_SUPPORT] & 0x1;
		if (card->ext_csd.cmdq_support) {
			card->ext_csd.cmdq_depth = (ext_csd[EXT_CSD_CMDQ_DEPTH] & 0x1F) + 1;
			pr_err("%s: %s: CMDQ supported: depth: %d\n",
				mmc_hostname(card->host), __func__,
				card->ext_csd.cmdq_depth);
		}

		memcpy(card->ext_csd.fwrev, &ext_csd[EXT_CSD_FIRMWARE_VERSION],
            MMC_FIRMWARE_LEN);
        card->ext_csd.ffu_capable =
            (ext_csd[EXT_CSD_SUPPORTED_MODE] & 0x1) &&
            !(ext_csd[EXT_CSD_FW_CONFIG] & 0x1);

	}
	else
	{
	       /*0x00 means Not defined, write 0x00 in lower eMMC version*/
		card->ext_csd.pre_eol_info = 0x00;
		card->ext_csd.device_life_time_est_typ_a = 0x00;
		card->ext_csd.device_life_time_est_typ_b = 0x00;
		card->ext_csd.cmdq_support = 0;
		card->ext_csd.cmdq_depth = 0;
	}

	/* eMMC5.1*/
	if (card->ext_csd.rev >= 8) {
/*strobe*/
		if (ext_csd[EXT_CSD_STROBE_SUPPORT]) {
			card->ext_csd.strobe_enhanced = ext_csd[EXT_CSD_STROBE_SUPPORT];
			if (card->ext_csd.strobe_enhanced)
				pr_info("%s: support STROBE_ENHANCED.\n", mmc_hostname(card->host));
			else
				pr_warn("%s: EXT_CSD_STROBE_SUPPORT bit is not set\n", mmc_hostname(card->host));
		}
/*cache flush barrier*/
		if (ext_csd[EXT_CSD_BARRIER_SUPPORT]) {
			card->ext_csd.cache_flush_barrier = ext_csd[EXT_CSD_BARRIER_SUPPORT];
			if (card->ext_csd.cache_flush_barrier)
				pr_info("%s: support BARRIER_SUPPORT.\n", mmc_hostname(card->host));
		}
/*cache flush policy*/
		if (ext_csd[EXT_CSD_CACHE_FLUSH_POLICY]) {
			card->ext_csd.cache_flush_policy = ext_csd[EXT_CSD_CACHE_FLUSH_POLICY] & EXT_CSD_CACHE_FLUSH_POLICY_FIFO;
			if (card->ext_csd.cache_flush_policy)
				pr_info("%s: support CACHE_FLUSH_POLICY_FIFO.\n", mmc_hostname(card->host));
		}
/*bkops auto*/
		if (ext_csd[EXT_CSD_BKOPS_SUPPORT] & 0x1) {
			card->ext_csd.bkops_auto_en = ext_csd[EXT_CSD_BKOPS_EN] & EXT_CSD_BKOPS_AUTO_EN;
			if (!card->ext_csd.bkops_auto_en) {
				pr_info("%s: BKOPS_AUTO_EN bit is not set.\n", mmc_hostname(card->host));
			}
		}
/*rpmb 8k*/
		if (ext_csd[EXT_CSD_WR_REL_PARAM] & EXT_CSD_RPMB_REL_WR_EN) {
			pr_info("%s: support RPMB 8K Bytes read/write.\n", mmc_hostname(card->host));
		}
	}

	pr_err("%s: EXT_CSD revision 0x%02x pre_eol_info = 0x%02X device_life_time_est_typ_a = 0x%02X device_life_time_est_typ_b = 0x%02X.\n",
		mmc_hostname(card->host),card->ext_csd.rev, card->ext_csd.pre_eol_info,
		    card->ext_csd.device_life_time_est_typ_a, card->ext_csd.device_life_time_est_typ_b);

out:
	return err;
}

static int mmc_read_ext_csd(struct mmc_card *card)
{
	u8 *ext_csd;
	int err;

    if (!mmc_can_ext_csd(card))
        return 0;
	err = mmc_get_ext_csd(card, &ext_csd);

    if (err) {
        /* If the host or the card can't do the switch,
         * fail more gracefully. */
        if ((err != -EINVAL)
        && (err != -ENOSYS)
        && (err != -EFAULT))
            return err;

        /*
        * High capacity cards should have this "magic" size
        * stored in their CSD.
        */
        if (card->csd.capacity == (4096 * 512)) {
            pr_err("%s: unable to read EXT_CSD on a possible high capacity card. Card will be ignored.\n",
                mmc_hostname(card->host));
        } else {
            pr_warn("%s: unable to read EXT_CSD, performance might suffer\n",
                mmc_hostname(card->host));
            err = 0;
        }

        return err;
    }

	err = mmc_decode_ext_csd(card, ext_csd);

#ifdef CONFIG_HW_SYSTEM_WR_PROTECT
    err = mmc_system_partition_wr_prot_init(card, ext_csd);
#endif
	kfree(ext_csd);
	return err;
}

static int mmc_compare_ext_csds(struct mmc_card *card, unsigned bus_width)
{
	u8 *bw_ext_csd;
	int err;

	if (bus_width == MMC_BUS_WIDTH_1)
		return 0;

	err = mmc_get_ext_csd(card, &bw_ext_csd);
	if (err)
		return err;

	/* only compare read only fields */
	err = !((card->ext_csd.raw_partition_support ==
			bw_ext_csd[EXT_CSD_PARTITION_SUPPORT]) &&
		(card->ext_csd.raw_erased_mem_count ==
			bw_ext_csd[EXT_CSD_ERASED_MEM_CONT]) &&
		(card->ext_csd.rev ==
			bw_ext_csd[EXT_CSD_REV]) &&
		(card->ext_csd.raw_ext_csd_structure ==
			bw_ext_csd[EXT_CSD_STRUCTURE]) &&
		(card->ext_csd.raw_card_type ==
			bw_ext_csd[EXT_CSD_CARD_TYPE]) &&
		(card->ext_csd.raw_driver_strength ==
			bw_ext_csd[EXT_CSD_DRIVER_STRENGTH]) &&
		(card->ext_csd.raw_s_a_timeout ==
			bw_ext_csd[EXT_CSD_S_A_TIMEOUT]) &&
		(card->ext_csd.raw_hc_erase_gap_size ==
			bw_ext_csd[EXT_CSD_HC_WP_GRP_SIZE]) &&
		(card->ext_csd.raw_erase_timeout_mult ==
			bw_ext_csd[EXT_CSD_ERASE_TIMEOUT_MULT]) &&
		(card->ext_csd.raw_hc_erase_grp_size ==
			bw_ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE]) &&
		(card->ext_csd.raw_sec_trim_mult ==
			bw_ext_csd[EXT_CSD_SEC_TRIM_MULT]) &&
		(card->ext_csd.raw_sec_erase_mult ==
			bw_ext_csd[EXT_CSD_SEC_ERASE_MULT]) &&
		(card->ext_csd.raw_sec_feature_support ==
			bw_ext_csd[EXT_CSD_SEC_FEATURE_SUPPORT]) &&
		(card->ext_csd.raw_trim_mult ==
			bw_ext_csd[EXT_CSD_TRIM_MULT]) &&
		(card->ext_csd.raw_sectors[0] ==
			bw_ext_csd[EXT_CSD_SEC_CNT + 0]) &&
		(card->ext_csd.raw_sectors[1] ==
			bw_ext_csd[EXT_CSD_SEC_CNT + 1]) &&
		(card->ext_csd.raw_sectors[2] ==
			bw_ext_csd[EXT_CSD_SEC_CNT + 2]) &&
		(card->ext_csd.raw_sectors[3] ==
			bw_ext_csd[EXT_CSD_SEC_CNT + 3]) &&
		(card->ext_csd.raw_pwr_cl_52_195 ==
			bw_ext_csd[EXT_CSD_PWR_CL_52_195]) &&
		(card->ext_csd.raw_pwr_cl_26_195 ==
			bw_ext_csd[EXT_CSD_PWR_CL_26_195]) &&
		(card->ext_csd.raw_pwr_cl_52_360 ==
			bw_ext_csd[EXT_CSD_PWR_CL_52_360]) &&
		(card->ext_csd.raw_pwr_cl_26_360 ==
			bw_ext_csd[EXT_CSD_PWR_CL_26_360]) &&
		(card->ext_csd.raw_pwr_cl_200_195 ==
			bw_ext_csd[EXT_CSD_PWR_CL_200_195]) &&
		(card->ext_csd.raw_pwr_cl_200_360 ==
			bw_ext_csd[EXT_CSD_PWR_CL_200_360]) &&
        (card->ext_csd.raw_pwr_cl_ddr_200_360 ==
			bw_ext_csd[EXT_CSD_PWR_CL_DDR_200_360]) &&
		(card->ext_csd.raw_pwr_cl_ddr_52_195 ==
			bw_ext_csd[EXT_CSD_PWR_CL_DDR_52_195]) &&
		(card->ext_csd.raw_pwr_cl_ddr_52_360 ==
			bw_ext_csd[EXT_CSD_PWR_CL_DDR_52_360]) &&
		(card->ext_csd.raw_pwr_cl_ddr_200_360 ==
			bw_ext_csd[EXT_CSD_PWR_CL_DDR_200_360]));

	if (err)
		err = -EINVAL;

	kfree(bw_ext_csd);
	return err;
}

MMC_DEV_ATTR(cid, "%08x%08x%08x%08x\n", card->raw_cid[0], card->raw_cid[1],
	card->raw_cid[2], card->raw_cid[3]);
MMC_DEV_ATTR(csd, "%08x%08x%08x%08x\n", card->raw_csd[0], card->raw_csd[1],
	card->raw_csd[2], card->raw_csd[3]);
MMC_DEV_ATTR(date, "%02d/%04d\n", card->cid.month, card->cid.year);
MMC_DEV_ATTR(erase_size, "%u\n", card->erase_size << 9);
MMC_DEV_ATTR(preferred_erase_size, "%u\n", card->pref_erase << 9);
MMC_DEV_ATTR(ffu_capable, "%d\n", card->ext_csd.ffu_capable);
MMC_DEV_ATTR(hwrev, "0x%x\n", card->cid.hwrev);
MMC_DEV_ATTR(manfid, "0x%06x\n", card->cid.manfid);
MMC_DEV_ATTR(name, "%s\n", card->cid.prod_name);
MMC_DEV_ATTR(oemid, "0x%04x\n", card->cid.oemid);
MMC_DEV_ATTR(prv, "0x%x\n", card->cid.prv);
MMC_DEV_ATTR(serial, "0x%08x\n", card->cid.serial);
MMC_DEV_ATTR(enhanced_area_offset, "%llu\n",
		card->ext_csd.enhanced_area_offset);
MMC_DEV_ATTR(enhanced_area_size, "%u\n", card->ext_csd.enhanced_area_size);
MMC_DEV_ATTR(raw_rpmb_size_mult, "%#x\n", card->ext_csd.raw_rpmb_size_mult);
MMC_DEV_ATTR(rel_sectors, "%#x\n", card->ext_csd.rel_sectors);
MMC_DEV_ATTR(pre_eol_info, "0x%02X\n", card->ext_csd.pre_eol_info);
MMC_DEV_ATTR(life_time_est_typ_a, "0x%02X\n", card->ext_csd.device_life_time_est_typ_a);
MMC_DEV_ATTR(life_time_est_typ_b, "0x%02X\n", card->ext_csd.device_life_time_est_typ_b);

static ssize_t mmc_fwrev_show(struct device *dev,
                             struct device_attribute *attr,
                             char *buf)
{
    struct mmc_card *card = mmc_dev_to_card(dev);

    if (card->ext_csd.rev < 7) {
        return sprintf(buf, "0x%x\n", card->cid.fwrev);
    } else {
        return sprintf(buf, "0x%*phN\n", MMC_FIRMWARE_LEN,
                        card->ext_csd.fwrev);
    }
}

static DEVICE_ATTR(fwrev, S_IRUGO, mmc_fwrev_show, NULL);


static struct attribute *mmc_std_attrs[] = {
	&dev_attr_cid.attr,
	&dev_attr_csd.attr,
	&dev_attr_date.attr,
	&dev_attr_erase_size.attr,
	&dev_attr_preferred_erase_size.attr,
	&dev_attr_fwrev.attr,
	&dev_attr_ffu_capable.attr,
	&dev_attr_hwrev.attr,
	&dev_attr_manfid.attr,
	&dev_attr_name.attr,
	&dev_attr_oemid.attr,
	&dev_attr_prv.attr,
	&dev_attr_serial.attr,
	&dev_attr_enhanced_area_offset.attr,
	&dev_attr_enhanced_area_size.attr,
	&dev_attr_raw_rpmb_size_mult.attr,
	&dev_attr_rel_sectors.attr,
	&dev_attr_pre_eol_info.attr,
	&dev_attr_life_time_est_typ_a.attr,
	&dev_attr_life_time_est_typ_b.attr,

	NULL,
};

static struct attribute_group mmc_std_attr_group = {
	.attrs = mmc_std_attrs,
};

static const struct attribute_group *mmc_attr_groups[] = {
	&mmc_std_attr_group,
	NULL,
};

static struct device_type mmc_type = {
	.groups = mmc_attr_groups,
};

/*
 * Select the PowerClass for the current bus width
 * If power class is defined for 4/8 bit bus in the
 * extended CSD register, select it by executing the
 * mmc_switch command.
 */
static int __mmc_select_powerclass(struct mmc_card *card,
				   unsigned int bus_width)
{
	struct mmc_host *host = card->host;
	struct mmc_ext_csd *ext_csd = &card->ext_csd;
	unsigned int pwrclass_val = 0;
	int err = 0;

	switch (1 << host->ios.vdd) {
	case MMC_VDD_165_195:
		if (host->ios.clock <= MMC_HIGH_26_MAX_DTR)
			pwrclass_val = ext_csd->raw_pwr_cl_26_195;
		else if	(host->ios.clock <= MMC_HIGH_52_MAX_DTR)
            pwrclass_val = (bus_width <= EXT_CSD_BUS_WIDTH_8) ?
            ext_csd->raw_pwr_cl_52_195 :
            ext_csd->raw_pwr_cl_ddr_52_195;
		else if (host->ios.clock <= MMC_HS200_MAX_DTR)
			pwrclass_val = ext_csd->raw_pwr_cl_200_195;
		break;
	case MMC_VDD_27_28:
	case MMC_VDD_28_29:
	case MMC_VDD_29_30:
	case MMC_VDD_30_31:
	case MMC_VDD_31_32:
	case MMC_VDD_32_33:
	case MMC_VDD_33_34:
	case MMC_VDD_34_35:
	case MMC_VDD_35_36:
		if (host->ios.clock <= MMC_HIGH_26_MAX_DTR)
			pwrclass_val = ext_csd->raw_pwr_cl_26_360;
		else if	(host->ios.clock <= MMC_HIGH_52_MAX_DTR)
            pwrclass_val = (bus_width <= EXT_CSD_BUS_WIDTH_8) ?
            ext_csd->raw_pwr_cl_52_360 :
            ext_csd->raw_pwr_cl_ddr_52_360;
		else if (host->ios.clock <= MMC_HS200_MAX_DTR)
			pwrclass_val = (bus_width == EXT_CSD_DDR_BUS_WIDTH_8) ?
				ext_csd->raw_pwr_cl_ddr_200_360 :
				ext_csd->raw_pwr_cl_200_360;
		break;
	default:
		pr_warning("%s: Voltage range not supported "
			   "for power class.\n", mmc_hostname(host));
		return -EINVAL;
	}

	if (bus_width & (EXT_CSD_BUS_WIDTH_8 | EXT_CSD_DDR_BUS_WIDTH_8))
		pwrclass_val = (pwrclass_val & EXT_CSD_PWR_CL_8BIT_MASK) >>
				EXT_CSD_PWR_CL_8BIT_SHIFT;
	else
		pwrclass_val = (pwrclass_val & EXT_CSD_PWR_CL_4BIT_MASK) >>
				EXT_CSD_PWR_CL_4BIT_SHIFT;

	/* If the power class is different from the default value */
	if (pwrclass_val > 0) {
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				 EXT_CSD_POWER_CLASS,
				 pwrclass_val,
				 ext_csd->generic_cmd6_time);
	}

	return err;
}

static int mmc_select_powerclass(struct mmc_card *card)
{
    struct mmc_host *host = card->host;
    u32 bus_width, ext_csd_bits;
    int err, ddr;

    /* Power class selection is supported for versions >= 4.0 */
    if (!mmc_can_ext_csd(card))
        return 0;

    bus_width = host->ios.bus_width;
    /* Power class values are defined only for 4/8 bit bus */
    if (bus_width == MMC_BUS_WIDTH_1)
        return 0;

    ddr = card->mmc_avail_type & (EXT_CSD_CARD_TYPE_DDR_52 | EXT_CSD_CARD_TYPE_HS400);
    if (ddr)
        ext_csd_bits = (bus_width == MMC_BUS_WIDTH_8) ?
            EXT_CSD_DDR_BUS_WIDTH_8 : EXT_CSD_DDR_BUS_WIDTH_4;
    else
        ext_csd_bits = (bus_width == MMC_BUS_WIDTH_8) ?
            EXT_CSD_BUS_WIDTH_8 :  EXT_CSD_BUS_WIDTH_4;

    err = __mmc_select_powerclass(card, ext_csd_bits);
    if (err)
        pr_warn("%s: power class selection to bus width %d ddr %d failed\n",
            mmc_hostname(host), 1 << bus_width, ddr);

    return err;
}


/*
 * Revert to the high-speed mode from above speed
*/
static int mmc_revert_to_hs(struct mmc_card *card)
{
	int err = 0;
	struct mmc_host *host = card->host;

	BUG_ON(!card);

	/*
	 * CMD13, which is used to confirm the completion of timing
	 * change, will be issued at higher speed timing condtion
	 * rather than high-speed. If device has completed the change
	 * to high-speed mode, it may not be proper timing to issue
	 * command. Low speed supplies better timing margin than high
	 * speed. Accordingly clock rate & timging should be chagned
	 * ahead before actual switch.
	 */
	mmc_set_timing(card->host, MMC_TIMING_MMC_HS);
	mmc_set_clock(host, MMC_HIGH_52_MAX_DTR);

	err = __mmc_set_signal_voltage(host, MMC_SIGNAL_VOLTAGE_330);
	/* If fails try again during next card power cycle */
	if (err)
		return err;

	return __mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
			  EXT_CSD_HS_TIMING, EXT_CSD_TIMING_HS,
			  card->ext_csd.generic_cmd6_time,
			  true,true,true);
}

/*
 * Set the bus speed for the selected speed mode.
 */
static void mmc_set_bus_speed(struct mmc_card *card)
{
    unsigned int max_dtr = (unsigned int)-1;

    if ((mmc_card_hs200(card) || mmc_card_hs400(card)) &&
        max_dtr > card->ext_csd.hs200_max_dtr)
        max_dtr = card->ext_csd.hs200_max_dtr;
    else if (mmc_card_hs(card) && max_dtr > card->ext_csd.hs_max_dtr)
        max_dtr = card->ext_csd.hs_max_dtr;
    else if (max_dtr > card->csd.max_dtr)
        max_dtr = card->csd.max_dtr;

    mmc_set_clock(card->host, max_dtr);
}
/*
 * Select the bus width amoung 4-bit and 8-bit(SDR).
 * If the bus width is changed successfully, return the selected width value.
 * Zero is returned instead of error value if the wide width is not supported.
 */
static int mmc_select_bus_width(struct mmc_card *card)
{
	static unsigned ext_csd_bits[] = {
		EXT_CSD_BUS_WIDTH_8,
		EXT_CSD_BUS_WIDTH_4,
	};
	static unsigned bus_widths[] = {
		MMC_BUS_WIDTH_8,
		MMC_BUS_WIDTH_4,
	};
    struct mmc_host *host = card->host;
    unsigned idx, bus_width = 0;
    int err = 0;

    if (!mmc_can_ext_csd(card) &&
        !(host->caps & (MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA)))
        return 0;

	idx = (host->caps & MMC_CAP_8_BIT_DATA) ? 0 : 1;

	/*
	 * Unlike SD, MMC cards dont have a configuration register to notify
	 * supported bus width. So bus test command should be run to identify
	 * the supported bus width or compare the ext csd values of current
	 * bus width and ext csd values of 1 bit mode read earlier.
	 */
	for (; idx < ARRAY_SIZE(bus_widths); idx++) {
		/*
		 * Host is capable of 8bit transfer, then switch
		 * the device to work in 8bit transfer mode. If the
		 * mmc switch command returns error then switch to
		 * 4bit transfer mode. On success set the corresponding
		 * bus width on the host.
		 */
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				 EXT_CSD_BUS_WIDTH,
				 ext_csd_bits[idx],
				 card->ext_csd.generic_cmd6_time);
		if (err)
			continue;

        bus_width = bus_widths[idx];
        mmc_set_bus_width(host, bus_width);
        /*
         * If controller can't handle bus width test,
         * compare ext_csd previously read in 1 bit mode
         * against ext_csd at new bus width
         */
		if (!(host->caps & MMC_CAP_BUS_WIDTH_TEST))
			err = mmc_compare_ext_csds(card, bus_width);
		else
			err = mmc_bus_test(card, bus_width);
		if (!err) {
		    err = bus_width;
			break;
	    } else {
            pr_warn("%s: switch to bus width %d failed\n",
            mmc_hostname(host), ext_csd_bits[idx]);
	    }
	}

	return err;
}
static int mmc_select_hs(struct mmc_card *card)
{
    int err;

    err = __mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
        EXT_CSD_HS_TIMING, EXT_CSD_TIMING_HS,
        card->ext_csd.generic_cmd6_time,
        true, true, true);
	if (!err)
	    mmc_set_timing(card->host, MMC_TIMING_MMC_HS);
	return err;
}

/*���ٿ����趨Ƶ��25M  ���ÿ�λ��*/
static void mmc_select_new_sd(struct mmc_card *card)
{
        unsigned int max_dtr;
        
        mmc_set_timing(card->host, MMC_TIMING_NEW_SD);
        max_dtr = (unsigned int)-1;
        if (max_dtr > card->csd.max_dtr) {
        max_dtr = card->csd.max_dtr;
        }
        mmc_set_clock(card->host, max_dtr);
}


/*
 * Activate wide bus and DDR if supported.
 */
static int mmc_select_hs_ddr(struct mmc_card *card)
{
    struct mmc_host *host = card->host;
    u32 bus_width, ext_csd_bits;
    int err = 0;

    if (!(card->mmc_avail_type & EXT_CSD_CARD_TYPE_DDR_52))
        return 0;

    bus_width = host->ios.bus_width;
    if (bus_width == MMC_BUS_WIDTH_1)
        return 0;

    ext_csd_bits = (bus_width == MMC_BUS_WIDTH_8) ?
        EXT_CSD_DDR_BUS_WIDTH_8 : EXT_CSD_DDR_BUS_WIDTH_4;

    err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
            EXT_CSD_BUS_WIDTH,
            ext_csd_bits,
            card->ext_csd.generic_cmd6_time);
    if (err) {
        pr_warn("%s: switch to bus width %d ddr failed\n",
            mmc_hostname(host), 1 << bus_width);
        return err;
    }

    /*
    * eMMC cards can support 3.3V to 1.2V i/o (vccq)
    * signaling.
    *
    * EXT_CSD_CARD_TYPE_DDR_1_8V means 3.3V or 1.8V vccq.
    *
    * 1.8V vccq at 3.3V core voltage (vcc) is not required
    * in the JEDEC spec for DDR.
    *
    * Even (e)MMC card can support 3.3v to 1.2v vccq, but not all
    * host controller can support this, like some of the SDHCI
    * controller which connect to an eMMC device. Some of these
    * host controller still needs to use 1.8v vccq for supporting
    * DDR mode.
    *
    * So the sequence will be:
    * if (host and device can both support 1.2v IO)
    *      use 1.2v IO;
    * else if (host and device can both support 1.8v IO)
    *      use 1.8v IO;
    * so if host and device can only support 3.3v IO, this is the
    * last choice.
    *
    * WARNING: eMMC rules are NOT the same as SD DDR
    */
    err = -EINVAL;
    if (card->mmc_avail_type & EXT_CSD_CARD_TYPE_DDR_1_2V)
        err = __mmc_set_signal_voltage(host, MMC_SIGNAL_VOLTAGE_120);

    if (err && (card->mmc_avail_type & EXT_CSD_CARD_TYPE_DDR_1_8V))
        err = __mmc_set_signal_voltage(host, MMC_SIGNAL_VOLTAGE_180);

    /* make sure vccq is 3.3v after switching disaster */
    if (err)
        err = __mmc_set_signal_voltage(host, MMC_SIGNAL_VOLTAGE_330);

    if (!err)
        mmc_set_timing(host, MMC_TIMING_MMC_DDR52);

    return err;
}

/*
 * For device supporting HS200 mode, the following sequence
 * should be done before executing the tuning process.
 * 1. set the desired bus width(4-bit or 8-bit, 1-bit is not supported)
 * 2. switch to HS200 mode
 * 3. set the clock to > 52Mhz and <=200MHz
 */
static int mmc_select_hs200(struct mmc_card *card)
{
   struct mmc_host *host = card->host;
   int err = -EINVAL;
   unsigned char timing_value = 0;

   /*
    * Set the bus width(4 or 8) with host's support and
    * switch to HS200 mode if bus width is set successfully.
    */
   err = mmc_select_bus_width(card);
   if (!IS_ERR_VALUE(err)) {
        timing_value = EXT_CSD_TIMING_HS200;
        if ((host->caps & MMC_CAP_DRIVER_TYPE_A)
        	&& (card->ext_csd.raw_driver_strength & (1 << EXT_CSD_DRVIER_STRENGTH_50)))
        {
        	timing_value |= (EXT_CSD_DRVIER_STRENGTH_50 << EXT_CSD_DRVIER_STRENGTH_SHIFT);
        }
    	err = __mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
			EXT_CSD_HS_TIMING, timing_value,
    			 card->ext_csd.generic_cmd6_time,
    			 true, true, true);
        if (!err)
            mmc_set_timing(host, MMC_TIMING_MMC_HS200);

        if (card->mmc_avail_type & EXT_CSD_CARD_TYPE_HS200_1_2V)
            err = __mmc_set_signal_voltage(host, MMC_SIGNAL_VOLTAGE_120);

        if (card->mmc_avail_type & EXT_CSD_CARD_TYPE_HS200_1_8V)
            err = __mmc_set_signal_voltage(host, MMC_SIGNAL_VOLTAGE_180);
    }

	return err;
}

static int mmc_select_hs400(struct mmc_card *card)
{
	struct mmc_host *host = card->host;
	int err = 0;
	unsigned char timing_value = 0;
	unsigned char bus_width = 0;

	pr_debug("%s: mmc_avail_type=%x, bus_width=%d\n", __func__, card->mmc_avail_type, host->ios.bus_width);

	/*
	 * The bus width is set to only 8 DDR in HS400 mode
	 */
	if (!(card->mmc_avail_type & EXT_CSD_CARD_TYPE_HS400 &&
	      host->ios.bus_width == MMC_BUS_WIDTH_8))
		return 0;
	/*
	 * Before setting BUS_WIDTH for dual data rate operation,
	 * HS_TIMING must be set to High Speed(0x1)
	 */
	err = mmc_revert_to_hs(card);
	if (err) {
		pr_warn("%s: switch to high-speed from hs200 failed, err:%d\n",
			mmc_hostname(host), err);
		return err;
	}

	bus_width = EXT_CSD_DDR_BUS_WIDTH_8;
	if (card->ext_csd.strobe_enhanced_en && host->ops->enable_enhanced_strobe) {
		bus_width |= EXT_CSD_STROBE_ENHANCED;
	}
	err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
			 EXT_CSD_BUS_WIDTH,
			 bus_width,
			 card->ext_csd.generic_cmd6_time);
	if (err) {
		pr_warn("%s: switch bus width or enhanced strobe for hs400 failed, err:%d\n",
			mmc_hostname(host), err);
		return err;
	}

	timing_value = EXT_CSD_TIMING_HS400;
	if (card->ext_csd.raw_driver_strength & (1 << EXT_CSD_DRVIER_STRENGTH_40))
	{
		timing_value |= (EXT_CSD_DRVIER_STRENGTH_40 << EXT_CSD_DRVIER_STRENGTH_SHIFT);
	}
	err = __mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
			 EXT_CSD_HS_TIMING, timing_value,
			 card->ext_csd.generic_cmd6_time,
			 true,true,true);
	if (err) {
		pr_warn("%s: switch to hs400 failed, err:%d\n",
			 mmc_hostname(host), err);
		return err;
	}
	mmc_set_timing(host, MMC_TIMING_MMC_HS400);
	mmc_set_bus_speed(card);

	if (card->mmc_avail_type & EXT_CSD_CARD_TYPE_HS400_1_2V)
		err = __mmc_set_signal_voltage(host, MMC_SIGNAL_VOLTAGE_120);
	if (card->mmc_avail_type & EXT_CSD_CARD_TYPE_HS400_1_8V)
		err = __mmc_set_signal_voltage(host, MMC_SIGNAL_VOLTAGE_180);
	/* If fails try again during next card power cycle */
	if (err)
		return err;

	if (card->ext_csd.strobe_enhanced_en && host->ops->enable_enhanced_strobe) {
		err = host->ops->enable_enhanced_strobe(host);
		if (err)
			return err;
	}

	pr_debug("%s: done!\n", __func__);
	return 0;
}
/*
 * Activate High Speed or HS200 mode if supported.
 */
static int mmc_select_timing(struct mmc_card *card)
{
    int err = 0;

    if (!mmc_can_ext_csd(card))
        goto bus_speed;

    if (card->mmc_avail_type & EXT_CSD_CARD_TYPE_HS200)
        err = mmc_select_hs200(card);
    else if (card->mmc_avail_type & EXT_CSD_CARD_TYPE_HS)
        err = mmc_select_hs(card);
    else if(0 == card->mmc_avail_type)
        mmc_select_new_sd(card);

    if (err && err != -EBADMSG)
        return err;

    if (err) {
        pr_warn("%s: switch to %s failed\n",
            mmc_card_hs(card) ? "high-speed" :
            (mmc_card_hs200(card) ? "hs200" : ""),
            mmc_hostname(card->host));
        err = 0;
    }

bus_speed:
    /*
     * Set the bus speed to the selected bus timing.
     * If timing is not selected, backward compatible is the default.
     */
    mmc_set_bus_speed(card);
    return err;
}

/*
 * Execute tuning sequence to seek the proper bus operating
 * conditions for HS200 and HS400, which sends CMD21 to the device.
 */
static int mmc_hs200_tuning(struct mmc_card *card)
{
    struct mmc_host *host = card->host;
    int err = 0;

    /*
     * Timing should be adjusted to the HS400 target
     * operation frequency for tuning process
     */
    if (card->mmc_avail_type & EXT_CSD_CARD_TYPE_HS400 &&
        host->ios.bus_width == MMC_BUS_WIDTH_8)
    if (host->ops->prepare_hs400_tuning)
        host->ops->prepare_hs400_tuning(host, &host->ios);

    if (host->ops->execute_tuning) {
        mmc_host_clk_hold(host);
        err = host->ops->execute_tuning(host,
            MMC_SEND_TUNING_BLOCK_HS200);
        mmc_host_clk_release(host);

        if (err)
            pr_warn("%s: tuning execution failed\n",
                mmc_hostname(host));
    }

    return err;
}

/*
 * Handle the detection and initialisation of a card.
 *
 * In the case of a resume, "oldcard" will contain the card
 * we're trying to reinitialise.
 */
static int mmc_init_card(struct mmc_host *host, u32 ocr,
	struct mmc_card *oldcard)
{
	struct mmc_card *card;
	int err;
	u32 cid[4];
	u32 rocr;

	BUG_ON(!host);
	WARN_ON(!host->claimed);

	/* Set correct bus mode for MMC before attempting init */
	if (!mmc_host_is_spi(host))
		mmc_set_bus_mode(host, MMC_BUSMODE_OPENDRAIN);

	/*
	 * Since we're changing the OCR value, we seem to
	 * need to tell some cards to go back to the idle
	 * state.  We wait 1ms to give cards time to
	 * respond.
	 * mmc_go_idle is needed for eMMC that are asleep
	 */


	printk(KERN_INFO "mmc init begin cmd0,caps is 0x%x \n",host->caps);

	mmc_go_idle(host);

	printk(KERN_INFO "mmc init processing cmd1 MMC_SEND_OP_COND\n");
	/* The extra bit indicates that we support high capacity */
	err = mmc_send_op_cond(host, ocr | (1 << 30), &rocr);
	if (err)
		goto err;

	/*
	 * For SPI, enable CRC as appropriate.
	 */
	if (mmc_host_is_spi(host)) {
		err = mmc_spi_set_crc(host, use_spi_crc);
		if (err)
			goto err;
	}

	printk(KERN_INFO "mmc init processing cmd2 MMC_ALL_SEND_CID\n");
	/*
	 * Fetch CID from card.
	 */
	if (mmc_host_is_spi(host))
		err = mmc_send_cid(host, cid);
	else
		err = mmc_all_send_cid(host, cid);
	if (err)
		goto err;

	if (oldcard) {
		if (memcmp(cid, oldcard->raw_cid, sizeof(cid)) != 0) {
			err = -ENOENT;
			goto err;
		}

		card = oldcard;
	} else {
		/*
		 * Allocate card structure.
		 */
		card = mmc_alloc_card(host, &mmc_type);
		if (IS_ERR(card)) {
			err = PTR_ERR(card);
			goto err;
		}

		card->ocr = ocr;
		card->type = MMC_TYPE_MMC;
		card->rca = 1;
		memcpy(card->raw_cid, cid, sizeof(card->raw_cid));
	}

	printk(KERN_INFO "mmc init processing cmd3 MMC_SET_RELATIVE_ADDR\n");
	/*
	 * For native busses:  set card RCA and quit open drain mode.
	 */
	if (!mmc_host_is_spi(host)) {
		err = mmc_set_relative_addr(card);
		if (err)
			goto free_card;

		mmc_set_bus_mode(host, MMC_BUSMODE_PUSHPULL);
	}

	if (!oldcard) {
		/*
		 * Fetch CSD from card.
		 */
		err = mmc_send_csd(card, card->raw_csd);
		if (err)
			goto free_card;

		err = mmc_decode_csd(card);
		if (err)
			goto free_card;
		err = mmc_decode_cid(card);
		if (err)
			goto free_card;
	}

	/*
	 * handling only for cards supporting DSR and hosts requesting
	 * DSR configuration
	 */
	if (card->csd.dsr_imp && host->dsr_req)
		mmc_set_dsr(host);

	printk(KERN_INFO "mmc init processing cmd7 MMC_SELECT_CARD\n");
	/*
	 * Select card, as all following commands rely on that.
	 */
	if (!mmc_host_is_spi(host)) {
		err = mmc_select_card(card);
		if (err)
			goto free_card;
	}

	if (!oldcard) {
		err = mmc_read_ext_csd(card);
		if (err)
			goto free_card;

		/* If doing byte addressing, check if required to do sector
		 * addressing.  Handle the case of <2GB cards needing sector
		 * addressing.  See section 8.1 JEDEC Standard JED84-A441;
		 * ocr register has bit 30 set for sector addressing.
		 */
		if (!(mmc_card_blockaddr(card)) && (rocr & (1<<30)))
			mmc_card_set_blockaddr(card);

		/* Erase size depends on CSD and Extended CSD */
		mmc_set_erase_size(card);
	}

        /*
	 * If enhanced_area_en is TRUE, host needs to enable ERASE_GRP_DEF
	 * bit.  This bit will be lost every time after a reset or power off.
	 */
	/*balong not support,bypass*/
	if (card->ext_csd.partition_setting_completed ||
	    (card->ext_csd.rev >= 3 && (host->caps2 & MMC_CAP2_HC_ERASE_SZ))) {
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				 EXT_CSD_ERASE_GROUP_DEF, 1,
				 card->ext_csd.generic_cmd6_time);

		if (err && err != -EBADMSG)
			goto free_card;

		if (err) {
			err = 0;
			/*
			 * Just disable enhanced area off & sz
			 * will try to enable ERASE_GROUP_DEF
			 * during next time reinit
			 */
			card->ext_csd.enhanced_area_offset = -EINVAL;
			card->ext_csd.enhanced_area_size = -EINVAL;
		} else {
			card->ext_csd.erase_group_def = 1;
			/*
			 * enable ERASE_GRP_DEF successfully.
			 * This will affect the erase size, so
			 * here need to reset erase size
			 */
			mmc_set_erase_size(card);
		}
	}

	/*
	 * Ensure eMMC user default partition is enabled
	 */
	if (card->ext_csd.part_config & EXT_CSD_PART_CONFIG_ACC_MASK) {
		card->ext_csd.part_config &= ~EXT_CSD_PART_CONFIG_ACC_MASK;
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_PART_CONFIG,
				 card->ext_csd.part_config,
				 card->ext_csd.part_time);
		if (err && err != -EBADMSG)
			goto free_card;
	}

	/*
	 * Enable power_off_notification byte in the ext_csd register
	 */
	if (card->ext_csd.rev >= 6) {
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				 EXT_CSD_POWER_OFF_NOTIFICATION,
				 EXT_CSD_POWER_ON,
				 card->ext_csd.generic_cmd6_time);
		if (err && err != -EBADMSG)
			goto free_card;

		/*
		 * The err can be -EBADMSG or 0,
		 * so check for success and update the flag
		 */
		if (!err)
			card->ext_csd.power_off_notification = EXT_CSD_POWER_ON;
	}

	/*
	 * Enable Enhanced strobe (if supported)
	 */
	if (card->ext_csd.strobe_enhanced) {
		if (host->caps2 & MMC_CAP2_ENHANCED_STROBE) {
			card->ext_csd.strobe_enhanced_en = 1;
			pr_err("%s: strobe_enhanced_en=%d\n", __func__, card->ext_csd.strobe_enhanced_en);
		} else {
			card->ext_csd.strobe_enhanced_en = 0;
		}
	}

	/*
	 * Select timing interface
	 */
	err = mmc_select_timing(card);
	if (err)
		goto free_card;

	if (mmc_card_hs200(card)) {
		err = mmc_hs200_tuning(card);
		if (err)
			goto err;

		err = mmc_select_hs400(card);
		if (err){
			pr_warning("%s: mmc_select_hs400 execution"
				   " failed\n", mmc_hostname(card->host));
			goto err;
		}
	} else if(mmc_card_hs(card)) {
        /* Select the desired bus width optionally */
        err = mmc_select_bus_width(card);
        if (!IS_ERR_VALUE(err)) {
            err = mmc_select_hs_ddr(card);
                if (err)
            goto err;
		}
	}else{
		mmc_select_bus_width(card);
      }   
      
	/*
	 * Choose the power class with selected bus interface
	 */
	mmc_select_powerclass(card);

	/*
	 * Enable HPI feature (if supported)
	 */
	if (card->ext_csd.hpi) {
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				EXT_CSD_HPI_MGMT, 1,
				card->ext_csd.generic_cmd6_time);
		if (err && err != -EBADMSG)
			goto free_card;
		if (err) {
			pr_warning("%s: Enabling HPI failed\n",
				   mmc_hostname(card->host));
			err = 0;
		} else
			card->ext_csd.hpi_en = 1;
		pr_debug("%s: hpi_en=%d\n", __func__, card->ext_csd.hpi_en);
	}

	/*
	 * If cache size is higher than 0, this indicates
	 * the existence of cache and it can be turned on.
	 */
	if ((host->caps2 & MMC_CAP2_CACHE_CTRL) &&
			card->ext_csd.cache_size > 0) {
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				EXT_CSD_CACHE_CTRL, 1,
				card->ext_csd.generic_cmd6_time);
		if (err && err != -EBADMSG)
			goto free_card;

		/*
		 * Only if no error, cache is turned on successfully.
		 */
		if (err) {
			pr_warning("%s: Cache is supported, "
					"but failed to turn on (%d)\n",
					mmc_hostname(card->host), err);
			card->ext_csd.cache_ctrl = 0;
			err = 0;
		} else {
			card->ext_csd.cache_ctrl = 1;
		}
		pr_debug("%s: cache_ctrl=%d\n", __func__, card->ext_csd.cache_ctrl);
	}

	/*
	 * The mandatory minimum values are defined for packed command.
	 * read: 5, write: 3
	 */
	if (card->ext_csd.max_packed_writes >= 3 &&
	    card->ext_csd.max_packed_reads >= 5 &&
	    host->caps2 & MMC_CAP2_PACKED_CMD) {
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				EXT_CSD_EXP_EVENTS_CTRL,
				EXT_CSD_PACKED_EVENT_EN,
				card->ext_csd.generic_cmd6_time);
		if (err && err != -EBADMSG)
			goto free_card;
		if (err) {
			pr_warn("%s: Enabling packed event failed\n",
				mmc_hostname(card->host));
			card->ext_csd.packed_event_en = 0;
			err = 0;
		} else {
			card->ext_csd.packed_event_en = 1;
		}
		pr_debug("%s: packed_event_en=%d, max_packed_writes=%d, max_packed_reads=%d\n", __func__,
			card->ext_csd.packed_event_en, card->ext_csd.max_packed_writes, card->ext_csd.max_packed_reads);
	}

	/*
	 * Enable command queue (if supported)
	 */
	if (card->ext_csd.cmdq_support) {
		if (host->caps2 & MMC_CAP2_CMD_QUEUE) {
			card->ext_csd.cmdq_mode_en = 1;
			pr_err("%s: cmdq_mode_en=%d\n", __func__, card->ext_csd.cmdq_mode_en);
		} else {
			card->ext_csd.cmdq_mode_en = 0;
		}
	}

	/*
	 * Enable Barrier feature (if supported)
	 */
	if (card->ext_csd.cache_flush_barrier && (host->caps2 & MMC_CAP2_CACHE_FLUSH_BARRIER)) {
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				EXT_CSD_BARRIER_CTRL, 1,
				card->ext_csd.generic_cmd6_time);
		if (err && err != -EBADMSG)
			goto free_card;
		if (err) {
			pr_warning("%s: Enabling cache flush barrier failed\n",
				   mmc_hostname(card->host));
			err = 0;
			card->ext_csd.cache_flush_barrier_en = 0;
		} else {
			card->ext_csd.cache_flush_barrier_en = 1;
			pr_err("%s: cache_flush_barrier_en=%d\n", __func__, card->ext_csd.cache_flush_barrier_en);
		}
	}

	/*
	 * Enable BKOPS AUTO  feature (if supported)
	 */
	if ((host->caps2 & MMC_CAP2_BKOPS_AUTO_CTRL) && (host->pm_flags & MMC_PM_KEEP_POWER) && card->ext_csd.bkops && (card->ext_csd.rev >= 8)) {
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
				EXT_CSD_BKOPS_EN, EXT_CSD_BKOPS_AUTO_EN,
				card->ext_csd.generic_cmd6_time);
		if (err && err != -EBADMSG)
			goto free_card;
		if (err) {
			pr_warning("%s: BKOPS_AUTO_EN failed\n",
				   mmc_hostname(card->host));
			err = 0;
			card->ext_csd.bkops_auto_en = 0;
		} else {
			card->ext_csd.bkops_auto_en = 1;
			pr_err("%s: support BKOPS_AUTO_EN, bkops_auto_en=%d\n", __func__, card->ext_csd.bkops_auto_en);
		}
	}

	if (!oldcard)
		host->card = card;

	pr_info("%s: eMMC init card done, support bus_width=%d bit.\n", mmc_hostname(card->host), (0x1 << card->host->ios.bus_width));

#ifdef CONFIG_HUAWEI_EMMC_DSM
	/* eMMC v5.0 or later */
	if(!strcmp(mmc_hostname(card->host), "mmc0")){
		err = mmc_get_ext_csd(card, &card->cached_ext_csd);
		if(err) {
			pr_warn("%s: get ext csd failed err=%d \n",
				mmc_hostname(card->host),err);
			err = 0;
		}
		if (card->ext_csd.rev >= 7) {
			err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_EXP_EVENTS_CTRL,
					EXT_CSD_DYNCAP_EVENT_EN,
					card->ext_csd.generic_cmd6_time);
			if (err) {
				pr_warn("%s: Enabling dyncap and syspool event failed\n",
					mmc_hostname(card->host));
				err = 0;
			}

			if( card->ext_csd.device_life_time_est_typ_a >= DEVICE_LIFE_TRIGGER_LEVEL ||
				card->ext_csd.device_life_time_est_typ_b >= DEVICE_LIFE_TRIGGER_LEVEL)
			{
					DSM_EMMC_LOG(card, DSM_EMMC_LIFE_TIME_EST_ERR,
						"%s:eMMC life time has problem, device_life_time_est_typ_a[268]:%d, device_life_time_est_typ_b{269]:%d\n",
						__FUNCTION__, card->ext_csd.device_life_time_est_typ_a, card->ext_csd.device_life_time_est_typ_b);
			}

			if(card->ext_csd.pre_eol_info == EXT_CSD_PRE_EOL_INFO_WARNING ||
				card->ext_csd.pre_eol_info == EXT_CSD_PRE_EOL_INFO_URGENT)
			{
				DSM_EMMC_LOG(card, DSM_EMMC_PRE_EOL_INFO_ERR,
					"%s:eMMC average reserved blocks has problem, PRE_EOL_INFO[267]:%d\n", __FUNCTION__,
					card->ext_csd.pre_eol_info);
			}
		}
		if(NULL != card->cached_ext_csd)
		{
		    kfree(card->cached_ext_csd);
		    card->cached_ext_csd = NULL;
		}

	}
#endif
	return 0;

free_card:
	if (!oldcard)
		mmc_remove_card(card);
err:
	return err;
}

static int mmc_can_sleep_notify(const struct mmc_card *card)
{
	return card && mmc_card_mmc(card) && card->ext_csd.raw_sleep_noti_time;
}

static int mmc_can_sleep(struct mmc_card *card)
{
	return (card && card->ext_csd.rev >= 3);
}

int mmc_can_poweroff_notify(const struct mmc_card *card)
{
	return card &&
		mmc_card_mmc(card) &&
		(card->ext_csd.power_off_notification == EXT_CSD_POWER_ON);
}
EXPORT_SYMBOL(mmc_can_poweroff_notify);

int mmc_poweroff_notify(struct mmc_card *card, unsigned int notify_type)
{
	unsigned int timeout = card->ext_csd.generic_cmd6_time;
	int err;

	/* Use EXT_CSD_POWER_OFF_SHORT as default notification type. */
	if (notify_type == EXT_CSD_POWER_OFF_LONG)
		timeout = card->ext_csd.power_off_longtime;
	else if (notify_type == EXT_CSD_SLEEP_NOTIFICATION)
		timeout = card->ext_csd.sleep_notification_time;

	err = __mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
		EXT_CSD_POWER_OFF_NOTIFICATION,
		notify_type, timeout, true, false,false);
	if (err)
		pr_err("%s: Power Off Notification timed out, %u\n",
		       mmc_hostname(card->host), timeout);

	/* Disable the power off notification after the switch operation. */
	card->ext_csd.power_off_notification = EXT_CSD_NO_POWER_NOTIFICATION;

	return err;
}
EXPORT_SYMBOL(mmc_poweroff_notify);
/*
 * Host is being removed. Free up the current card.
 */
static void mmc_remove(struct mmc_host *host)
{
	BUG_ON(!host);
	BUG_ON(!host->card);

	mmc_remove_card(host->card);
	host->card = NULL;
}

/*
 * Card detection - card is alive.
 */
static int mmc_alive(struct mmc_host *host)
{
	return mmc_send_status(host->card, NULL);
}

/*
 * Card detection callback from host.
 */
static void mmc_detect(struct mmc_host *host)
{
	int err;

	BUG_ON(!host);
	BUG_ON(!host->card);

	mmc_get_card(host->card);

	/*
	 * Just check if our card has been removed.
	 */
	err = _mmc_detect_card_removed(host);

	mmc_put_card(host->card);

	if (err) {
		mmc_remove(host);

		mmc_claim_host(host);
		mmc_detach_bus(host);
		mmc_power_off(host);
		mmc_release_host(host);
	}
}

static int _mmc_suspend(struct mmc_host *host, bool is_suspend)
{
	int err = 0;
	unsigned int notify_type = is_suspend ? EXT_CSD_SLEEP_NOTIFICATION :
					EXT_CSD_POWER_OFF_LONG;
	unsigned long timeout = 8000;

	BUG_ON(!host);
	BUG_ON(!host->card);

	mmc_claim_host(host);

	if (mmc_card_suspended(host->card))
		goto out;

	if (mmc_card_doing_bkops(host->card)) {
		err = mmc_stop_bkops(host->card);
		if (err)
			goto out;
	}

	if (mmc_card_cmdq(host->card)) {
		/* wait for cmdq req handle done.*/
		while (host->cmdq_ctx.active_reqs) {
			if (timeout == 0) {
				pr_err("%s: wait cmdq complete reqs timeout !\n", __func__);
			        return -ETIMEDOUT;
			}
			timeout--;
			mdelay(1);
		}

		if (host->cmdq_ops->disable) {
			err = host->cmdq_ops->disable(host, true);
			if (err) {
				pr_err("%s: cmdq disable failed.\n", __func__);
				goto out;
			}
		}
	        err = mmc_switch(host->card, EXT_CSD_CMD_SET_NORMAL,
			EXT_CSD_CMDQ_MODE, 0,
			host->card->ext_csd.generic_cmd6_time);
		if (err) {
			pr_err("%s: set card normal failed.\n", __func__);
			goto out;
		}
		mmc_card_clr_cmdq(host->card);
	}

	err = mmc_cache_ctrl(host, 0);
	if (err) {
		pr_err("%s: cache disab failed.\n", __func__);
		goto out;
	}

	if (mmc_can_sleep(host->card)){
		if (mmc_can_poweroff_notify(host->card) &&
			((host->caps2 & MMC_CAP2_FULL_PWR_CYCLE && mmc_can_sleep_notify(host->card)) || !is_suspend)) {
			err = mmc_poweroff_notify(host->card, notify_type);
			if (err)
				goto out;
		}
		err = mmc_card_sleep(host);
	}
	else if (!mmc_host_is_spi(host)) {
		if (mmc_can_poweroff_notify(host->card))
			err = mmc_poweroff_notify(host->card, EXT_CSD_POWER_OFF_SHORT);
		else
			err = mmc_deselect_cards(host);
	}

	if (!err){
		if(!mmc_card_keep_power(host))
			mmc_power_off_vcc(host);
		mmc_card_set_suspended(host->card);
	}

out:
	mmc_release_host(host);
	return err;
}


/*
 * Suspend callback.
 */
static int mmc_suspend(struct mmc_host *host)
{
	int err;

	err = _mmc_suspend(host, true);
	if (!err) {
		pm_runtime_disable(&host->card->dev);
		pm_runtime_set_suspended(&host->card->dev);
	}

	return err;
}

/*
 * Resume callback from host.
 *
 * This function tries to determine if the same card is still present
 * and, if so, restore all state to it.
 */
static int _mmc_resume(struct mmc_host *host)
{
	int err = 0;

	BUG_ON(!host);
	BUG_ON(!host->card);

	mmc_claim_host(host);
	if (!mmc_card_suspended(host->card))
		goto out;
	if (!mmc_card_keep_power(host)) {
		mmc_power_up_vcc(host, host->card->ocr);
	}

	err = mmc_card_awake(host);/*according to K3 modify*/
	if (err) {
		mmc_power_off(host);
		mdelay(200);
		mmc_power_up(host, host->card->ocr);
		err = mmc_init_card(host, host->card->ocr, host->card);
	}
	if (!err) {
		err = mmc_cache_ctrl(host, 1);
		if (err)
			pr_err("%s: error %d cache on\n",
				mmc_hostname(host), err);
	}
	mmc_card_clr_suspended(host->card);
/*
        err = mmc_switch(host->card, EXT_CSD_CMD_SET_NORMAL,
		EXT_CSD_CMDQ_MODE, 1,
		host->card->ext_csd.generic_cmd6_time);
	if (err)
		pr_err("%s: set card CMDQ failed.\n", __func__);
	mmc_card_set_cmdq(host->card);
	if (host->cmdq_ops->enable) {
		host->cmdq_ops->enable(host);
	}
*/
out:
	mmc_release_host(host);
	return err;
}

/*
 * Shutdown callback
 */
static int mmc_shutdown(struct mmc_host *host)
{
    int err = 0;

   /*
    * In a specific case for poweroff notify, we need to resume the card
    * before we can shutdown it properly.
    */
    if (mmc_can_poweroff_notify(host->card) &&
        !(host->caps2 & MMC_CAP2_FULL_PWR_CYCLE))
            err = _mmc_resume(host);

    if (!err)
        err = _mmc_suspend(host, false);

    return err;
}

/*
 * Callback for resume.
 */
static int mmc_resume(struct mmc_host *host)
{
	int err = 0;

	if (!(host->caps & MMC_CAP_RUNTIME_RESUME)) {
		err = _mmc_resume(host);
		pm_runtime_set_active(&host->card->dev);
		pm_runtime_mark_last_busy(&host->card->dev);
	}
	pm_runtime_enable(&host->card->dev);

	return err;
}

/*
 * Callback for runtime_suspend.
 */
static int mmc_runtime_suspend(struct mmc_host *host)
{
	int err;

	if (!(host->caps & MMC_CAP_AGGRESSIVE_PM))
		return 0;

	err = _mmc_suspend(host, true);
	if (err)
		pr_err("%s: error %d doing aggessive suspend\n",
			mmc_hostname(host), err);

	return err;
}

/*
 * Callback for runtime_resume.
 */
static int mmc_runtime_resume(struct mmc_host *host)
{
	int err;

	if (!(host->caps & (MMC_CAP_AGGRESSIVE_PM | MMC_CAP_RUNTIME_RESUME)))
		return 0;

	err = _mmc_resume(host);
	if (err)
		pr_err("%s: error %d doing aggessive resume\n",
			mmc_hostname(host), err);

	return 0;
}

static int mmc_power_restore(struct mmc_host *host)
{
	int ret;

	mmc_claim_host(host);
	ret = mmc_init_card(host, host->card->ocr, host->card);
	mmc_release_host(host);

	return ret;
}

static int mmc_sleep(struct mmc_host *host)
{
    struct mmc_card *card = host->card;
    int err = -ENOSYS;

    if (card && card->ext_csd.rev >= 3) {
        err = mmc_card_sleepawake(host, 1);
    if (err < 0)
        pr_debug("%s: Error %d while putting card into sleep",
            mmc_hostname(host), err);
    }

    return err;
}

static int mmc_awake(struct mmc_host *host)
{
    struct mmc_card *card = host->card;
    int err = -ENOSYS;

    if (card && card->ext_csd.rev >= 3) {
        err = mmc_card_sleepawake(host, 0);
        if (err < 0)
            pr_debug("%s: Error %d while awaking sleeping card",
                mmc_hostname(host), err);
    }

    return err;
}

static const struct mmc_bus_ops mmc_ops = {
	.awake = mmc_awake,
	.sleep = mmc_sleep,
	.remove = mmc_remove,
	.detect = mmc_detect,
#ifdef CONFIG_MMC_PASSWORDS
	.sysfs_add = NULL,
	.sysfs_remove = NULL,
#endif
	.suspend = mmc_suspend,
	.resume = mmc_resume,
	.runtime_suspend = mmc_runtime_suspend,
	.runtime_resume = mmc_runtime_resume,
	.power_restore = mmc_power_restore,
	.alive = mmc_alive,
	.shutdown = mmc_shutdown,
};

/*
 * Starting point for MMC card init.
 */
int mmc_attach_mmc(struct mmc_host *host)
{
	int err;
	u32 ocr, rocr;

	BUG_ON(!host);
	WARN_ON(!host->claimed);

	/* Set correct bus mode for MMC before attempting attach */
	if (!mmc_host_is_spi(host))
		mmc_set_bus_mode(host, MMC_BUSMODE_OPENDRAIN);

	err = mmc_send_op_cond(host, 0, &ocr);
	if (err)
		return err;

	mmc_attach_bus(host, &mmc_ops);
	if (host->ocr_avail_mmc)
		host->ocr_avail = host->ocr_avail_mmc;

	/*
	 * We need to get OCR a different way for SPI.
	 */
	if (mmc_host_is_spi(host)) {
		err = mmc_spi_read_ocr(host, 1, &ocr);
		if (err)
			goto err;
	}

	rocr = mmc_select_voltage(host, ocr);

	/*
	 * Can we support the voltage of the card?
	 */
	if (!rocr) {
		err = -EINVAL;
		goto err;
	}

	/*
	 * Detect and init the card.
	 */
	err = mmc_init_card(host, rocr, NULL);
	if (err)
		goto err;

	mmc_release_host(host);
	err = mmc_add_card(host->card);
	mmc_claim_host(host);
	if (err)
		goto remove_card;

	return 0;

remove_card:
	mmc_release_host(host);
	mmc_remove_card(host->card);
	mmc_claim_host(host);
	host->card = NULL;
err:
	mmc_detach_bus(host);

	pr_err("%s: error %d whilst initialising MMC card\n",
		mmc_hostname(host), err);

	if(host->index == 0)
	mmc_trace_emmc_init_fail_reset();

	return err;
}

#ifdef CONFIG_HUAWEI_EMMC_DSM
unsigned int emmc_dsm_real_upload_size=0;
static unsigned int g_last_msg_code=0; /*last sent error code*/
static unsigned int g_last_msg_count=0; /*last sent the code count*/
#define ERR_MAX_COUNT 10
/*
 * first send the err msg to /dev/exception node.
 * if it produces lots of reduplicated msg, will just record the times
 * for every error, it's better to set max times
 * @code: error number
 * @err_msg: error message
 * @return: 0:don't report, 1: report
 */
static int dsm_emmc_process_log(int code, char *err_msg)
{
	int ret=0;
	/*the MAX times of erevy err code*/
	static char vdet_err_max_count = ERR_MAX_COUNT;
#ifndef CONFIG_HUAWEI_EMMC_DSM_ONLY_VDET
	static char system_w_err_max_count = ERR_MAX_COUNT;
	static char erase_err_max_count = ERR_MAX_COUNT;
	static char send_cxd_err_max_count = ERR_MAX_COUNT;
	static char emmc_read_err_max_count = ERR_MAX_COUNT;
	static char emmc_write_err_max_count = ERR_MAX_COUNT;
	static char emmc_tuning_err_max_count = ERR_MAX_COUNT;
	static char emmc_set_width_err_max_count = ERR_MAX_COUNT;
	static char emmc_pre_eol_max_count = ERR_MAX_COUNT;
	static char emmc_life_time_err_max_count = ERR_MAX_COUNT;
	static char emmc_rsp_err_max_count = ERR_MAX_COUNT;
	static char emmc_rw_timeout_max_count = ERR_MAX_COUNT;
	static char emmc_host_err_max_count = ERR_MAX_COUNT;
	static char emmc_urgent_bkops_max_count = ERR_MAX_COUNT;
	static char emmc_dyncap_needed_max_count = ERR_MAX_COUNT;
	static char emmc_syspool_exhausted_max_count = ERR_MAX_COUNT;
	static char emmc_packed_failure_max_count = ERR_MAX_COUNT;
#endif

	/*filter: if it has the same msg code with last, record err code&count*/
    if (g_last_msg_code == code) {
		ret = 0;
		g_last_msg_count++;
    } else {
		g_last_msg_code = code;
		g_last_msg_count = 0;
		ret = 1;
    }

	/*restrict count of every error, note:deplicated msg donesn't its count*/
	if(1 == ret){
		switch (code){
			case DSM_EMMC_VDET_ERR:
				if(0 < vdet_err_max_count){
					vdet_err_max_count--;
					ret = 1;
				}else{
					ret = 0;
				}
				break;
#ifndef CONFIG_HUAWEI_EMMC_DSM_ONLY_VDET
			case DSM_SYSTEM_W_ERR:
				if(0 < system_w_err_max_count){
					system_w_err_max_count--;
					ret = 1;
				}else{
					ret = 0;
				}
				break;
			case DSM_EMMC_ERASE_ERR:
				if(0 < erase_err_max_count){
					erase_err_max_count--;
					ret = 1;
				}else{
					ret = 0;
				}
				break;
			case DSM_EMMC_SEND_CXD_ERR:
				if(0 < send_cxd_err_max_count){
					send_cxd_err_max_count--;
					ret = 1;
				}else{
					ret = 0;
				}
				break;
			case DSM_EMMC_READ_ERR:
				if(0 < emmc_read_err_max_count){
					emmc_read_err_max_count--;
					ret = 1;
				}else{
					ret = 0;
				}
				break;
			case DSM_EMMC_WRITE_ERR:
				if(0 < emmc_write_err_max_count){
					emmc_write_err_max_count--;
					ret = 1;
				}else{
					ret = 0;
				}
				break;
			case DSM_EMMC_SET_BUS_WIDTH_ERR:
				if(0 < emmc_set_width_err_max_count){
					emmc_set_width_err_max_count--;
					ret = 1;
				}else{
					ret = 0;
				}
				break;
			case DSM_EMMC_PRE_EOL_INFO_ERR:
				if(0 < emmc_pre_eol_max_count){
					emmc_pre_eol_max_count--;
					ret = 1;
				}else{
					ret = 0;
				}
				break;
			case DSM_EMMC_TUNING_ERROR:
				if(0 < emmc_tuning_err_max_count){
					emmc_tuning_err_max_count--;
					ret = 1;
				}else{
					ret = 0;
				}
				break;
			case DSM_EMMC_LIFE_TIME_EST_ERR:
				if(0 < emmc_life_time_err_max_count){
					emmc_life_time_err_max_count--;
					ret = 1;
				}else{
					ret = 0;
				}
				break;
			case DSM_EMMC_RSP_ERR:
				if(0 < emmc_rsp_err_max_count){
					emmc_rsp_err_max_count--;
					ret = 1;
				}else{
					ret = 0;
				}
				break;
			case DSM_EMMC_RW_TIMEOUT_ERR:
				if(0 < emmc_rw_timeout_max_count){
					emmc_rw_timeout_max_count--;
					ret = 1;
				}else{
					ret = 0;
				}
				break;
			case DSM_EMMC_HOST_ERR:
				if(0 < emmc_host_err_max_count){
					emmc_host_err_max_count--;
					ret = 1;
				}else{
					ret = 0;
				}
				break;
			case DSM_EMMC_URGENT_BKOPS:
				if(0 < emmc_urgent_bkops_max_count){
					emmc_urgent_bkops_max_count--;
					ret = 1;
				}else{
					ret = 0;
				}
				break;
			case DSM_EMMC_DYNCAP_NEEDED:
				if(0 < emmc_dyncap_needed_max_count){
					emmc_dyncap_needed_max_count--;
					ret = 1;
				}else{
					ret = 0;
				}
				break;
			case DSM_EMMC_SYSPOOL_EXHAUSTED:
				if(0 < emmc_syspool_exhausted_max_count){
					emmc_syspool_exhausted_max_count--;
					ret = 1;
				}else{
					ret = 0;
				}
				break;
			case DSM_EMMC_PACKED_FAILURE:
				if(0 < emmc_packed_failure_max_count){
					emmc_packed_failure_max_count--;
					ret = 1;
				}else{
					ret = 0;
				}
				break;
#endif
			default:
				ret = 0;
				break;
		}
	}

	return ret;
}

/*
 * Put error message into buffer.
 * @code: error number
 * @err_msg: error message
 * @return: 0:no buffer to report, 1: report
 */
int dsm_emmc_get_log(void *card, int code, char *err_msg)
{
	int ret = 0;
	int buff_size = sizeof(g_emmc_dsm_log.emmc_dsm_log);
	char *dsm_log_buff = g_emmc_dsm_log.emmc_dsm_log;
	struct mmc_card * card_dev=(struct mmc_card * )card;
	unsigned int last_msg_code = g_last_msg_code;
	unsigned int last_msg_count = g_last_msg_count;
	int i=1;
	u8 *ext_csd=NULL;

	printk(KERN_ERR "dj enter dsm_emmc_get_log\n");
	if(dsm_emmc_process_log(code, err_msg)){
		/*clear global buffer*/
		memset(g_emmc_dsm_log.emmc_dsm_log,0,buff_size);
		/*print duplicated code and its count */
		if((0 < last_msg_count) && (0 == g_last_msg_count)){
			ret = snprintf(dsm_log_buff,buff_size,"last Err num: %d, the times: %d\n",last_msg_code, last_msg_count + 1);
			dsm_log_buff += ret;
			buff_size -= ret;
			last_msg_code = 0;
			last_msg_count = 0;
		}

		ret = snprintf(dsm_log_buff,buff_size,"Err num: %d, %s\n",code, err_msg);
		dsm_log_buff += ret;
		buff_size -= ret;
		pr_info("Err num: %d, %s\n",code, err_msg);
        printk(KERN_ERR "dj Err num: %d, %s\n",code, err_msg);
		/*print card CID info*/
		if(NULL != card_dev){
			if(sizeof(struct mmc_cid) < buff_size){
				ret = snprintf(dsm_log_buff,buff_size,
					"Card's cid:%08x%08x%08x%08x\n\n", card_dev->raw_cid[0], card_dev->raw_cid[1],
					card_dev->raw_cid[2], card_dev->raw_cid[3]);
				dsm_log_buff += ret;
				buff_size -= ret;
				pr_info("Card's cid:%08x%08x%08x%08x\n\n", card_dev->raw_cid[0], card_dev->raw_cid[1],
					card_dev->raw_cid[2], card_dev->raw_cid[3]);
			}else{
				printk(KERN_ERR "%s:g_emmc_dsm_log Buff size is not enough\n", __FUNCTION__);
				printk(KERN_ERR "%s:eMMC error message is: %s\n", __FUNCTION__, err_msg);
			}

			/*print card ios info*/
			if(sizeof(card_dev->host->ios)< buff_size){
				if(NULL != card_dev->host){
					ret = snprintf(dsm_log_buff,buff_size,
						"Card's ios.clock:%uHz, ios.old_rate:%uHz, ios.power_mode:%u, ios.timing:%u, ios.bus_mode:%u, ios.bus_width:%u\n",
						card_dev->host->ios.clock, card_dev->host->ios.old_rate, card_dev->host->ios.power_mode, card_dev->host->ios.timing,
						card_dev->host->ios.bus_mode, card_dev->host->ios.bus_width);
					dsm_log_buff += ret;
					buff_size -= ret;
					pr_info("Card's ios.clock:%uHz, ios.old_rate:%uHz, ios.power_mode:%u, ios.timing:%u, ios.bus_mode:%u, ios.bus_width:%u\n",
						card_dev->host->ios.clock, card_dev->host->ios.old_rate, card_dev->host->ios.power_mode, card_dev->host->ios.timing,
						card_dev->host->ios.bus_mode, card_dev->host->ios.bus_width);
				}
			}else{
				printk(KERN_ERR "%s:g_emmc_dsm_log Buff size is not enough\n", __FUNCTION__);
				printk(KERN_ERR "%s:eMMC error message is: %s\n", __FUNCTION__, err_msg);
			}

			/*print card ext_csd info*/
			if(EMMC_EXT_CSD_LENGHT < buff_size){
				if(NULL != card_dev->cached_ext_csd){
					ret = snprintf(dsm_log_buff,buff_size,"eMMC ext_csd(Note: just static slice data is believable):\n");
					dsm_log_buff += ret;
					buff_size -= ret;

					ext_csd = card_dev->cached_ext_csd;
					for (i = 0; i < EMMC_EXT_CSD_LENGHT; i++){
						ret = snprintf(dsm_log_buff,buff_size,"%02x", ext_csd[i]);
						dsm_log_buff += ret;
						buff_size -= ret;
					}
					ret = snprintf(dsm_log_buff,buff_size,"\n\n");
					dsm_log_buff += ret;
					buff_size -= ret;
				}
			}else{
				printk(KERN_ERR "%s:g_emmc_dsm_log Buff size is not enough\n", __FUNCTION__);
				printk(KERN_ERR "%s:eMMC error message is: %s\n", __FUNCTION__, err_msg);
			}
		}
		/*get size of used buffer*/
		emmc_dsm_real_upload_size = sizeof(g_emmc_dsm_log.emmc_dsm_log) - buff_size;
		pr_debug("DSM_DEBUG %s\n",g_emmc_dsm_log.emmc_dsm_log);
        printk(KERN_ERR "dj DSM_DEBUG %s\n",g_emmc_dsm_log.emmc_dsm_log);
		return 1;
	}else{
		printk("%s:Err num: %d, %s\n",__FUNCTION__, code, err_msg);
		if(NULL != card_dev){
			pr_info("Card's cid:%08x%08x%08x%08x\n\n", card_dev->raw_cid[0], card_dev->raw_cid[1],
				card_dev->raw_cid[2], card_dev->raw_cid[3]);
			pr_info("Card's ios.clock:%uHz, ios.old_rate:%uHz, ios.power_mode:%u, ios.timing:%u, ios.bus_mode:%u, ios.bus_width:%u\n",
					card_dev->host->ios.clock, card_dev->host->ios.old_rate, card_dev->host->ios.power_mode, card_dev->host->ios.timing,
					card_dev->host->ios.bus_mode, card_dev->host->ios.bus_width);
		}
	}
      printk(KERN_ERR "dj leave dsm_emmc_get_log\n");
	return 0;
}
EXPORT_SYMBOL(dsm_emmc_get_log);
#endif