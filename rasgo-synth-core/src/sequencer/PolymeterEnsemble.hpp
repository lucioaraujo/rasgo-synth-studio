#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <vector>
#include "../dsp/Oscillator.hpp"
#include "../dsp/Envelope.hpp"

namespace rasgo {

// Fase 114 -- metrica como sistema de organizacao do tempo (task #3,
// quarto item do territorio conceitual musical): pedido explicito do
// usuario de pensar metrica alem do modalismo armenio -- nao "qual
// compasso usar?" mas "quantos ciclos estao acontecendo simultaneamente?"
// (formulacao literal do usuario, a ideia que ele chamou de "metrica como
// contraponto").
//
// Eixo A -- polimetria real: kNumVoices vozes percussivas independentes,
// cada uma com seu proprio comprimento de ciclo em PASSOS reais (nao
// compassos -- um conjunto coprimo curado, o mesmo exemplo numerico do
// usuario: "melodia 11, baixo 7, harmonia 5, percussao 8"). Cada voz
// reconta sua propria posicao dentro do proprio ciclo lendo o MESMO
// relogio mestre de passos que o resto do motor ja usa (stepCounter em
// renderIntegratedPiece) -- nao um clock independente, so uma leitura
// modular diferente do mesmo relogio, exatamente como o usuario descreveu
// ("cada elemento retorna ao comeco em um momento diferente"). As vozes
// se reencontram num macroperiodo real e computavel -- o MMC dos
// comprimentos escolhidos -- diferente do polyrhythm divider ja existente
// neste motor (que combina divisoes XOR de UM relogio compartilhado, nao
// vozes com "compassos" proprios de verdade).
//
// Eixo B -- pulso nao isocrono: dentro do proprio ciclo de cada voz, os
// pontos de ataque NAO caem em posicoes regularmente espacadas -- os
// intervalos entre eles sao gerados livremente (pontos de corte
// aleatorios normalizados pra somar exatamente o comprimento do ciclo,
// como o usuario descreveu: "300/200/350/150ms somando exatamente
// 1000ms") em vez das celulas fixas de 2 e 3 do aksak ja existente neste
// motor -- uma geometria temporal deformada real e documentada (Justin
// London, "Hearing in Time: Psychological Aspects of Musical Meter",
// 2004 -- meter nao-isocrono).
class PolymeterEnsemble {
public:
    static constexpr int kNumVoices = 4;

    void setSampleRate(double sr) {
        for (auto &e : envelopes_) e.setSampleRate(sr);
    }

    void configure(int rootMidiNote, unsigned seed) {
        std::mt19937 rng(seed);
        std::uniform_real_distribution<double> unit(0.0, 1.0);

        // Conjunto coprimo curado -- o mesmo exemplo numerico do usuario
        // (5, 7, 8, 11, 13). Pairwise coprimo por construcao (dois primos
        // distintos quaisquer, ou 8=2^3 e 9=3^2 contra qualquer um dos
        // primos ou entre si), entao o macroperiodo real e sempre o
        // PRODUTO dos comprimentos escolhidos -- MMC calculado de verdade
        // abaixo mesmo assim, pra ficar correto mesmo se este conjunto
        // mudar no futuro.
        static const int kCandidateLengths[] = {5, 7, 8, 9, 11, 13};
        std::vector<int> pool(std::begin(kCandidateLengths), std::end(kCandidateLengths));
        std::shuffle(pool.begin(), pool.end(), rng);
        for (int v = 0; v < kNumVoices; ++v) cycleLengthSteps_[static_cast<size_t>(v)] = pool[static_cast<size_t>(v)];

        // Registro ascendente por voz -- raiz, quinta, oitava, oitava+
        // quinta -- um pequeno conjunto percussivo afinado onde cada
        // periodicidade tem sua propria identidade de altura, condicao
        // que o proprio usuario apontou como necessaria pro efeito de
        // contraponto de periodicidades realmente se ler ("cada elemento
        // retorna ao comeco em um momento diferente" precisa ser
        // audivelmente rastreavel).
        static const int kIntervals[kNumVoices] = {0, 7, 12, 19};
        for (int v = 0; v < kNumVoices; ++v) {
            oscillators_[static_cast<size_t>(v)].setWaveform(Oscillator::Waveform::Sine);
            oscillators_[static_cast<size_t>(v)].setFrequency(midiToFreq(rootMidiNote + kIntervals[v]));
            envelopes_[static_cast<size_t>(v)].setADSR(0.005, 0.12 + unit(rng) * 0.1, 0.0, 0.03);
            panPos_[static_cast<size_t>(v)] = -1.0f + 2.0f * static_cast<float>(v) / static_cast<float>(kNumVoices - 1);
        }

        // Eixo B por voz: k pontos de ataque (2..5), intervalos entre eles
        // gerados livremente e normalizados pra somar exatamente o
        // comprimento do proprio ciclo em passos inteiros -- processo
        // real tipo Dirichlet (pesos aleatorios positivos, normalizados),
        // arredondado com o resto absorvido no ultimo intervalo pra
        // garantir soma exata.
        for (int v = 0; v < kNumVoices; ++v) {
            int cycleLen = cycleLengthSteps_[static_cast<size_t>(v)];
            int k = 2 + static_cast<int>(unit(rng) * 4.0); // 2..5
            k = std::min(k, cycleLen); // nunca mais ataques que passos disponiveis
            std::vector<double> weights(static_cast<size_t>(k));
            double total = 0.0;
            for (auto &w : weights) { w = 0.2 + unit(rng); total += w; }
            std::vector<int> gaps(static_cast<size_t>(k));
            int assigned = 0;
            for (int j = 0; j < k - 1; ++j) {
                int g = std::max(1, static_cast<int>(std::round(weights[static_cast<size_t>(j)] / total * cycleLen)));
                gaps[static_cast<size_t>(j)] = g;
                assigned += g;
            }
            gaps[static_cast<size_t>(k - 1)] = std::max(1, cycleLen - assigned); // resto absorvido aqui, soma exata garantida
            // Se o arredondamento estourou o total (gaps ja somam >=
            // cycleLen antes do ultimo), reduz do maior gap pra manter a
            // soma exata sem zerar nenhum intervalo.
            int sum = 0; for (int g : gaps) sum += g;
            while (sum > cycleLen) {
                auto maxIt = std::max_element(gaps.begin(), gaps.end());
                if (*maxIt <= 1) break;
                --(*maxIt);
                --sum;
            }
            std::vector<int> onsetSteps;
            int pos = 0;
            for (int g : gaps) { onsetSteps.push_back(pos); pos += g; }
            onsetSteps_[static_cast<size_t>(v)] = onsetSteps;
            gaps_[static_cast<size_t>(v)] = gaps;
        }

        macroperiodSteps_ = cycleLengthSteps_[0];
        for (int v = 1; v < kNumVoices; ++v) {
            macroperiodSteps_ = std::lcm(macroperiodSteps_, cycleLengthSteps_[static_cast<size_t>(v)]);
        }
    }

    void setGain(float g) { masterGain_ = g; }

    // Chamado uma vez por fronteira REAL de passo (o mesmo relogio mestre
    // que StepSequencer/percussao ja usam), com o indice de passo
    // monotonico desde o inicio da peca -- nao o indice que envolve
    // dentro do compasso do sequencer principal, que teria comprimento
    // errado pra esta matematica.
    void tickStep(size_t globalStepIndex) {
        bool allAtZero = true;
        for (int v = 0; v < kNumVoices; ++v) {
            int cycleLen = cycleLengthSteps_[static_cast<size_t>(v)];
            int pos = static_cast<int>(globalStepIndex % static_cast<size_t>(cycleLen));
            if (pos != 0) allAtZero = false;
            const auto &onsets = onsetSteps_[static_cast<size_t>(v)];
            if (std::find(onsets.begin(), onsets.end(), pos) != onsets.end()) {
                envelopes_[static_cast<size_t>(v)].noteOn();
                ++pulseCounts_[static_cast<size_t>(v)];
            }
        }
        if (allAtZero) ++realignmentCount_;
    }

    float tick(float *outL = nullptr, float *outR = nullptr) {
        float sumL = 0.0f, sumR = 0.0f;
        constexpr float kPi = 3.14159265358979323846f;
        for (int v = 0; v < kNumVoices; ++v) {
            float s = oscillators_[static_cast<size_t>(v)].tick() * envelopes_[static_cast<size_t>(v)].tick();
            float panAngle = (panPos_[static_cast<size_t>(v)] * 0.5f + 0.5f) * (kPi * 0.5f);
            sumL += s * std::cos(panAngle);
            sumR += s * std::sin(panAngle);
        }
        sumL *= masterGain_;
        sumR *= masterGain_;
        if (outL) *outL = sumL;
        if (outR) *outR = sumR;
        return sumL + sumR;
    }

    int macroperiodSteps() const { return macroperiodSteps_; }
    int realignmentCount() const { return realignmentCount_; }
    int pulseCount(int v) const { return pulseCounts_[static_cast<size_t>(v)]; }
    int cycleLength(int v) const { return cycleLengthSteps_[static_cast<size_t>(v)]; }
    const std::vector<int> &gaps(int v) const { return gaps_[static_cast<size_t>(v)]; }

    std::string description() const {
        std::ostringstream desc;
        desc << "ciclos ";
        for (int v = 0; v < kNumVoices; ++v) {
            if (v) desc << "/";
            desc << cycleLengthSteps_[static_cast<size_t>(v)];
        }
        desc << " passos, macroperiodo " << macroperiodSteps_ << " passos";
        return desc.str();
    }

private:
    static double midiToFreq(int midiNote) {
        return 440.0 * std::pow(2.0, (midiNote - 69) / 12.0);
    }

    std::array<int, kNumVoices> cycleLengthSteps_{};
    std::array<std::vector<int>, kNumVoices> onsetSteps_;
    std::array<std::vector<int>, kNumVoices> gaps_;
    std::array<Oscillator, kNumVoices> oscillators_;
    std::array<Envelope, kNumVoices> envelopes_;
    std::array<float, kNumVoices> panPos_{};
    std::array<int, kNumVoices> pulseCounts_{};
    int macroperiodSteps_ = 1;
    int realignmentCount_ = 0;
    float masterGain_ = 0.15f;
};

} // namespace rasgo
