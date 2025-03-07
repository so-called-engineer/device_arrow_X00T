/*
   Copyright (c) 2015, The Linux Foundation. All rights reserved.
   Copyright (C) 2016 The CyanogenMod Project.
   Copyright (C) 2018-2019 The LineageOS Project
   Copyright (C) 2018-2019 KudProject Development
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.
   THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <cstdio>

#include <android-base/file.h>
#include <android-base/properties.h>
#include <android-base/strings.h>

#include <sys/sysinfo.h>
#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>

#include "property_service.h"
#include "vendor_init.h"

using android::base::GetProperty;
using android::base::ReadFileToString;
using android::base::Trim;
using android::init::property_set;

void property_override(char const prop[], char const value[])
{
    prop_info *pi;

    pi = (prop_info*) __system_property_find(prop);
    if (pi)
        __system_property_update(pi, value, strlen(value));
    else
        __system_property_add(prop, strlen(prop), value, strlen(value));
}

void property_override_dual(char const system_prop[], char const vendor_prop[], char const value[])
{
    property_override(system_prop, value);
    property_override(vendor_prop, value);
}

void property_override_triple(char const system_prop[], char const vendor_prop[], char const bootimg_prop[], char const value[])
{
    property_override(system_prop, value);
    property_override(vendor_prop, value);
    property_override(bootimg_prop, value);
}

static void init_alarm_boot_properties()
{
    char const *boot_reason_file = "/proc/sys/kernel/boot_reason";
    std::string boot_reason;
    std::string reboot_reason = GetProperty("ro.boot.alarmboot", "");

    if (ReadFileToString(boot_reason_file, &boot_reason)) {
        /*
         * Setup ro.alarm_boot value to true when it is RTC triggered boot up
         * For existing PMIC chips, the following mapping applies
         * for the value of boot_reason:
         *
         * 0 -> unknown
         * 1 -> hard reset
         * 2 -> sudden momentary power loss (SMPL)
         * 3 -> real time clock (RTC)
         * 4 -> DC charger inserted
         * 5 -> USB charger inserted
         * 6 -> PON1 pin toggled (for secondary PMICs)
         * 7 -> CBLPWR_N pin toggled (for external power supply)
         * 8 -> KPDPWR_N pin toggled (power key pressed)
         */
        if (Trim(boot_reason) == "3" || reboot_reason == "true")
            property_set("ro.alarm_boot", "true");
        else
            property_set("ro.alarm_boot", "false");
    }
}

void vendor_check_variant()
{
    struct sysinfo sys;
    char const *region_file = "/mnt/vendor/persist/flag/countrycode.txt";
    char const *build_fingerprint, *product_device, *product_model, *product_name;
    std::string region;

    sysinfo(&sys);

    // Make sure the region value is trimmed first
    if (ReadFileToString(region_file, &region))
        region = Trim(region);

    // Russian model has a slightly different product name
    if (region == "RU")
        product_name = "RU_X00TD";
    else
        product_name = "WW_X00TD";

    // 6 GB variant
    if (sys.totalram > 4096ull * 1024 * 1024) {
        // Russian model
        if (region == "RU") {
            //build_fingerprint = "google/coral/coral:10/QQ2A.200305.003/6156912:user/release-keys";
            product_device = "ASUS_X00T_9";

        // Global model
        } else {
            //build_fingerprint = "google/coral/coral:10/QQ2A.200305.003/6156912:user/release-keys";
            product_device = "ASUS_X00T_3";
        }

    // 3/4 GB variants
    } else {
        // Russian model
        if (region == "RU") {
            //build_fingerprint = "google/coral/coral:10/QQ2A.200305.003/6156912:user/release-keys";
            product_device = "ASUS_X00T_6";

        // Global model
        } else {
            //build_fingerprint = "google/coral/coral:10/QQ2A.200305.003/6156912:user/release-keys";
            product_device = "ASUS_X00T_2";
        }
    }

    // Product model overrides
    if (region == "RU" || region == "TW" ||
        (region == "PH" && sys.totalram > 3072ull * 1024 * 1024))
        product_model = "ASUS_X00TDB";
    else if (sys.totalram < 3072ull * 1024 * 1024)
        product_model = "ASUS_X00TDA";
    else
        product_model = "ASUS_X00TD";
   
   //Google walley Fingerptint
    build_fingerprint = "google/walleye/walleye:8.1.0/OPM1.171019.011/4448085:user/release-keys";

    // Override props based on values set
    property_override_dual("ro.product.device", "ro.vendor.product.device", product_device);
    property_override_dual("ro.product.model", "ro.vendor.product.model", product_model);
    property_override_dual("ro.product.name", "ro.vendor.product.name", product_name);
    property_override_triple("ro.build.fingerprint", "ro.vendor.build.fingerprint", "ro.bootimage.build.fingerprint", build_fingerprint);

    // Set region code via ro.config.versatility prop
    property_set("ro.config.versatility", region);
}

void vendor_load_properties()
{
    init_alarm_boot_properties();
    vendor_check_variant();
}
