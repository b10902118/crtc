#include <stdint.h>

extern "C" {

int FMOD_Debug_Initialize(void* a, void* b, void* c, void* d) { return 0; }

int FMOD_System_Create(void** system, unsigned int headerversion) {
    if (system) {
        *system = (void*)0x1234;
    }
    return 0;
}

int _ZN4FMOD12ChannelGroup7releaseEv(void* self) { return 0; }
int _ZN4FMOD14ChannelControl11getUserDataEPPv(void* self, void** userdata) {
    if (userdata) *userdata = nullptr;
    return 0;
}
int _ZN4FMOD14ChannelControl11setUserDataEPv(void* self, void* userdata) { return 0; }
int _ZN4FMOD14ChannelControl4stopEv(void* self) { return 0; }
int _ZN4FMOD14ChannelControl9isPlayingEPb(void* self, bool* playing) {
    if (playing) *playing = false;
    return 0;
}
int _ZN4FMOD14ChannelControl9setPausedEb(void* self, bool paused) { return 0; }
int _ZN4FMOD14ChannelControl9setVolumeEf(void* self, float volume) { return 0; }
int _ZN4FMOD5Sound7releaseEv(void* self) { return 0; }
int _ZN4FMOD5Sound9getLengthEPjj(void* self, unsigned int* length, unsigned int lengthtype) {
    if (length) *length = 60000; // 60 seconds
    return 0;
}
int _ZN4FMOD6System11createSoundEPKcjP22FMOD_CREATESOUNDEXINFOPPNS_5SoundE(void* self, const char* name, unsigned int mode, void* exinfo, void** sound) {
    if (sound) *sound = (void*)0x2345;
    return 0;
}
int _ZN4FMOD6System11mixerResumeEv(void* self) { return 0; }
int _ZN4FMOD6System12createStreamEPKcjP22FMOD_CREATESOUNDEXINFOPPNS_5SoundE(void* self, const char* name, unsigned int mode, void* exinfo, void** sound) {
    if (sound) *sound = (void*)0x3456;
    return 0;
}
int _ZN4FMOD6System12mixerSuspendEv(void* self) { return 0; }
int _ZN4FMOD6System13getNumDriversEPi(void* self, int* numdrivers) {
    if (numdrivers) *numdrivers = 1;
    return 0;
}
int _ZN4FMOD6System13setFileSystemEPF11FMOD_RESULTPKcPjPPvS5_EPFS1_S5_S5_EPFS1_S5_S5_jS4_S5_EPFS1_S5_jS5_EPFS1_P18FMOD_ASYNCREADINFOS5_ESI_i(void* self, void* a, void* b, void* c, void* d, void* e, int f) {
    return 0;
}
int _ZN4FMOD6System16setDSPBufferSizeEji(void* self, unsigned int bufferlength, int numbuffers) { return 0; }
int _ZN4FMOD6System18createChannelGroupEPKcPPNS_12ChannelGroupE(void* self, const char* name, void** group) {
    if (group) *group = (void*)0x4567;
    return 0;
}
int _ZN4FMOD6System19setStreamBufferSizeEjj(void* self, unsigned int filebuffersize, unsigned int type) { return 0; }
int _ZN4FMOD6System4initEijPv(void* self, int maxchannels, unsigned int flags, void* extradriverdata) { return 0; }
int _ZN4FMOD6System5closeEv(void* self) { return 0; }
int _ZN4FMOD6System6updateEv(void* self) { return 0; }
int _ZN4FMOD6System7releaseEv(void* self) { return 0; }
int _ZN4FMOD6System9playSoundEPNS_5SoundEPNS_12ChannelGroupEbPPNS_7ChannelE(void* self, void* sound, void* group, bool paused, void** channel) {
    if (channel) *channel = (void*)0x5678;
    return 0;
}
int _ZN4FMOD6System9setOutputE15FMOD_OUTPUTTYPE(void* self, int outputtype) { return 0; }
int _ZN4FMOD7Channel11setPositionEjj(void* self, unsigned int position, unsigned int postype) { return 0; }
int _ZN4FMOD7Channel11setPriorityEi(void* self, int priority) { return 0; }
int _ZN4FMOD7Channel12getFrequencyEPf(void* self, float* frequency) {
    if (frequency) *frequency = 44100.0f;
    return 0;
}
int _ZN4FMOD7Channel12setFrequencyEf(void* self, float frequency) { return 0; }
int _ZN4FMOD7Channel12setLoopCountEi(void* self, int loopcount) { return 0; }
int _ZN4FMOD7Channel15setChannelGroupEPNS_12ChannelGroupE(void* self, void* group) { return 0; }

}
