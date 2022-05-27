CFG_TEE_TA_LOG_LEVEL ?= 4

TA_DEV_KIT_DIR=$(shell echo $$SDKTARGETSYSROOT)/usr/include/optee/export-user_ta

# The UUID for the Trusted Application
BINARY=66666666-6666-6666-6666-666666666666

-include $(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk

ifeq ($(wildcard $(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk), )
clean:
	@echo 'Note: $$(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk not found, cannot clean TA'
	@echo 'Note: TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR)'
endif
