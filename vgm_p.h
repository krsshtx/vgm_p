

struct VGMHeader
{
    u_int32_t indent;
    u_int32_t EoF;
    u_int32_t version;
    u_int32_t sn76489Clock;
    u_int32_t ym2413Clock;
    u_int32_t gd3Offset;
    u_int32_t totalSamples;
    u_int32_t loopOffset;
    u_int32_t loopNumSamples;
    u_int32_t rate;
    u_int32_t snX;
    u_int32_t ym2612Clock;
    u_int32_t ym2151Clock;
    u_int32_t vgmDataOffset;
    u_int32_t segaPCMClock;
    u_int32_t spcmInterface;
    void Reset()
    {
        indent = 0;
        EoF = 0;
        version = 0;
        sn76489Clock = 0;
        ym2413Clock = 0;
        gd3Offset = 0;
        totalSamples = 0;
        loopOffset = 0;
        loopNumSamples = 0;
        rate = 0;
        snX = 0;
        ym2612Clock = 0;
        ym2151Clock = 0;
        vgmDataOffset = 0;
        segaPCMClock = 0;
        spcmInterface = 0;
    }
};
 
