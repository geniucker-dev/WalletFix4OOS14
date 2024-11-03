/* Copyright 2022-2023 John "topjohnwu" Wu
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <string>
#include <vector>
#include <string_view>
#include <unordered_map>
#include <filesystem>
#include <unistd.h>
#include <fcntl.h>
#include <android/log.h>
#include <sys/system_properties.h>

#include "zygisk.hpp"


using zygisk::Api;
using zygisk::AppSpecializeArgs;
using zygisk::ServerSpecializeArgs;

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "WalletFix4OOS14", __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "WalletFix4OOS14", __VA_ARGS__)

#define CONFIG_FILE "/data/adb/walletfix/spoof_vars"

#define DEFAULT_CONFIG_FILE "/data/adb/modules/walletfix4oos14/spoof_vars"

ssize_t xread(int fd, void *buffer, size_t count) {
    LOGD("xread, fd: %d, count: %zu", fd, count);
    ssize_t total = 0;
    char *buf = (char *)buffer;
    while (count > 0) {
        ssize_t ret = read(fd, buf, count);
        if (ret < 0) return -1;
        buf += ret;
        total += ret;
        count -= ret;
    }
    return total;
}

ssize_t xwrite(int fd, const void *buffer, size_t count) {
    LOGD("xwrite, fd: %d, count: %zu", fd, count);
    ssize_t total = 0;
    char *buf = (char *)buffer;
    while (count > 0) {
        ssize_t ret = write(fd, buf, count);
        if (ret < 0) return -1;
        buf += ret;
        total += ret;
        count -= ret;
    }
    return total;
}

std::vector<std::string> split(const std::string& str, const std::string& delimiter) {
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t end = str.find(delimiter);

    while (end != std::string::npos) {
        tokens.push_back(str.substr(start, end - start));
        start = end + delimiter.length();
        end = str.find(delimiter, start);
    }

    tokens.push_back(str.substr(start));
    return tokens;
}

std::string trim(const std::string& str) {
    // remove spaces, tabs, newlines
    size_t start = str.find_first_not_of(" \t\n\r");
    size_t end = str.find_last_not_of(" \t\n\r");
    if (start == std::string::npos || end == std::string::npos || start > end) {
        return "";
    }
    return str.substr(start, end - start + 1);
}

class MyModule : public zygisk::ModuleBase {
public:
    void onLoad(Api *api, JNIEnv *env) override {
        this->api = api;
        this->env = env;
    }

    void preAppSpecialize(AppSpecializeArgs *args) override {
        LOGD("preAppSpecialize");
        api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);

        if (!args || !args->app_data_dir) {
            return;
        }

        auto app_data_dir = env->GetStringUTFChars(args->app_data_dir, nullptr);
        auto nice_name = env->GetStringUTFChars(args->nice_name, nullptr);

        std::string_view dir(app_data_dir);
        std::string_view process(nice_name);

        LOGD("process: %s", process.data());
        if (!process.starts_with("com.finshell.wallet")) {
            return;
        }

        int fd = api->connectCompanion();
        LOGD("connectCompanion: %d", fd);
        int configSize;
        std::string configStr;
        xread(fd, &configSize, sizeof(configSize));
        if (configSize > 0) {
            configStr.resize(configSize);
            xread(fd, configStr.data(), configSize * sizeof(uint8_t));
            // Parse config
            LOGD("Config: %s", configStr.c_str());
            LOGD("Parsing config");
            auto lines = split(configStr, "\n");
            LOGD("Parsed %zu lines", lines.size());
            for (auto &line: lines) {
                // skip empty lines
                LOGD("Line: %s", line.c_str());
                LOGD("1");
                if (trim(line).empty()) {
                    LOGD("Empty line");
                    continue;
                }
                LOGD("2");
                auto parts = split(line, "=");
                LOGD("3");
                if (parts.size() != 2) {
                    LOGD("Invalid line");
                    continue;
                }
                LOGD("4");
                auto key = trim(parts[0]);
                auto value = trim(parts[1]);
                spoofVars[key] = value;
                LOGD("Parsed: %s=%s", key.c_str(), value.c_str());
            }
            LOGD("Parsed %zu spoof vars", spoofVars.size());
            
        }
        close(fd);
        LOGD("Close companion, fd: %d", fd);
        LOGI("Config file size: %d", configSize);

        UpdateBuildFields();

        LOGI("Spoofed build fields updated");

        spoofVars.clear();
    }

    void preServerSpecialize(ServerSpecializeArgs *args) override {
        LOGD("preServerSpecialize");
        api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
    }

private:
    Api *api;
    JNIEnv *env;
    std::unordered_map<std::string, std::string> spoofVars;

    void UpdateBuildFields() {
        LOGD("UpdateBuildFields");
        jclass buildClass = env->FindClass("android/os/Build");
        LOGD("buildClass: %p", buildClass);
        jclass versionClass = env->FindClass("android/os/Build$VERSION");
        LOGD("versionClass: %p", versionClass);

        for (auto &[key, val]: spoofVars) {
            const char *fieldName = key.c_str();

            jfieldID fieldID = env->GetStaticFieldID(buildClass, fieldName, "Ljava/lang/String;");

            if (env->ExceptionCheck()) {
                env->ExceptionClear();

                fieldID = env->GetStaticFieldID(versionClass, fieldName, "Ljava/lang/String;");

                if (env->ExceptionCheck()) {
                    env->ExceptionClear();
                    continue;
                }
            }

            if (fieldID != nullptr) {
                const char *value = val.c_str();
                jstring jValue = env->NewStringUTF(value);

                env->SetStaticObjectField(buildClass, fieldID, jValue);
                if (env->ExceptionCheck()) {
                    env->ExceptionClear();
                    continue;
                }

                LOGI("Set '%s' to '%s'", fieldName, value);
            }
        }
    }
};

static std::vector<uint8_t> readFile(const char *path) {
    FILE *file = fopen(path, "rb");

    if (!file) return {};

    int size = static_cast<int>(std::filesystem::file_size(path));

    std::vector<uint8_t> vector(size);

    fread(vector.data(), 1, size, file);

    fclose(file);

    return vector;
}

static void companion_handler(int fd) {
    LOGD("companion_handler, fd: %d", fd);
    std::vector<uint8_t> config_data;

    config_data = readFile(CONFIG_FILE);
    LOGD("config_data size: %zu", config_data.size());
    if (config_data.empty()) {
        LOGD("Using default config file");
        config_data = readFile(DEFAULT_CONFIG_FILE);
    }

    int configSize = static_cast<int>(config_data.size());

    xwrite(fd, &configSize, sizeof(configSize));

    if (configSize > 0) {
        xwrite(fd, config_data.data(), configSize * sizeof(uint8_t));
    }
    LOGD("companion_handler done, fd: %d", fd);
}

// Register our module class and the companion handler function
REGISTER_ZYGISK_MODULE(MyModule)
REGISTER_ZYGISK_COMPANION(companion_handler)
