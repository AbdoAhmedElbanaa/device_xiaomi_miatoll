#!/bin/bash

# hardware/sony/timekeep
git clone -b lineage-23.2 https://github.com/LineageOS/android_hardware_sony_timekeep.git hardware/sony/timekeep --depth=1

# hardware/xiaomi
git clone -b 16-QPR2 https://github.com/AndroidOne-Experience/hardware_xiaomi.git hardware/xiaomi --depth=1

# hardware/lineage/combat
rm -rf hardware/lineage/compat
git clone -b 16-QPR2 https://github.com/AndroidOne-Experience/hardware_lineage_compat.git hardware/lineage/compat --depth=1

# Vendor tree
git clone -b 16-QPR2 https://github.com/AndroidOne-Experience/vendor_xiaomi_miatoll.git vendor/xiaomi/miatoll --depth=1
git clone https://github.com/AndroidOne-Experience/vendor_dolby.git vendor/dolby --depth=1 -b v1.2

# Kernel tree
git clone -b 16-QPR2 https://github.com/AndroidOne-Experience/kernel_xiaomi_sm6250.git kernel/xiaomi/sm6250 --depth=1
