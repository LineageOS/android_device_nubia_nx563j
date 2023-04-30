/*
 * Copyright (C) 2019 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_HARDWARE_LIGHT_V2_0_LIGHT_H
#define ANDROID_HARDWARE_LIGHT_V2_0_LIGHT_H

#include <android/hardware/light/2.0/ILight.h>
#include <hardware/lights.h>
#include <hidl/Status.h>
#include <mutex>
#include <vector>
#include <unordered_map>

// Events
#define ONGOING_NONE             0
#define ONGOING_BUTTONS         (1 << 0)
#define ONGOING_ATTENTION       (1 << 1)
#define ONGOING_NOTIFICATION    (1 << 2)
#define ONGOING_BATTERY         (1 << 3)

// Blink mode
#define BLINK_MODE_ON 1
#define BLINK_MODE_OFF 2
#define BLINK_MODE_BREATH_AUTO 3
#define BLINK_MODE_BREATH_ONCE 6
#define BLINK_MODE_BREATH_CUSTOM 7

using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::light::V2_0::Flash;
using ::android::hardware::light::V2_0::ILight;
using ::android::hardware::light::V2_0::LightState;
using ::android::hardware::light::V2_0::Status;
using ::android::hardware::light::V2_0::Type;

static unsigned int brightness_table[256] = {
    0,    1,    17,   65,   97,   146,  178,  226,  436,  597,  758,  887,
    968,  1080, 1161, 1258, 1338, 1371, 1451, 1532, 1580, 1661, 1709, 1741,
    1822, 1870, 1902, 1983, 2031, 2080, 2112, 2160, 2193, 2241, 2273, 2322,
    2322, 2322, 2322, 2450, 2450, 2483, 2531, 2531, 2563, 2563, 2612, 2612,
    2644, 2644, 2692, 2692, 2724, 2724, 2773, 2773, 2805, 2805, 2853, 2853,
    2853, 2902, 2902, 2902, 2934, 2934, 2934, 2982, 2982, 2982, 3015, 3015,
    3015, 3063, 3063, 3063, 3063, 3095, 3095, 3095, 3095, 3144, 3144, 3176,
    3176, 3176, 3176, 3176, 3224, 3224, 3224, 3272, 3272, 3272, 3272, 3272,
    3305, 3305, 3305, 3305, 3305, 3353, 3353, 3353, 3353, 3353, 3385, 3385,
    3385, 3385, 3385, 3434, 3434, 3434, 3434, 3434, 3466, 3466, 3466, 3466,
    3466, 3466, 3466, 3466, 3514, 3514, 3514, 3514, 3546, 3546, 3546, 3546,
    3546, 3546, 3595, 3595, 3595, 3595, 3595, 3595, 3595, 3595, 3595, 3595,
    3627, 3627, 3627, 3627, 3627, 3627, 3627, 3675, 3675, 3675, 3675, 3675,
    3675, 3675, 3724, 3724, 3724, 3724, 3724, 3724, 3724, 3724, 3724, 3724,
    3756, 3756, 3756, 3756, 3756, 3756, 3756, 3756, 3756, 3804, 3804, 3804,
    3804, 3804, 3804, 3804, 3804, 3804, 3837, 3837, 3837, 3837, 3837, 3837,
    3837, 3837, 3837, 3837, 3885, 3885, 3885, 3885, 3885, 3885, 3885, 3885,
    3885, 3885, 3917, 3917, 3917, 3917, 3917, 3917, 3917, 3917, 3917, 3917,
    3917, 3917, 3966, 3966, 3966, 3966, 3966, 3966, 3966, 3966, 3966, 3966,
    3966, 3998, 3998, 3998, 3998, 3998, 3998, 3998, 3998, 3998, 3998, 3998,
    3998, 4046, 4046, 4046, 4046, 4046, 4046, 4046, 4046, 4046, 4046, 4046,
    4046, 4046, 4095, 4095
};

static int g_ongoing = ONGOING_NONE;

static uint32_t attentionBrightness = 0;
static uint32_t batteryBrightness = 0;
static uint32_t buttonBrightness = 0;
static uint32_t notificationBrightness = 0;

struct led_data
{
    int blink_mode;
    int min_grade;
    int max_grade;
    int fade_time;
    int fade_on_time;
    int fade_off_time;
};
static std::unordered_map<int, led_data> mLedDatas {
    { ONGOING_NONE, {BLINK_MODE_OFF, -1, -1, -1, -1, -1} },
    { ONGOING_BUTTONS, {BLINK_MODE_OFF, -1, -1, -1, -1, -1} },
    { ONGOING_ATTENTION, {BLINK_MODE_OFF, -1, -1, -1, -1, -1} },
    { ONGOING_NOTIFICATION, {BLINK_MODE_OFF, -1, -1, -1, -1, -1} },
    { ONGOING_BATTERY, {BLINK_MODE_OFF, -1, -1, -1, -1, -1} },
};
static struct led_data current_led_param = mLedDatas[ONGOING_NONE];

typedef void (*LightStateHandler)(const LightState&);

struct LightBackend {
    Type type;
    LightState state;
    LightStateHandler handler;

    LightBackend(Type type, LightStateHandler handler) : type(type), handler(handler) {
        this->state.color = 0xff000000;
    }
};

namespace android {
namespace hardware {
namespace light {
namespace V2_0 {
namespace implementation {

class Light : public ILight {
  public:
    Return<Status> setLight(Type type, const LightState& state) override;
    Return<void> getSupportedTypes(getSupportedTypes_cb _hidl_cb) override;

  private:
    std::mutex globalLock;
};

}  // namespace implementation
}  // namespace V2_0
}  // namespace light
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_LIGHT_V2_0_LIGHT_H
