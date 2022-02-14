#pragma once

#include <vector>

#include "../../shared/specprocess.h"

class BinScramblerFFTProcessor : public STFT {
  public:
        // previous scrambled index and new scrambled index. Will interpolate between the previous and the new
        std::vector<int> *m_AInd;
        std::vector<int> *m_BInd;

        void setPhase(int phase) { m_phase = phase; }
        void setMaxPhase(int maxPhase) { m_maxPhase = maxPhase; }

        // takes int vector pointer as arguments to the scrambled indices. The scrambling is done external to the spec proc
        BinScramblerFFTProcessor(int size, int hops, int offset, int sRate, std::vector<int> *A, std::vector<int> *B) : STFT(size, hops, offset, sRate),
            m_AInd(A), m_BInd(B), m_phase(0), m_maxPhase(sRate){}
        // swap the index pointers
        void swap(){ std::swap(m_AInd, m_BInd);}
        virtual void spectral_process(const PolarVector &in, PolarVector &out, int bins) override;

private:
    int m_phase;
    int m_maxPhase;
};
