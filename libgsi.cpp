//
// Copyright (C) 2019 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "libgsi/libgsi.h"

#include <string.h>
#include <unistd.h>

#include <string>

#include <android-base/file.h>
#include <android-base/parseint.h>

#include "file_paths.h"
#include "libgsi_private.h"

namespace android {
namespace gsi {

using namespace std::literals;

bool IsGsiRunning() {
    return !access(kGsiBootedIndicatorFile, F_OK);
}

bool IsGsiInstalled() {
    return !access(kGsiInstallStatusFile, F_OK);
}

static bool CanBootIntoGsi(std::string* error) {
    if (!IsGsiInstalled()) {
        *error = "not detected";
        return false;
    }

    std::string boot_key;
    if (!GetInstallStatus(&boot_key)) {
        *error = "error ("s + strerror(errno) + ")";
        return false;
    }

    // Give up if we've failed to boot kMaxBootAttempts times.
    int attempts;
    if (GetBootAttempts(boot_key, &attempts)) {
        if (attempts + 1 >= kMaxBootAttempts) {
            *error = "exceeded max boot attempts";
            return false;
        }
        std::string new_key = std::to_string(attempts + 1);
        if (!android::base::WriteStringToFile(new_key, kGsiInstallStatusFile)) {
            *error = "error ("s + strerror(errno) + ")";
            return false;
        }
        return true;
    }

    return boot_key == kInstallStatusOk;
}

bool CanBootIntoGsi(std::string* metadata_file, std::string* error) {
    // Always delete this as a safety precaution, so we can return to the
    // original system image. If we're confident GSI will boot, this will
    // get re-created by MarkSystemAsGsi.
    android::base::RemoveFileIfExists(kGsiBootedIndicatorFile);

    if (!CanBootIntoGsi(error)) {
        return false;
    }

    *metadata_file = kGsiMetadata;
    return true;
}

bool UninstallGsi() {
    if (!android::base::WriteStringToFile(kInstallStatusWipe, kGsiInstallStatusFile)) {
        return false;
    }
    return true;
}

bool MarkSystemAsGsi() {
    return android::base::WriteStringToFile("1", kGsiBootedIndicatorFile);
}

bool GetInstallStatus(std::string* status) {
    return android::base::ReadFileToString(kGsiInstallStatusFile, status);
}

bool GetBootAttempts(const std::string& boot_key, int* attempts) {
    return android::base::ParseInt(boot_key, attempts);
}

}  // namespace gsi
}  // namespace android
