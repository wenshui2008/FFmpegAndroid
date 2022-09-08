//
// Created by xu fulong on 2022/9/4.
//

#include "ff_audio_player.h"
#include <jni.h>
#include <unistd.h>

#define SLEEP_TIME (16000)

FFAudioPlayer *audioPlayer;

AUDIO_PLAYER_FUNC(void, play, jstring path) {
    if (path == nullptr)
        return;

    int result = 0;
    const char* native_path = env->GetStringUTFChars(path, JNI_FALSE);
    audioPlayer = new FFAudioPlayer();
    // open stream, and init work
    audioPlayer->open(native_path);
    // init AudioTrack
    jclass audio_class = env->GetObjectClass(thiz);
    jmethodID audio_track_method = env->GetMethodID(audio_class,
                                                    "createAudioTrack", "(II)Landroid/media/AudioTrack;");
    jobject audio_track = env->CallObjectMethod(thiz, audio_track_method,
                                                audioPlayer->getSampleRate(), audioPlayer->getChannel());
    // play function
    jclass audio_track_class = env->GetObjectClass(audio_track);
    jmethodID play_method = env->GetMethodID(audio_track_class, "play", "()V");
    env->CallVoidMethod(audio_track, play_method);
    // method if of write
    jmethodID write_method = env->GetMethodID(audio_track_class, "write", "([BII)I");

    // demux decode and play
    while (result >= 0) {
        result = audioPlayer->decodeAudio();
        if (result == 0) {
            continue;
        } else if (result < 0) {
            break;
        }
        int size = result;
        // call AudioTrack to play(should be reused array)
        jbyteArray audio_array = env->NewByteArray(size);
        jbyte *data_address = env->GetByteArrayElements(audio_array, JNI_FALSE);
        memcpy(data_address, audioPlayer->getDecodeFrame(), size);
        env->ReleaseByteArrayElements(audio_array, data_address, 0);
        env->CallIntMethod(audio_track, write_method, audio_array, 0, size);
        env->DeleteLocalRef(audio_array);

        // audio sync
        usleep(SLEEP_TIME);
    }

    env->ReleaseStringUTFChars(path, native_path);
    jmethodID release_method = env->GetMethodID(audio_class, "releaseAudioTrack", "()V");
    env->CallVoidMethod(thiz, release_method);
    audioPlayer->close();
    delete audioPlayer;
}

AUDIO_PLAYER_FUNC(void, again, jstring filter_jstr) {
    if (!filter_jstr) return;
    audioPlayer->setFilterAgain(true);
    const char *desc = env->GetStringUTFChars(filter_jstr, nullptr);
    audioPlayer->setFilterDesc(desc);
}
