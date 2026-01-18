#ifndef AUDIOSYSTEM_HPP
#define AUDIOSYSTEM_HPP

class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem();
    
    bool init();
    void loadSounds();
    
    // Controls
    void playLaunch();
    void setEngineActive(bool active);
    void setAmbienceActive(bool active);
    void setMasterVolume(float volume);
    void toggleMute();
    
    bool isMuted() const { return m_Muted; }

private:
    struct InternalData;
    InternalData* m_Data;
    
    bool m_Muted;
    bool m_EngineActive;
};

#endif
