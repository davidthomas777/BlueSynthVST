/*
  ==============================================================================

    AdsrData.cpp
    Created: 17 Dec 2025 4:07:17pm
    Author:  David Thomas

  ==============================================================================
*/

#include "AdsrData.h"

void AdsrData::updateADSR(const float attack, const float decay, const float sustain, const float release) {
    adsrParams.attack = attack;
    adsrParams.decay = decay;
    adsrParams.sustain = sustain;
    adsrParams.release = release;
    
    if (!hasModulation() && !modRunning) setParameters(adsrParams);
}

bool AdsrData::hasModulation() const noexcept
{ return modulation != nullptr && (modulation->active(first) || modulation->active(first+1) || modulation->active(first+2) || modulation->active(first+3)); }
float AdsrData::mod (int offset, float base) const noexcept
{
    if(modulation==nullptr) return base;
    float value=modulation->apply(first+offset,base,*lfoValues);
    modulation->publishDisplay(first+offset,value);
    return value;
}
void AdsrData::noteOn()
{
    setParameters(adsrParams);
    stage=adsrParams.attack>0 ? 1 : adsrParams.decay>0 && adsrParams.sustain<1 ? 2 : 3;
    juce::ADSR::noteOn();
    const float initialAttack=mod(0,adsrParams.attack);
    decaySustain=getParameters().sustain;
    modRunning=hasModulation(); modValue=0;
    if(modRunning)
    {
        if(initialAttack>0) { stage=1; }
        else { modValue=1; enterDecay(); }
    }
}
void AdsrData::noteOff()
{
    juce::ADSR::noteOff();
    if(stage==0) return;
    stage=4;
    const float initialRelease=mod(3,adsrParams.release);
    releaseLevel=modValue;
    if(modRunning || hasModulation())
    {
        modRunning=true;
        if(initialRelease<=0) { modValue=0; stage=0; }
    }
}
void AdsrData::enterDecay()
{
    const float decay=mod(1,adsrParams.decay); decaySustain=mod(2,adsrParams.sustain);
    if(decay>0 && decaySustain<1) { stage=2; }
    else { stage=3; modValue=decaySustain; }
}
float AdsrData::getNextSample() noexcept
{
    if(!modRunning && hasModulation() && stage!=0)
    {
        modRunning=true;
        if(stage==4) releaseLevel=modValue;
    }
    if(modRunning)
    {
        const float attack=mod(0,adsrParams.attack);
        const float decay=mod(1,adsrParams.decay);
        const float sustain=mod(2,adsrParams.sustain);
        const float release=mod(3,adsrParams.release);
        if(stage==1)
        {
            modValue+=attack>0 ? (float)(1/(attack*sampleRate)) : 1;
            if(modValue>=1) { modValue=1; enterDecay(); return 1; }
        }
        else if(stage==2)
        {
            decaySustain=sustain;
            modValue-=decay>0 ? (float)((1-decaySustain)/(decay*sampleRate)) : 1;
            if(modValue<=decaySustain) { modValue=decaySustain; stage=3; }
        }
        else if(stage==3) modValue=sustain;
        else if(stage==4)
        {
            modValue-=release>0 ? (float)(releaseLevel/(release*sampleRate)) : releaseLevel;
            if(modValue<=0) { modValue=0; stage=0; }
        }
        return modValue;
    }
    modValue=juce::ADSR::getNextSample();
    if(stage==1 && modValue>=1) { stage=2; decaySustain=getParameters().sustain; }
    if(stage==2 && modValue<=getParameters().sustain) stage=3;
    if(!juce::ADSR::isActive()) stage=0;
    return modValue;
}
