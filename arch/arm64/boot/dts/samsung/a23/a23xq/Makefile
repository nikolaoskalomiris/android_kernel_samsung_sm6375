dtbo-y += a23xq_eur_open_w00_r10.dtbo # ADDED BY LEGO AUTOMATICALLY: DO NOT SUBMIT
a23xq_eur_open_w00_r10.dtbo-base := ../../../vendor/qcom/blair.dtb # ADDED BY LEGO AUTOMATICALLY: DO NOT SUBMIT
dtbo-y += a23xq_eur_open_w00_r09.dtbo # ADDED BY LEGO AUTOMATICALLY: DO NOT SUBMIT
a23xq_eur_open_w00_r09.dtbo-base := ../../../vendor/qcom/blair.dtb # ADDED BY LEGO AUTOMATICALLY: DO NOT SUBMIT
dtbo-y += a23xq_eur_open_w00_r06.dtbo # ADDED BY LEGO AUTOMATICALLY: DO NOT SUBMIT
a23xq_eur_open_w00_r06.dtbo-base := ../../../vendor/qcom/blair.dtb # ADDED BY LEGO AUTOMATICALLY: DO NOT SUBMIT
dtbo-y += a23xq_eur_open_w00_r05.dtbo # ADDED BY LEGO AUTOMATICALLY: DO NOT SUBMIT
a23xq_eur_open_w00_r05.dtbo-base := ../../../vendor/qcom/blair.dtb # ADDED BY LEGO AUTOMATICALLY: DO NOT SUBMIT
dtbo-y += a23xq_eur_open_w00_r04.dtbo # ADDED BY LEGO AUTOMATICALLY: DO NOT SUBMIT
a23xq_eur_open_w00_r04.dtbo-base := ../../../vendor/qcom/blair.dtb # ADDED BY LEGO AUTOMATICALLY: DO NOT SUBMIT
dtbo-y += a23xq_eur_open_w00_r03.dtbo # ADDED BY LEGO AUTOMATICALLY: DO NOT SUBMIT
a23xq_eur_open_w00_r03.dtbo-base := ../../../vendor/qcom/blair.dtb # ADDED BY LEGO AUTOMATICALLY: DO NOT SUBMIT
dtbo-y += a23xq_eur_open_w00_r02.dtbo # ADDED BY LEGO AUTOMATICALLY: DO NOT SUBMIT
a23xq_eur_open_w00_r02.dtbo-base := ../../../vendor/qcom/blair.dtb # ADDED BY LEGO AUTOMATICALLY: DO NOT SUBMIT
dtbo-y += a23xq_eur_open_w00_r01.dtbo # ADDED BY LEGO AUTOMATICALLY: DO NOT SUBMIT
a23xq_eur_open_w00_r01.dtbo-base := ../../../vendor/qcom/blair.dtb # ADDED BY LEGO AUTOMATICALLY: DO NOT SUBMIT
dtbo-y += a23xq_eur_open_w00_r00.dtbo # ADDED BY LEGO AUTOMATICALLY: DO NOT SUBMIT
a23xq_eur_open_w00_r00.dtbo-base := ../../../vendor/qcom/blair.dtb # ADDED BY LEGO AUTOMATICALLY: DO NOT SUBMIT
# SPDX-License-Identifier: GPL-2.0

ifeq ($(CONFIG_BUILD_ARM64_DT_OVERLAY),y)

define __sec_dtbo_build
dtbo-$(2) += $(1)
$(1)-base := $(3)
endef
	
always          := $(dtb-y)
subdir-y        := $(dts-dirs)
endif

clean-files := *.dtb *.reverse.dts *.dtbo
