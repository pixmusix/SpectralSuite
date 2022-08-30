#include "SpectralGateFFTProcessor.h"

SpectralGateFFTProcessor::SpectralGateFFTProcessor(int size, int hops, int offset, int sRate, std::shared_ptr<PhaseBuffer> phaseBuffer)
    : PhaseVocoder(size, hops, offset, sRate, phaseBuffer), m_cutOff(0.0), m_balance(0.0)
{
    recalculateInternalParameters();
}

void SpectralGateFFTProcessor::setCutOff(float cutoff) {
    if(cutoff != m_cutOff) {
        m_cutOff = cutoff;
        recalculateInternalParameters();
    }
}

void SpectralGateFFTProcessor::setBalance(float balance) {
    if(balance != m_balance) {
        m_balance = balance;
        recalculateInternalParameters();
    }
}

void SpectralGateFFTProcessor::setTilt(float tilt) {
    if(tilt != m_tilt) {
        m_tilt = tilt;
    }
}

void SpectralGateFFTProcessor::spectral_process(const PolarVector &in, PolarVector &out, int bins) {
    float mag;
    float tilt = m_tilt;
    tilt = (tilt - 0.5f) * 2.f;
//    tilt = tilt = pow(tilt, 3);

    out[0]=in[0];
    for(int n = 1; n < bins; ++n){
        float frac = (float)n/(float)bins;
        float highScaleOffset = (frac - 0.5f) * 2.f * m_gate_high * tilt;
        float gateHigh = m_gate_high + highScaleOffset;
        
        float lowScaleOffset = highScaleOffset * 0.8;
        float gateLow = m_gate_low + lowScaleOffset;
        
        
        mag = in[n].m_mag;
        if (mag >= gateHigh){
          out[n].m_mag = mag * m_balance_strong_scale;
        }
        else if(mag < gateLow){
          out[n].m_mag = mag * m_balance_weak_scale;
        }
        else {
            out[n].m_mag = 0.f;
        }
        out[n].m_phase = in[n].m_phase;
    }
};

void SpectralGateFFTProcessor::recalculateInternalParameters() {
    m_gate_high = pow(m_cutOff, 10);
    m_gate_low = m_cutOff * 0.6f;
    m_balance_strong_scale = powf(m_balance, 3.f);
    m_balance_weak_scale = pow(1.f-m_balance, 4.f);
}
