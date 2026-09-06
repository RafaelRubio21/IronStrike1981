#pragma once
#include "raylib.h"

// Um Sound do raylib é UM canal só: chamar PlaySound nele de novo reinicia o
// mesmo buffer. Com um único Sound compartilhado, dois tanques atirando juntos
// cortavam o som um do outro, e a metralhadora (um tiro a cada 0.08s) cortava
// a si mesma o tempo todo.
//
// O pool carrega o arquivo UMA vez e cria vozes extras com LoadSoundAlias, que
// compartilham as mesmas amostras — ou seja, N vozes não custam N vezes a
// memória do áudio, só o canal de reprodução.
class SoundPool
{
public:
    static constexpr int MAX_VOICES = 8;

    // voices = quantas cópias do mesmo som podem soar ao mesmo tempo.
    // Arquivo inexistente não é erro: o pool simplesmente fica mudo.
    void Load(const char* path, int voices)
    {
        Unload();

        source = LoadSound(path);
        if (source.frameCount == 0) return;

        if (voices < 1) voices = 1;
        if (voices > MAX_VOICES) voices = MAX_VOICES;

        voiceList[0] = source; // a voz 0 é o próprio som carregado
        for (int i = 1; i < voices; i++)
        {
            voiceList[i] = LoadSoundAlias(source);
        }
        voiceCount = voices;
    }

    bool IsLoaded() const { return voiceCount > 0; }

    // Toca na primeira voz livre. Com todas ocupadas, reaproveita a próxima da
    // fila: cortar um som antigo é melhor do que engolir o novo.
    // O volume vai por chamada porque tipos diferentes podem dividir o mesmo
    // pool com volumes diferentes (é o caso das explosões).
    void Play(float volume)
    {
        if (voiceCount == 0) return;

        for (int i = 0; i < voiceCount; i++)
        {
            const int v = (nextVoice + i) % voiceCount;
            if (!IsSoundPlaying(voiceList[v]))
            {
                nextVoice = (v + 1) % voiceCount;
                SetSoundVolume(voiceList[v], volume);
                PlaySound(voiceList[v]);
                return;
            }
        }

        SetSoundVolume(voiceList[nextVoice], volume);
        PlaySound(voiceList[nextVoice]);
        nextVoice = (nextVoice + 1) % voiceCount;
    }

    bool IsAnyPlaying() const
    {
        for (int i = 0; i < voiceCount; i++)
        {
            if (IsSoundPlaying(voiceList[i])) return true;
        }
        return false;
    }

    void Stop()
    {
        for (int i = 0; i < voiceCount; i++)
        {
            if (IsSoundPlaying(voiceList[i])) StopSound(voiceList[i]);
        }
    }

    // Precisa rodar antes de CloseAudioDevice(). Os aliases não são donos das
    // amostras, então saem primeiro e o som de origem por último.
    void Unload()
    {
        for (int i = 1; i < voiceCount; i++)
        {
            UnloadSoundAlias(voiceList[i]);
            voiceList[i] = {};
        }

        if (voiceCount > 0 && source.frameCount != 0) UnloadSound(source);

        source = {};
        voiceList[0] = {};
        voiceCount = 0;
        nextVoice = 0;
    }

private:
    Sound source = {};
    Sound voiceList[MAX_VOICES] = {};
    int voiceCount = 0;
    int nextVoice = 0;
};
