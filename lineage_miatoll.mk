#
# SPDX-FileCopyrightText: The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

# Inherit from those products. Most specific first.
$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit.mk)
TARGET_SUPPORTS_OMX_SERVICE := false
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/non_ab_device.mk)

# ============================================
# RisingOS Build Configuration & Flags
# ============================================

# Maintainer & Device Specs (evaluated by vendor/rising/config/version.mk)
RISING_MAINTAINER := Abdoelbanaa
RISING_BATTERY := 5020mAh
RISING_DISPLAY := 1080x2400
RISING_RAM := 4GB/6GB/8GB
RISING_STORAGE := 64GB/128GB

# Blur support (true/false)
TARGET_ENABLE_BLUR := true

# Whether to skip shipping Aperture camera (MiuiCamera is shipped by default)
PRODUCT_NO_CAMERA := true

# Torch strength control support
TORCH_STR_SUPPORTED := true

# ============================================
# GMS / Package Type Flags
# ============================================
# Default: Full GAPPS build
WITH_GMS ?= true

# Uncomment for CORE build (Pico GApps):
# WITH_GMS := true
# TARGET_USES_PICO_GAPPS := true

# Uncomment for VANILLA build:
# WITH_GMS := false

# Uncomment for MICROG build:
# WITH_GMS := false
# WITH_MICROG := true

# Inherit some common Lineage / RisingOS stuff.
$(call inherit-product, vendor/lineage/config/common_full_phone.mk)

# Inherit from miatoll device
$(call inherit-product, device/xiaomi/miatoll/atoll.mk)

# Boot animation
TARGET_SCREEN_HEIGHT := 2400
TARGET_SCREEN_WIDTH := 1080

# Device identifiers
PRODUCT_NAME := lineage_miatoll
PRODUCT_DEVICE := miatoll
PRODUCT_MANUFACTURER := Xiaomi
PRODUCT_BRAND := Redmi
PRODUCT_MODEL := SM6250

PRODUCT_GMS_CLIENTID_BASE := android-xiaomi

# Chipset, Maintainer & Fingerprint overrides (ro.rising.chipset / ro.rising.maintainer)
PRODUCT_BUILD_PROP_OVERRIDES += \
    BuildDesc="miatoll_global-user 12 SKQ1.211019.001 V14.0.3.0.SJZMIXM release-keys" \
    BuildFingerprint=Redmi/miatoll_global/miatoll:12/RKQ1.211019.001/V14.0.3.0.SJZMIXM:user/release-keys \
    RisingChipset="Snapdragon 720G" \
    RisingMaintainer="$(RISING_MAINTAINER)"

# Fix missing ro.build.product for non-AB OTA package generation
PRODUCT_SYSTEM_PROPERTIES += \
    ro.build.product=miatoll
