/*
** Copyright 2023, The LineageOS Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include "Vibrator.h"

#include <android-base/logging.h>

#include <cmath>
#include <fstream>
#include <iostream>
#include <thread>

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

/*
 * Write value to path and close file.
 */
template <typename T>
static ndk::ScopedAStatus writeNode(const std::string& path, const T& value) {
    std::ofstream node(path);
    if (!node) {
        LOG(ERROR) << "Failed to open: " << path;
        return ndk::ScopedAStatus::fromStatus(STATUS_UNKNOWN_ERROR);
    }

    LOG(DEBUG) << "writeNode node: " << path << " value: " << value;

    node << value << std::endl;
    if (!node) {
        LOG(ERROR) << "Failed to write: " << value;
        return ndk::ScopedAStatus::fromStatus(STATUS_UNKNOWN_ERROR);
    }

    return ndk::ScopedAStatus::ok();
}

static bool nodeExists(const std::string& path) {
    std::ofstream f(path.c_str());
    return f.good();
}

Vibrator::Vibrator() {
    mIsTimedOutVibrator = nodeExists(VIBRATOR_TIMEOUT_PATH);
    mHasTimedOutIntensity = nodeExists(VIBRATOR_INTENSITY_PATH);
}

ndk::ScopedAStatus Vibrator::getCapabilities(int32_t* _aidl_return) {
    *_aidl_return = IVibrator::CAP_ON_CALLBACK | IVibrator::CAP_PERFORM_CALLBACK |
                    IVibrator::CAP_EXTERNAL_CONTROL /*| IVibrator::CAP_COMPOSE_EFFECTS |
                    IVibrator::CAP_ALWAYS_ON_CONTROL*/;

    if (mHasTimedOutIntensity) {
        *_aidl_return = *_aidl_return | IVibrator::CAP_AMPLITUDE_CONTROL |
                        IVibrator::CAP_EXTERNAL_AMPLITUDE_CONTROL;
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::off() {
    return activate(0);
}

ndk::ScopedAStatus Vibrator::on(int32_t timeoutMs, const std::shared_ptr<IVibratorCallback>& callback) {
    ndk::ScopedAStatus status = activate(timeoutMs);

    if (callback != nullptr) {
        std::thread([=] {
            LOG(DEBUG) << "Starting on on another thread";
            usleep(timeoutMs * 1000);
            LOG(DEBUG) << "Notifying on complete";
            if (!callback->onComplete().isOk()) {
                LOG(ERROR) << "Failed to call onComplete";
            }
        }).detach();
    }

    return status;
}

ndk::ScopedAStatus Vibrator::perform(Effect effect, EffectStrength strength, const std::shared_ptr<IVibratorCallback>& callback, int32_t* _aidl_return) {
    ndk::ScopedAStatus status;
    float amplitude;
    uint32_t ms;

    amplitude = strengthToAmplitude(strength, &status);
    if (!status.isOk()) {
        return status;
    }
    setAmplitude(amplitude);

    ms = effectToMs(effect, &status);
    if (!status.isOk()) {
        return status;
    }
    status = activate(ms);

    if (callback != nullptr) {
        std::thread([=] {
            LOG(DEBUG) << "Starting perform on another thread";
            usleep(ms * 1000);
            LOG(DEBUG) << "Notifying perform complete";
            callback->onComplete();
        }).detach();
    }

    *_aidl_return = ms;
    return status;
}

ndk::ScopedAStatus Vibrator::getSupportedEffects(std::vector<Effect>* _aidl_return) {
    *_aidl_return = { Effect::CLICK, Effect::TICK };
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::setAmplitude(float amplitude) {
    uint32_t intensity;

    if (amplitude <= 0.0f || amplitude > 1.0f) {
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
    }

    LOG(DEBUG) << "Setting amplitude: " << amplitude;

    intensity = amplitude * INTENSITY_MAX;

    LOG(DEBUG) << "Setting intensity: " << intensity;

    if (mHasTimedOutIntensity) {
        return writeNode(VIBRATOR_INTENSITY_PATH, intensity);
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::setExternalControl(bool enabled) {
    if (mEnabled) {
        LOG(WARNING) << "Setting external control while the vibrator is enabled is "
                        "unsupported!";
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    LOG(INFO) << "ExternalControl: " << mExternalControl << " -> " << enabled;
    mExternalControl = enabled;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::getCompositionDelayMax(int32_t* /*_aidl_return*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getCompositionSizeMax(int32_t* /*_aidl_return*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getSupportedPrimitives(std::vector<CompositePrimitive>* /*_aidl_return*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getPrimitiveDuration(CompositePrimitive /*primitive*/, int32_t* /*_aidl_return*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::compose(const std::vector<CompositeEffect>& /*composite*/, const std::shared_ptr<IVibratorCallback>& /*callback*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getSupportedAlwaysOnEffects(std::vector<Effect>* /*_aidl_return*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::alwaysOnEnable(int32_t /*id*/, Effect /*effect*/, EffectStrength /*strength*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::alwaysOnDisable(int32_t /*id*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getResonantFrequency(float* /*_aidl_return*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getQFactor(float* /*_aidl_return*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getFrequencyResolution(float* /*_aidl_return*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getFrequencyMinimum(float* /*_aidl_return*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getBandwidthAmplitudeMap(std::vector<float>* /*_aidl_return*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getPwlePrimitiveDurationMax(int32_t* /*_aidl_return*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getPwleCompositionSizeMax(int32_t* /*_aidl_return*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getSupportedBraking(std::vector<Braking>* /*_aidl_return*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::composePwle(const std::vector<PrimitivePwle>& /*composite*/, const std::shared_ptr<IVibratorCallback>& /*callback*/) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::activate(uint32_t timeoutMs) {
    std::lock_guard<std::mutex> lock{mMutex};
    if (!mIsTimedOutVibrator) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    return writeNode(VIBRATOR_TIMEOUT_PATH, timeoutMs);
}

float Vibrator::strengthToAmplitude(EffectStrength strength, ndk::ScopedAStatus* status) {
    *status = ndk::ScopedAStatus::ok();

    switch (strength) {
        case EffectStrength::LIGHT:
            return AMPLITUDE_LIGHT;
        case EffectStrength::MEDIUM:
            return AMPLITUDE_MEDIUM;
        case EffectStrength::STRONG:
            return AMPLITUDE_STRONG;
    }

    *status = ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    return 0;
}

uint32_t Vibrator::effectToMs(Effect effect, ndk::ScopedAStatus* status) {
    *status = ndk::ScopedAStatus::ok();

    switch (effect) {
        case Effect::CLICK:
            return 20;
        case Effect::TICK:
            return 15;
        default:
            break;
    }

    *status = ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    return 0;
}

} // namespace vibrator
} // namespace hardware
} // namespace android
} // namespace aidl
