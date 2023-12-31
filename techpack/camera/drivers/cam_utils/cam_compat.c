// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2014-2021, The Linux Foundation. All rights reserved.
 */

#include <linux/dma-mapping.h>
#include <linux/of_address.h>

#include "cam_compat.h"
#include "cam_debug_util.h"
#include "cam_cpas_api.h"
#include "camera_main.h"
#include "qseecom_kernel.h"

#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
#define SECCAM_TA_NAME "sec_fr"
#define SECCAM_QSEECOM_SBUFF_SIZE (64 * 1024)

static DEFINE_SPINLOCK(secure_mode_lock);
int load_ref_cnt;

static int cam_refcnt_status(int rw, bool protect)
{
	int ret = 0;

	spin_lock(&secure_mode_lock);
	switch (rw) {
	case 0: // read
		ret = load_ref_cnt;
		break;
	case 1: //write
		{
			if (protect) load_ref_cnt++;
			else load_ref_cnt--;
			ret = load_ref_cnt;
		}
		break;
	}
	spin_unlock(&secure_mode_lock);

	return ret;
}

static int cam_csiphy_reload_TA(struct qseecom_handle **ta_qseecom_handle)
{
	int rc = 0, retry = 0;

	CAM_INFO(CAM_CSIPHY, "[SCM_DBG] Try to recovery");
	rc = qseecom_shutdown_app(ta_qseecom_handle);
	if (rc) {
		CAM_ERR(CAM_CSIPHY, "TA UnLoad failed\n");
		rc = -EINVAL;
	}
	CAM_INFO(CAM_CSIPHY, "[SCM_DBG] 1. TA Unload done");
	msleep(10);

	do {
		rc = qseecom_start_app(
			ta_qseecom_handle,
			SECCAM_TA_NAME,
			SECCAM_QSEECOM_SBUFF_SIZE);
		if (rc == -ENOMEM) {
			CAM_ERR(CAM_CSIPHY, "[SCM_DBG] retry Loading TA");
			msleep(10);
		}
	} while ((rc == -ENOMEM) && (++retry < 30));
	CAM_INFO(CAM_CSIPHY, "[SCM_DBG] 2. TA load done");
	msleep(10);

	return rc;
}
int cam_reserve_icp_fw(struct cam_fw_alloc_info *icp_fw, size_t fw_length)
{
	int rc = 0;
	struct device_node *of_node;
	struct device_node *mem_node;
	struct resource     res;

	of_node = (icp_fw->fw_dev)->of_node;
	mem_node = of_parse_phandle(of_node, "memory-region", 0);
	if (!mem_node) {
		rc = -ENOMEM;
		CAM_ERR(CAM_SMMU, "FW memory carveout not found");
		goto end;
	}
	rc = of_address_to_resource(mem_node, 0, &res);
	of_node_put(mem_node);
	if (rc < 0) {
		CAM_ERR(CAM_SMMU, "Unable to get start of FW mem carveout");
		goto end;
	}
	icp_fw->fw_hdl = res.start;
	icp_fw->fw_kva = ioremap_wc(icp_fw->fw_hdl, fw_length);
	if (!icp_fw->fw_kva) {
		CAM_ERR(CAM_SMMU, "Failed to map the FW.");
		rc = -ENOMEM;
		goto end;
	}
	memset_io(icp_fw->fw_kva, 0, fw_length);

end:
	return rc;
}

void cam_unreserve_icp_fw(struct cam_fw_alloc_info *icp_fw, size_t fw_length)
{
	iounmap(icp_fw->fw_kva);
}

int cam_ife_notify_safe_lut_scm(bool safe_trigger)
{
	const uint32_t smmu_se_ife = 0;
	uint32_t camera_hw_version, rc = 0;

	rc = cam_cpas_get_cpas_hw_version(&camera_hw_version);
	if (!rc && qcom_scm_smmu_notify_secure_lut(smmu_se_ife, safe_trigger)) {
		switch (camera_hw_version) {
		case CAM_CPAS_TITAN_170_V100:
		case CAM_CPAS_TITAN_170_V110:
		case CAM_CPAS_TITAN_175_V100:
			CAM_ERR(CAM_ISP, "scm call to enable safe failed");
			rc = -EINVAL;
			break;
		default:
			break;
		}
	}

	return rc;
}

int cam_csiphy_notify_secure_mode(struct csiphy_device *csiphy_dev,
	bool protect, int32_t offset)
{
//implemention TA Loading code from SM6225 CL23653850
	static struct qseecom_handle   *ta_qseecom_handle;
	int32_t rc;
	int ret = 0;
	uint32_t retry = 0;

	if (offset >= CSIPHY_MAX_INSTANCES_PER_PHY) {
		CAM_ERR(CAM_CSIPHY, "Invalid CSIPHY offset");
		return -EINVAL;
	}

	CAM_INFO(CAM_CSIPHY, "++ @@ phy : %d, protect : %d, load_ref_cnt %d", offset, protect, cam_refcnt_status(0,0));

	if (protect) {
		if (cam_refcnt_status(0,0) == 0)
		{
			CAM_INFO(CAM_CSIPHY, "Loading TA.....\n");
			do {
				rc = qseecom_start_app(
					&ta_qseecom_handle,
					SECCAM_TA_NAME,
					SECCAM_QSEECOM_SBUFF_SIZE);
				if (rc == -ENOMEM) {
					CAM_ERR(CAM_CSIPHY, "retry Loading TA");
					msleep(10);
				}
			} while ((rc == -ENOMEM) && (++retry < 30));

			if (rc) {
				CAM_ERR(CAM_CSIPHY, "TA Load failed\n");
				//ret = -EINVAL;
				return -EINVAL;
			}
		}
		CAM_INFO(CAM_CSIPHY, "SCM CALL for protection\n");
		if (qcom_scm_camera_protect_phy_lanes(protect,
			csiphy_dev->csiphy_info[offset]
				.csiphy_cpas_cp_reg_mask)) {
			CAM_ERR(CAM_CSIPHY, "scm call to hypervisor failed");
                        goto reload_ta;
		}
		else {
			cam_refcnt_status(1, protect);
		}
	}
	else {
		//BUG_ON (cam_refcnt_status(0,0) == 0);
		CAM_INFO(CAM_CSIPHY, "SCM CALL for un-protection\n");
		if (qcom_scm_camera_protect_phy_lanes(protect,
			csiphy_dev->csiphy_info[offset]
				.csiphy_cpas_cp_reg_mask)) {
			CAM_ERR(CAM_CSIPHY, "scm call to hypervisor failed");
			rc = cam_csiphy_reload_TA(&ta_qseecom_handle);
			if (rc < 0)
				CAM_ERR(CAM_CSIPHY, "[SCM_DBG] reload TA failed");

			if (qcom_scm_camera_protect_phy_lanes(protect,
			csiphy_dev->csiphy_info[offset]
				.csiphy_cpas_cp_reg_mask)) {
				CAM_ERR(CAM_CSIPHY, "[SCM_DBG] scm call to hypervisor failed, recovery failed");
				ret = -EINVAL;
			} else {
				CAM_ERR(CAM_CSIPHY, "[SCM_DBG] scm call to hypervisor success, recovery success");
				ret = 0;
			}
		}
		cam_refcnt_status(1, protect);
		if (cam_refcnt_status(0,0) == 0)
		{
			//Unload the TA when the last camera is switched back to non-secure mode
			CAM_INFO(CAM_CSIPHY, "UnLoading TA.....\n");
			rc = qseecom_shutdown_app(&ta_qseecom_handle);
			if (rc) {
				CAM_ERR(CAM_CSIPHY, "TA UnLoad failed\n");
				ret = -EINVAL;
			}
		}
	}
	CAM_INFO(CAM_CSIPHY, "-- @@ phy : %d, protect : %d, load_ref_cnt %d, ret %d", offset, protect, cam_refcnt_status(0,0), ret);

        return ret;
reload_ta:
	rc = qseecom_shutdown_app(&ta_qseecom_handle);
        if (rc) {
	        CAM_ERR(CAM_CSIPHY, "TA UnLoad failed\n");
	        ret = -EINVAL;
	}
	rc = qseecom_start_app(
		&ta_qseecom_handle,
		SECCAM_TA_NAME,
		SECCAM_QSEECOM_SBUFF_SIZE);
	if (rc) {
		CAM_ERR(CAM_CSIPHY, "TA Load failed\n");
		return -EINVAL;
	}
	if (qcom_scm_camera_protect_phy_lanes(protect,
			csiphy_dev->csiphy_info[offset]
				.csiphy_cpas_cp_reg_mask))
        {
                CAM_ERR(CAM_CSIPHY, "[SCM_DBG] scm call to hypervisor failed, recovery failed");
		ret = -EINVAL;
        }
	if (!load_ref_cnt) {
		rc = qseecom_shutdown_app(&ta_qseecom_handle);
                if (rc) {
			CAM_ERR(CAM_CSIPHY, "TA UnLoad failed\n");
			ret = -EINVAL;
		}
        }

    return ret;

}

void cam_cpastop_scm_write(struct cam_cpas_hw_errata_wa *errata_wa)
{
	int reg_val;

	qcom_scm_io_readl(errata_wa->data.reg_info.offset, &reg_val);
	reg_val |= errata_wa->data.reg_info.value;
	qcom_scm_io_writel(errata_wa->data.reg_info.offset, reg_val);
}

static int camera_platform_compare_dev(struct device *dev, const void *data)
{
	return platform_bus_type.match(dev, (struct device_driver *) data);
}
#else
int cam_reserve_icp_fw(struct cam_fw_alloc_info *icp_fw, size_t fw_length)
{
	int rc = 0;

	icp_fw->fw_kva = dma_alloc_coherent(icp_fw->fw_dev, fw_length,
		&icp_fw->fw_hdl, GFP_KERNEL);

	if (!icp_fw->fw_kva) {
		CAM_ERR(CAM_SMMU, "FW memory alloc failed");
		rc = -ENOMEM;
	}

	return rc;
}

void cam_unreserve_icp_fw(struct cam_fw_alloc_info *icp_fw, size_t fw_length)
{
	dma_free_coherent(icp_fw->fw_dev, fw_length, icp_fw->fw_kva,
		icp_fw->fw_hdl);
}

int cam_ife_notify_safe_lut_scm(bool safe_trigger)
{
	const uint32_t smmu_se_ife = 0;
	uint32_t camera_hw_version, rc = 0;
	struct scm_desc description = {
		.arginfo = SCM_ARGS(2, SCM_VAL, SCM_VAL),
		.args[0] = smmu_se_ife,
		.args[1] = safe_trigger,
	};

	rc = cam_cpas_get_cpas_hw_version(&camera_hw_version);
	if (!rc && scm_call2(SCM_SIP_FNID(0x15, 0x3), &description)) {
		switch (camera_hw_version) {
		case CAM_CPAS_TITAN_170_V100:
		case CAM_CPAS_TITAN_170_V110:
		case CAM_CPAS_TITAN_175_V100:
			CAM_ERR(CAM_ISP, "scm call to enable safe failed");
			rc = -EINVAL;
			break;
		default:
			break;
		}
	}

	return rc;
}

int cam_csiphy_notify_secure_mode(struct csiphy_device *csiphy_dev,
	bool protect, int32_t offset)
{
	int rc = 0;
	struct scm_desc description = {
		.arginfo = SCM_ARGS(2, SCM_VAL, SCM_VAL),
		.args[0] = protect,
		.args[1] = csiphy_dev->csiphy_info[offset]
			.csiphy_cpas_cp_reg_mask,
	};

	if (offset >= CSIPHY_MAX_INSTANCES_PER_PHY) {
		CAM_ERR(CAM_CSIPHY, "Invalid CSIPHY offset");
		rc = -EINVAL;
	} else if (scm_call2(SCM_SIP_FNID(0x18, 0x7), &description)) {
		CAM_ERR(CAM_CSIPHY, "SCM call to hypervisor failed");
		rc = -EINVAL;
	}

	return rc;
}

void cam_cpastop_scm_write(struct cam_cpas_hw_errata_wa *errata_wa)
{
	int reg_val;

	reg_val = scm_io_read(errata_wa->data.reg_info.offset);
	reg_val |= errata_wa->data.reg_info.value;
	scm_io_write(errata_wa->data.reg_info.offset, reg_val);
}

static int camera_platform_compare_dev(struct device *dev, void *data)
{
	return platform_bus_type.match(dev, (struct device_driver *) data);
}
#endif

/* Callback to compare device from match list before adding as component */
static inline int camera_component_compare_dev(struct device *dev, void *data)
{
	return dev == data;
}

/* Add component matches to list for master of aggregate driver */
int camera_component_match_add_drivers(struct device *master_dev,
	struct component_match **match_list)
{
	int i, rc = 0;
	struct platform_device *pdev = NULL;
	struct device *start_dev = NULL, *match_dev = NULL;

	if (!master_dev || !match_list) {
		CAM_ERR(CAM_UTIL, "Invalid parameters for component match add");
		rc = -EINVAL;
		goto end;
	}

	for (i = 0; i < ARRAY_SIZE(cam_component_drivers); i++) {
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		struct device_driver const *drv =
			&cam_component_drivers[i]->driver;
		const void *drv_ptr = (const void *)drv;
#else
		struct device_driver *drv = &cam_component_drivers[i]->driver;
		void *drv_ptr = (void *)drv;
#endif
		start_dev = NULL;
		while ((match_dev = bus_find_device(&platform_bus_type,
			start_dev, drv_ptr, &camera_platform_compare_dev))) {
			put_device(start_dev);
			pdev = to_platform_device(match_dev);
			CAM_DBG(CAM_UTIL, "Adding matched component:%s", pdev->name);
			component_match_add(master_dev, match_list,
				camera_component_compare_dev, match_dev);
			start_dev = match_dev;
		}
		put_device(start_dev);
	}

end:
	return rc;
}
