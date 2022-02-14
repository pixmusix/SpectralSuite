#pragma once

#include "../../shared/PluginParameters.h"
#include "../JuceLibraryCode/JuceHeader.h"

class PhaseLockParameters : public PluginParameters {
public:
    using PButtonAttachment = AudioProcessorValueTreeState::ButtonAttachment*;
    using PSliderAttachment = AudioProcessorValueTreeState::SliderAttachment*;

	PhaseLockParameters(AudioProcessor* processor);
    
	bool isPhaseLocked() const { return (bool)getParameterAsValue(KEY_PHASE_LOCK).getValue(); }
    float* getPhaseMixParameter() const { return getRawParameterValue(KEY_PHASE_MIX); }
    
    bool isFreqLocked() const { return (bool)getParameterAsValue(KEY_MAG_LOCK).getValue(); }
    float* getMagMixParameter() const { return getRawParameterValue(KEY_MAG_MIX); }
    
    float* getMagTrackParameter() const { return getRawParameterValue(KEY_MAG_TRACK); }
    float* getRandomPhaseParameter() const { return getRawParameterValue(KEY_RAND_PHASE); }
    
    bool shouldMorphMagAndPhase() const { return (bool)getParameterAsValue(KEY_MORPH_MAG_AND_PHASE).getValue(); }
    int getMorphDurationParameter() const { return (int)getParameterAsValue(KEY_MORPH_DURATION).getValue(); }
    
    PButtonAttachment createPhaseLockAttachment(ToggleButton& button) { return createButtonAttachment(KEY_PHASE_LOCK, button); }
    PSliderAttachment createPhaseMixAttachment(Slider& slider) { return createSliderAttachment(KEY_PHASE_MIX, slider); }
    
    PButtonAttachment createMagLockAttachment(ToggleButton& button) { return createButtonAttachment(KEY_MAG_LOCK, button); }
    PSliderAttachment createMagMixAttachment(Slider& slider) { return createSliderAttachment(KEY_MAG_MIX, slider); }
    
    PSliderAttachment createMagTrackAttachment(Slider& slider) { return createSliderAttachment(KEY_MAG_TRACK, slider); }
    PSliderAttachment createRandPhaseAttachment(Slider& slider) { return createSliderAttachment(KEY_RAND_PHASE, slider); }
        
    PButtonAttachment createMorphMagAndPhaseAttachment(ToggleButton& button){ return createButtonAttachment(KEY_MORPH_MAG_AND_PHASE, button); }
    PSliderAttachment createMorphDurationAttachment(Slider& slider) { return createSliderAttachment(KEY_MORPH_DURATION, slider); }
        
    void setControlPoints(Array<float> controlPoints, Array<juce::Point<int>> sourcePoints);
    Array<juce::Point<int>> getControlPoints();
    
private:
    using ParameterKey = const std::string;

    static ParameterKey KEY_PHASE_LOCK;
    static ParameterKey KEY_MAG_LOCK;
    
    static ParameterKey KEY_PHASE_MIX;
    static ParameterKey KEY_MAG_MIX;
    
    static ParameterKey KEY_MAG_TRACK;
    static ParameterKey KEY_RAND_PHASE;
    
    static ParameterKey KEY_MORPH_MAG_AND_PHASE;
    static ParameterKey KEY_MORPH_DURATION;
    
};
