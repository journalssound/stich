#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Parameters.h"

StichProcessor::StichProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input",  juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts_(*this, nullptr, "Parameters", Parameters::createLayout()),
      presetManager_(*this)
{
    // Cache raw parameter pointers
    grainSizeParam_     = apvts_.getRawParameterValue(ParamID::GrainSize);
    grainDensityParam_  = apvts_.getRawParameterValue(ParamID::GrainDensity);
    grainSprayParam_    = apvts_.getRawParameterValue(ParamID::GrainSpray);
    grainPitchParam_    = apvts_.getRawParameterValue(ParamID::GrainPitch);
    grainReverseParam_  = apvts_.getRawParameterValue(ParamID::GrainReverse);
    grainFreezeParam_   = apvts_.getRawParameterValue(ParamID::GrainFreeze);
    grainFeedbackParam_ = apvts_.getRawParameterValue(ParamID::GrainFeedback);
    grainMixParam_      = apvts_.getRawParameterValue(ParamID::GrainMix);
    grainWindowParam_   = apvts_.getRawParameterValue(ParamID::GrainWindow);
    grainModeParam_     = apvts_.getRawParameterValue(ParamID::GrainMode);

    seqRateParam_       = apvts_.getRawParameterValue(ParamID::SeqRate);
    seqNumStepsParam_   = apvts_.getRawParameterValue(ParamID::SeqNumSteps);
    seqSwingParam_      = apvts_.getRawParameterValue(ParamID::SeqSwing);
    seqGateLengthParam_ = apvts_.getRawParameterValue(ParamID::SeqGateLength);
    seqGateShapeParam_  = apvts_.getRawParameterValue(ParamID::SeqGateShape);
    seqEnabledParam_    = apvts_.getRawParameterValue(ParamID::SeqEnabled);

    patDensityParam_    = apvts_.getRawParameterValue(ParamID::PatDensity);
    patVariationParam_  = apvts_.getRawParameterValue(ParamID::PatVariation);
    patLockParam_       = apvts_.getRawParameterValue(ParamID::PatLock);

    filterTypeParam_    = apvts_.getRawParameterValue(ParamID::FilterType);
    filterResoParam_    = apvts_.getRawParameterValue(ParamID::FilterReso);

    reverbEnabledParam_   = apvts_.getRawParameterValue(ParamID::ReverbEnabled);
    reverbPreDelayParam_  = apvts_.getRawParameterValue(ParamID::ReverbPreDelay);
    reverbSizeParam_      = apvts_.getRawParameterValue(ParamID::ReverbSize);
    reverbDecayParam_     = apvts_.getRawParameterValue(ParamID::ReverbDecay);
    reverbDampingParam_   = apvts_.getRawParameterValue(ParamID::ReverbDamping);
    reverbDiffusionParam_ = apvts_.getRawParameterValue(ParamID::ReverbDiffusion);
    reverbModRateParam_   = apvts_.getRawParameterValue(ParamID::ReverbModRate);
    reverbModDepthParam_  = apvts_.getRawParameterValue(ParamID::ReverbModDepth);
    reverbLowCutParam_    = apvts_.getRawParameterValue(ParamID::ReverbLowCut);
    reverbHighCutParam_   = apvts_.getRawParameterValue(ParamID::ReverbHighCut);
    reverbMixParam_       = apvts_.getRawParameterValue(ParamID::ReverbMix);

    masterOutputParam_  = apvts_.getRawParameterValue(ParamID::MasterOutput);
    masterMixParam_     = apvts_.getRawParameterValue(ParamID::MasterMix);

    presetManager_.initFactoryPresets();
}

StichProcessor::~StichProcessor() = default;

void StichProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    granularEngine_.prepare(sampleRate, samplesPerBlock);
    sequencer_.prepare(sampleRate, samplesPerBlock);
    reverb_.prepare(sampleRate, samplesPerBlock);
}

void StichProcessor::releaseResources()
{
    granularEngine_.reset();
    sequencer_.reset();
    reverb_.reset();
}

bool StichProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void StichProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    int numSamples = buffer.getNumSamples();

    if (buffer.getNumChannels() < 2) return;

    // Update granular engine parameters
    granularEngine_.setGrainSize(grainSizeParam_->load());
    granularEngine_.setDensity(grainDensityParam_->load());
    granularEngine_.setSpray(grainSprayParam_->load());
    granularEngine_.setPitch(grainPitchParam_->load());
    granularEngine_.setReverse(grainReverseParam_->load());
    granularEngine_.setFreeze(grainFreezeParam_->load() > 0.5f);
    granularEngine_.setFeedback(grainFeedbackParam_->load());
    granularEngine_.setMix(grainMixParam_->load());
    granularEngine_.setWindow(static_cast<Stich::GrainWindow>(static_cast<int>(grainWindowParam_->load())));
    granularEngine_.setMode(static_cast<Stich::GranularMode>(static_cast<int>(grainModeParam_->load())));

    // Update sequencer parameters
    sequencer_.setRate(static_cast<int>(seqRateParam_->load()));
    sequencer_.setNumSteps(static_cast<int>(seqNumStepsParam_->load()));
    sequencer_.setSwing(seqSwingParam_->load());
    sequencer_.setGateLength(seqGateLengthParam_->load());
    sequencer_.setGateShape(static_cast<Stich::GateShape>(static_cast<int>(seqGateShapeParam_->load())));
    sequencer_.setEnabled(seqEnabledParam_->load() > 0.5f);
    sequencer_.setFilterType(static_cast<Stich::FilterType>(static_cast<int>(filterTypeParam_->load())));
    sequencer_.setFilterResonance(filterResoParam_->load());

    // Update pattern generator
    patternGen_.setDensity(patDensityParam_->load());
    patternGen_.setVariation(patVariationParam_->load());
    patternGen_.setLocked(patLockParam_->load() > 0.5f);

    // Update reverb parameters
    reverb_.setEnabled(reverbEnabledParam_->load() > 0.5f);
    reverb_.setPreDelay(reverbPreDelayParam_->load());
    reverb_.setSize(reverbSizeParam_->load() * 0.01f);
    reverb_.setDecay(reverbDecayParam_->load() * 0.01f);
    reverb_.setDamping(reverbDampingParam_->load() * 0.01f);
    reverb_.setDiffusion(reverbDiffusionParam_->load() * 0.01f);
    reverb_.setModRate(reverbModRateParam_->load());
    reverb_.setModDepth(reverbModDepthParam_->load() * 0.01f);
    reverb_.setLowCut(reverbLowCutParam_->load());
    reverb_.setHighCut(reverbHighCutParam_->load());
    reverb_.setMix(reverbMixParam_->load() * 0.01f);

    // Route sequencer modulation to granular engine
    auto seqOut = sequencer_.getCurrentOutput();
    granularEngine_.setPitchModulation(seqOut.pitchOffsetSt);
    granularEngine_.setFeedbackModulation(seqOut.fxSendAmount * 0.5f);

    // Keep dry signal for master mix
    juce::AudioBuffer<float> dryBuffer;
    float masterMix = masterMixParam_->load() * 0.01f;
    if (masterMix < 1.0f)
    {
        dryBuffer.makeCopyOf(buffer);
    }

    // --- Signal Chain ---
    float* left  = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);

    // 1. Granular processing
    juce::AudioBuffer<float> granularOut(2, numSamples);
    granularEngine_.process(granularOut.getWritePointer(0),
                            granularOut.getWritePointer(1),
                            buffer.getReadPointer(0),
                            buffer.getReadPointer(1),
                            numSamples);

    // Copy granular output to main buffer
    buffer.copyFrom(0, 0, granularOut, 0, 0, numSamples);
    buffer.copyFrom(1, 0, granularOut, 1, 0, numSamples);

    // 2. Step sequencer (gate + filter)
    juce::AudioPlayHead::PositionInfo posInfo;
    if (auto* ph = getPlayHead())
    {
        auto pos = ph->getPosition();
        if (pos.hasValue())
            posInfo = *pos;
    }

    left  = buffer.getWritePointer(0);
    right = buffer.getWritePointer(1);
    sequencer_.process(left, right, numSamples, &posInfo);

    // Detect sequencer cycle completion for pattern variation
    int curStep = sequencer_.getCurrentStep();
    if (prevStep_ > curStep && prevStep_ != -1)
    {
        patternGen_.onSequencerCycleComplete(sequencer_);
    }
    prevStep_ = curStep;

    // 3. Reverb (after sequencer, before master mix)
    left  = buffer.getWritePointer(0);
    right = buffer.getWritePointer(1);
    reverb_.process(left, right, numSamples);

    // 4. Master mix (blend processed with dry)
    if (masterMix < 1.0f)
    {
        float wet = masterMix;
        float dry = 1.0f - wet;
        for (int ch = 0; ch < 2; ++ch)
        {
            float* wetData = buffer.getWritePointer(ch);
            const float* dryData = dryBuffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i)
                wetData[i] = wetData[i] * wet + dryData[i] * dry;
        }
    }

    // 5. Master output gain
    float outputGain = juce::Decibels::decibelsToGain(masterOutputParam_->load(), -60.0f);
    buffer.applyGain(outputGain);
}

void StichProcessor::regeneratePattern()
{
    patternGen_.generatePattern(sequencer_);
}

// --- State Save/Restore ---

void StichProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts_.copyState();

    // Add step sequencer data
    auto seqState = sequencer_.serializeSteps();
    state.addChild(seqState, -1, nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void StichProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr)
    {
        auto tree = juce::ValueTree::fromXml(*xml);
        if (tree.isValid())
        {
            apvts_.replaceState(tree);

            auto seqState = tree.getChildWithName("StepSequencerState");
            if (seqState.isValid())
                sequencer_.deserializeSteps(seqState);
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new StichProcessor();
}
