#!/usr/bin/env -S PYTHONPATH=../../../tools/extract-utils python3
#
# SPDX-FileCopyrightText: 2024 The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

import extract_utils.tools

extract_utils.tools.DEFAULT_PATCHELF_VERSION = '0_17_2'

from extract_utils.main import (
    ExtractUtils,
    ExtractUtilsModule,
)
from extract_utils.fixups_blob import (
    blob_fixup,
    blob_fixups_user_type,
)
from extract_utils.fixups_lib import (
    lib_fixups,
    lib_fixups_user_type,
)

namespace_imports = [
    'device/nubia/msm8998-common',
    'hardware/qcom-caf/msm8998',
    'vendor/nubia/msm8998-common',
]

lib_fixups: lib_fixups_user_type = {
    **lib_fixups,
}

blob_fixups: blob_fixups_user_type = {
    'vendor/lib/hw/camera.msm8998.so': blob_fixup()
        .remove_needed('libgui.so')
        .remove_needed('libandroid.so'),
    (
     'vendor/lib/libnubia_effect.so',
     'vendor/lib64/libnubia_effect.so'
    ): blob_fixup()
        .remove_needed('libgui.so'),
    'vendor/lib/libNubiaImageAlgorithm.so': blob_fixup()
        .remove_needed('libjnigraphics.so')
        .remove_needed('libnativehelper.so')
        .add_needed('libui_shim.so')
        .add_needed('libNubiaImageAlgorithmShim.so'),
    'vendor/lib/libarcsoft_picauto.so': blob_fixup()
        .remove_needed('libandroid.so'),
    'vendor/lib64/com.fingerprints.extension@1.0.so': blob_fixup()
        .add_needed('libhidlbase_shim.so'),
    (
     'vendor/lib/libAltek_AF.so',
     'vendor/lib/libHAFIAFalSDE1.so',
     'vendor/lib/libIAFalSDE1.so',
     'vendor/lib/libIQ_Match_Lib.so',
     'vendor/lib/libSonyIMX318PdafLibrary.so',
     'vendor/lib/libalCMotion.so',
     'vendor/lib/libalParseOTP.so',
     'vendor/lib/libalRnB.so',
     'vendor/lib/libalSDE2.so',
     'vendor/lib/libalSDK.so',
     'vendor/lib/libalSPE.so',
     'vendor/lib/libarcsoft_beautyshot.so',
     'vendor/lib/libarcsoft_beautyshot_image_algorithm.so',
     'vendor/lib/libarcsoft_beautyshot_video_algorithm.so',
     'vendor/lib/libarcsoft_dualcam_refocus.so',
     'vendor/lib/libarcsoft_low_light_shot.so',
     'vendor/lib/libarcsoft_night_shot.so',
     'vendor/lib/libarcsoft_picauto.so',
     'vendor/lib64/libalParseOTP.so',
     'vendor/lib64/libalRnB.so',
     'vendor/lib64/libalSDE2.so',
     'vendor/lib64/libalSPE.so',
     'vendor/lib64/libarcsoft_beautyshot.so',
     'vendor/lib64/libarcsoft_beautyshot_image_algorithm.so',
     'vendor/lib64/libarcsoft_beautyshot_video_algorithm.so',
     'vendor/lib64/libarcsoft_dualcam_refocus.so',
     'vendor/lib64/libarcsoft_low_light_shot.so',
     'vendor/lib64/libarcsoft_night_shot.so',
    ): blob_fixup()
        .replace_needed('libstdc++.so', 'libstdc++_vendor.so'),
    'vendor/lib/libmmcamera_faceproc.so': blob_fixup()
        .clear_symbol_version('__aeabi_memcpy')
        .clear_symbol_version('__aeabi_memset')
        .clear_symbol_version('__gnu_Unwind_Find_exidx'),
    'vendor/bin/qfp-daemon': blob_fixup()
        .replace_needed('libhidltransport.so', 'libhidlbase.so'),
}  # fmt: skip

module = ExtractUtilsModule(
    'nx563j',
    'nubia',
    blob_fixups=blob_fixups,
    lib_fixups=lib_fixups,
    namespace_imports=namespace_imports,
)

if __name__ == '__main__':
    utils = ExtractUtils.device_with_common(
        module, 'msm8998-common', module.vendor
    )
    utils.run()
