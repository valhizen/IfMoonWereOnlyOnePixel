#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "AudioSystem.hpp"
#include <iostream>

struct AudioSystem::InternalData {
    ma_engine engine;
    ma_sound soundRocket;
    ma_sound soundLaunch;
    ma_sound soundAmbience;
};

AudioSystem::AudioSystem() : m_Data(new InternalData()), m_Muted(false), m_EngineActive(false) {
}

AudioSystem::~AudioSystem() {
    if (m_Data) {
        ma_sound_uninit(&m_Data->soundRocket);
        ma_sound_uninit(&m_Data->soundLaunch);
        ma_sound_uninit(&m_Data->soundAmbience);
        ma_engine_uninit(&m_Data->engine);
        delete m_Data;
    }
}

bool AudioSystem::init() {
    ma_result result;
    result = ma_engine_init(NULL, &m_Data->engine);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to initialize audio engine." << std::endl;
        return false;
    }
    return true;
}

void AudioSystem::loadSounds() {
    ma_result result;
    
    // Load Engine Sound (Looping)
    result = ma_sound_init_from_file(&m_Data->engine, "assets/large-rocket-engine-86240.mp3", 0, NULL, NULL, &m_Data->soundRocket);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to load rocket sound." << std::endl;
    } else {
        ma_sound_set_looping(&m_Data->soundRocket, MA_TRUE);
        ma_sound_set_volume(&m_Data->soundRocket, 0.0f); // Start silent until moving
        ma_sound_start(&m_Data->soundRocket);
    }
    
    // Load Launch Sound
    result = ma_sound_init_from_file(&m_Data->engine, "assets/launch_audio.mp3", 0, NULL, NULL, &m_Data->soundLaunch);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to load launch sound." << std::endl;
    } else {
        ma_sound_set_volume(&m_Data->soundLaunch, 1.0f);
    }
    
    // Load Ambience Sound (Looping)
    result = ma_sound_init_from_file(&m_Data->engine, "assets/space-vessel-background-noise-350616.mp3", 0, NULL, NULL, &m_Data->soundAmbience);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to load ambience sound." << std::endl;
    } else {
        ma_sound_set_looping(&m_Data->soundAmbience, MA_TRUE);
        ma_sound_set_volume(&m_Data->soundAmbience, 0.5f);
        ma_sound_start(&m_Data->soundAmbience);
    }
}

void AudioSystem::playLaunch() {
    if (m_Muted || !m_Data) return;
    ma_sound_seek_to_pcm_frame(&m_Data->soundLaunch, 0);
    ma_sound_start(&m_Data->soundLaunch);
}

void AudioSystem::setEngineActive(bool active) {
    m_EngineActive = active;
    if (m_Muted) return;
    
    if (m_Data) {
        // Fade in/out could be better, but direct set for now
        float targetVol = active ? 0.8f : 0.0f;
        // Basic interpolation could be done in update(), but instant for now
        ma_sound_set_volume(&m_Data->soundRocket, targetVol);
    }
}

void AudioSystem::setAmbienceActive(bool active) {
    if (m_Muted) return;
    if (m_Data) {
        float targetVol = active ? 0.5f : 0.0f;
        ma_sound_set_volume(&m_Data->soundAmbience, targetVol);
    }
}

void AudioSystem::toggleMute() {
    m_Muted = !m_Muted;
    if (m_Data) {
        if (m_Muted) {
            ma_sound_set_volume(&m_Data->soundRocket, 0.0f);
            ma_sound_set_volume(&m_Data->soundAmbience, 0.0f);
        } else {
            // Restore
            ma_sound_set_volume(&m_Data->soundRocket, m_EngineActive ? 0.8f : 0.0f);
            ma_sound_set_volume(&m_Data->soundAmbience, 0.5f);
        }
    }
}

void AudioSystem::setMasterVolume(float volume) {
    if (m_Data) {
        ma_engine_set_volume(&m_Data->engine, volume);
    }
}
