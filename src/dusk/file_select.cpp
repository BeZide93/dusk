#include "file_select.hpp"

#include "dusk/io.hpp"

#include <memory>
#include <stdexcept>
#include <string_view>

#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_stdinc.h>

#if defined(__ANDROID__) || defined(ANDROID)
#include <SDL3/SDL_system.h>
#include <jni.h>
#endif

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#if defined(__APPLE__) && TARGET_OS_IOS && !TARGET_OS_MACCATALYST
#define USE_IOS_DIALOG 1
#include "ios/FileSelectDialog.h"
#else
#define USE_IOS_DIALOG 0
#endif

namespace dusk {
namespace {

std::string fallback_display_name(std::string_view path) {
    if (path.empty()) {
        return {};
    }

    std::string pathString(path);
    const std::size_t slash = pathString.find_last_of("/\\");
    if (slash == std::string::npos || slash + 1 >= pathString.size()) {
        return pathString;
    }
    return pathString.substr(slash + 1);
}

#if defined(__ANDROID__) || defined(ANDROID)
bool clear_pending_exception(JNIEnv* env) {
    if (env == nullptr || !env->ExceptionCheck()) {
        return false;
    }
    env->ExceptionClear();
    return true;
}

std::string to_string(JNIEnv* env, jstring value) {
    if (env == nullptr || value == nullptr) {
        return {};
    }

    const char* utf8 = env->GetStringUTFChars(value, nullptr);
    if (utf8 == nullptr) {
        clear_pending_exception(env);
        return {};
    }

    std::string result(utf8);
    env->ReleaseStringUTFChars(value, utf8);
    return result;
}

std::string android_display_name(std::string_view path) {
    auto* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    if (env == nullptr) {
        return {};
    }

    jobject activity = static_cast<jobject>(SDL_GetAndroidActivity());
    if (activity == nullptr || clear_pending_exception(env)) {
        if (activity != nullptr) {
            env->DeleteLocalRef(activity);
        }
        return {};
    }

    jclass activityClass = env->GetObjectClass(activity);
    if (activityClass == nullptr || clear_pending_exception(env)) {
        env->DeleteLocalRef(activity);
        return {};
    }

    jmethodID getDisplayName = env->GetMethodID(
        activityClass, "getDisplayNameForUri", "(Ljava/lang/String;)Ljava/lang/String;");
    env->DeleteLocalRef(activityClass);
    if (getDisplayName == nullptr || clear_pending_exception(env)) {
        env->DeleteLocalRef(activity);
        return {};
    }

    jstring uri = env->NewStringUTF(std::string(path).c_str());
    if (uri == nullptr || clear_pending_exception(env)) {
        env->DeleteLocalRef(activity);
        return {};
    }

    auto* displayName =
        static_cast<jstring>(env->CallObjectMethod(activity, getDisplayName, uri));
    env->DeleteLocalRef(uri);
    env->DeleteLocalRef(activity);
    if (displayName == nullptr || clear_pending_exception(env)) {
        return {};
    }

    std::string result = to_string(env, displayName);
    env->DeleteLocalRef(displayName);
    return result;
}

std::string android_read_uri_text(std::string_view path) {
    auto* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    if (env == nullptr) {
        throw std::runtime_error("Android JNI environment is unavailable.");
    }

    jobject activity = static_cast<jobject>(SDL_GetAndroidActivity());
    if (activity == nullptr || clear_pending_exception(env)) {
        if (activity != nullptr) {
            env->DeleteLocalRef(activity);
        }
        throw std::runtime_error("Android activity is unavailable.");
    }

    jclass activityClass = env->GetObjectClass(activity);
    if (activityClass == nullptr || clear_pending_exception(env)) {
        env->DeleteLocalRef(activity);
        throw std::runtime_error("Android activity class is unavailable.");
    }

    jmethodID readBytes = env->GetMethodID(
        activityClass, "readBytesForUri", "(Ljava/lang/String;)[B");
    env->DeleteLocalRef(activityClass);
    if (readBytes == nullptr || clear_pending_exception(env)) {
        env->DeleteLocalRef(activity);
        throw std::runtime_error("Android URI reader is unavailable.");
    }

    const std::string pathString(path);
    jstring uri = env->NewStringUTF(pathString.c_str());
    if (uri == nullptr || clear_pending_exception(env)) {
        env->DeleteLocalRef(activity);
        throw std::runtime_error("Unable to create Android URI string.");
    }

    auto* bytes = static_cast<jbyteArray>(env->CallObjectMethod(activity, readBytes, uri));
    env->DeleteLocalRef(uri);
    env->DeleteLocalRef(activity);
    if (bytes == nullptr || clear_pending_exception(env)) {
        throw std::runtime_error("Unable to read selected file.");
    }

    const jsize size = env->GetArrayLength(bytes);
    std::string result(static_cast<size_t>(size), '\0');
    if (size > 0) {
        env->GetByteArrayRegion(bytes, 0, size, reinterpret_cast<jbyte*>(result.data()));
        if (clear_pending_exception(env)) {
            env->DeleteLocalRef(bytes);
            throw std::runtime_error("Unable to copy selected file data.");
        }
    }
    env->DeleteLocalRef(bytes);
    return result;
}

void android_write_uri_text(std::string_view path, std::string_view text) {
    auto* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    if (env == nullptr) {
        throw std::runtime_error("Android JNI environment is unavailable.");
    }

    jobject activity = static_cast<jobject>(SDL_GetAndroidActivity());
    if (activity == nullptr || clear_pending_exception(env)) {
        if (activity != nullptr) {
            env->DeleteLocalRef(activity);
        }
        throw std::runtime_error("Android activity is unavailable.");
    }

    jclass activityClass = env->GetObjectClass(activity);
    if (activityClass == nullptr || clear_pending_exception(env)) {
        env->DeleteLocalRef(activity);
        throw std::runtime_error("Android activity class is unavailable.");
    }

    jmethodID writeBytes =
        env->GetMethodID(activityClass, "writeBytesForUri", "(Ljava/lang/String;[B)V");
    env->DeleteLocalRef(activityClass);
    if (writeBytes == nullptr || clear_pending_exception(env)) {
        env->DeleteLocalRef(activity);
        throw std::runtime_error("Android URI writer is unavailable.");
    }

    const std::string pathString(path);
    jstring uri = env->NewStringUTF(pathString.c_str());
    if (uri == nullptr || clear_pending_exception(env)) {
        env->DeleteLocalRef(activity);
        throw std::runtime_error("Unable to create Android URI string.");
    }

    auto* bytes = env->NewByteArray(static_cast<jsize>(text.size()));
    if (bytes == nullptr || clear_pending_exception(env)) {
        env->DeleteLocalRef(uri);
        env->DeleteLocalRef(activity);
        throw std::runtime_error("Unable to allocate selected file data.");
    }
    if (!text.empty()) {
        env->SetByteArrayRegion(bytes, 0, static_cast<jsize>(text.size()),
            reinterpret_cast<const jbyte*>(text.data()));
        if (clear_pending_exception(env)) {
            env->DeleteLocalRef(bytes);
            env->DeleteLocalRef(uri);
            env->DeleteLocalRef(activity);
            throw std::runtime_error("Unable to copy selected file data.");
        }
    }

    env->CallVoidMethod(activity, writeBytes, uri, bytes);
    env->DeleteLocalRef(bytes);
    env->DeleteLocalRef(uri);
    env->DeleteLocalRef(activity);
    if (clear_pending_exception(env)) {
        throw std::runtime_error("Unable to write selected file.");
    }
}

void android_set_next_file_dialog_default_name(std::string_view defaultLocation) {
    std::string defaultName = fallback_display_name(defaultLocation);
    if (defaultName.empty()) {
        return;
    }

    auto* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    if (env == nullptr) {
        return;
    }

    jobject activity = static_cast<jobject>(SDL_GetAndroidActivity());
    if (activity == nullptr || clear_pending_exception(env)) {
        if (activity != nullptr) {
            env->DeleteLocalRef(activity);
        }
        return;
    }

    jclass activityClass = env->GetObjectClass(activity);
    if (activityClass == nullptr || clear_pending_exception(env)) {
        env->DeleteLocalRef(activity);
        return;
    }

    jmethodID setDefaultName = env->GetMethodID(
        activityClass, "setNextFileDialogDefaultName", "(Ljava/lang/String;)V");
    env->DeleteLocalRef(activityClass);
    if (setDefaultName == nullptr || clear_pending_exception(env)) {
        env->DeleteLocalRef(activity);
        return;
    }

    jstring name = env->NewStringUTF(defaultName.c_str());
    if (name == nullptr || clear_pending_exception(env)) {
        env->DeleteLocalRef(activity);
        return;
    }

    env->CallVoidMethod(activity, setDefaultName, name);
    env->DeleteLocalRef(name);
    env->DeleteLocalRef(activity);
    clear_pending_exception(env);
}
#endif

#if USE_IOS_DIALOG
struct IOSDialogCallbackState {
    FileCallback callback;
    void* userdata;
};

void onIOSDialogFinished(void* userdata, const char* path, const char* error) {
    std::unique_ptr<IOSDialogCallbackState> state(static_cast<IOSDialogCallbackState*>(userdata));

    if (error != nullptr) {
        state->callback(state->userdata, nullptr, error);
        return;
    }

    if (path == nullptr) {
        state->callback(state->userdata, nullptr, nullptr);
        return;
    }

    state->callback(state->userdata, path, nullptr);
}
#else
struct SDLDialogCallbackState {
    FileCallback callback;
    void* userdata;
};

void onSDLDialogFinished(void* userdata, const char* const* filelist, [[maybe_unused]] int filter) {
    std::unique_ptr<SDLDialogCallbackState> state(static_cast<SDLDialogCallbackState*>(userdata));

    if (filelist == nullptr) {
        state->callback(state->userdata, nullptr, SDL_GetError());
        return;
    }

    if (filelist[0] == nullptr) {
        state->callback(state->userdata, nullptr, nullptr);
        return;
    }

    state->callback(state->userdata, filelist[0], nullptr);
}
#endif

}  // namespace

void ShowFileSelect(FileCallback callback, void* userdata, SDL_Window* window,
                    const SDL_DialogFileFilter* filters, int nfilters, const char* default_location,
                    bool allow_many) {
    if (callback == nullptr) {
        return;
    }

#if USE_IOS_DIALOG
    auto state = std::make_unique<IOSDialogCallbackState>();
    state->callback = callback;
    state->userdata = userdata;

    Dusk_iOS_ShowFileSelect(&onIOSDialogFinished, state.release(), window, filters, nfilters,
                            default_location, allow_many);
#else
    auto state = std::make_unique<SDLDialogCallbackState>();
    state->callback = callback;
    state->userdata = userdata;

    SDL_ShowOpenFileDialog(&onSDLDialogFinished, state.release(), window, filters, nfilters,
                           default_location, allow_many);
#endif
}

void ShowFileSave(FileCallback callback, void* userdata, SDL_Window* window,
                  const SDL_DialogFileFilter* filters, int nfilters,
                  const char* default_location) {
    if (callback == nullptr) {
        return;
    }

#if USE_IOS_DIALOG
    callback(userdata, nullptr, "Save dialog is not supported on this platform.");
#else
    auto state = std::make_unique<SDLDialogCallbackState>();
    state->callback = callback;
    state->userdata = userdata;

#if defined(__ANDROID__) || defined(ANDROID)
    android_set_next_file_dialog_default_name(default_location != nullptr ? default_location : "");
#endif
    SDL_ShowSaveFileDialog(&onSDLDialogFinished, state.release(), window, filters, nfilters,
                           default_location);
#endif
}

std::string display_name_for_path(std::string_view path) {
#if defined(__ANDROID__) || defined(ANDROID)
    if (path.starts_with("content:") || path.starts_with("file:")) {
        std::string displayName = android_display_name(path);
        if (!displayName.empty()) {
            return displayName;
        }
    }
#endif
    return fallback_display_name(path);
}

std::string read_file_select_text(std::string_view path) {
#if defined(__ANDROID__) || defined(ANDROID)
    if (path.starts_with("content:") || path.starts_with("file:")) {
        return android_read_uri_text(path);
    }
#endif

    const std::string pathString(path);
    const auto data = io::FileStream::ReadAllBytes(pathString.c_str());
    if (data.empty()) {
        return {};
    }
    return {reinterpret_cast<const char*>(data.data()), data.size()};
}

void write_file_select_text(std::string_view path, std::string_view text) {
#if defined(__ANDROID__) || defined(ANDROID)
    if (path.starts_with("content:") || path.starts_with("file:")) {
        android_write_uri_text(path, text);
        return;
    }
#endif

    const std::string pathString(path);
    io::FileStream::WriteAllText(pathString.c_str(), text);
}
}  // namespace dusk
