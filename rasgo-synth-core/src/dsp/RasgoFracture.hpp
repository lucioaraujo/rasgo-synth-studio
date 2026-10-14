#pragma once
#include <array>
#include <cmath>
#include <random>

namespace rasgo {

// Fase 116 -- rasgo espectral real (task #7, territorio conceitual de
// ruptura e rasgo, pedido explicito do usuario: "um dos eixos
// fundamentais do Rasgo Synth", algo aprofundado e original, sem
// padronizacoes futeis). Nao um glitch generico -- um modelo REAL de
// mecanica de materiais: o Fiber Bundle Model (Pierce 1926, "Tensile
// Tests for Cotton Yarns and Fibres"; Daniels 1945, "The Statistical
// Theory of the Strength of Bundled Filaments", Proc. Royal Society A --
// a teoria estatistica fundadora da resistencia de feixes de fibra),
// aplicado ao feixe de 6 parciais do objeto espectral (fase 113) em vez
// de tratar ruptura como um evento pontual programado.
//
// Cada parcial e uma fibra real com seu proprio limiar de resistencia,
// amostrado de uma distribuicao de Weibull -- o modelo estatistico
// padrao pra resistencia de fibra desde Weibull (1939, "A Statistical
// Theory of the Strength of Materials"; 1951, "A Statistical
// Distribution Function of Wide Applicability"), a mesma distribuicao
// usada na pratica real de engenharia de materiais fibrosos ate hoje: um
// material real tem fibras mais fracas e mais fortes numa distribuicao
// de cauda caracteristica, nao um limiar uniforme identico pra todas.
//
// Uma tensao ambiente real cresce com a propria proximidade do climax do
// arco narrativo -- pressao real acumulada, nao um sorteio pontual
// (secao 37 do manifesto do usuario: "pressao -> resistencia -> limiar
// -> ruptura -> reorganizacao"). Quando a carga acumulada de uma fibra
// ultrapassa seu limiar, ela rasga -- e, diferente do modelo classico de
// "global load sharing" (a carga liberada redistribuida uniformemente
// por TODAS as fibras sobreviventes, o modelo mais comum na literatura
// de FBM pra resistencia agregada de um feixe), este usa "local load
// sharing" (Harlow & Phoenix, 1978, "The Chain-of-Bundles Probability
// Model For The Strength Of Fibrous Materials I") -- a carga liberada
// vai SO pras fibras vizinhas imediatas ainda intactas. Esse e o modelo
// fisicamente correto pra propagacao DIRECIONAL de uma ruptura atraves
// de uma estrutura ordenada -- exatamente como um rasgo real se
// propaga pelas fibras adjacentes de um tecido, nao salta pra um fio
// aleatorio do lado oposto do feixe. Isso pode desencadear avalanches
// reais (uma fibra vizinha que ja estava perto do proprio limiar rasga
// tambem, e assim por diante, recursivamente) -- um fenomeno real e
// documentado na literatura de FBM (estatistica de avalanches em feixes
// de fibra sob local load sharing, ver tambem Newman & Phoenix 2001).
//
// A fibra 0 (o fundamental) tem resistencia efetivamente infinita -- o
// "fio sobrevivente" do proprio vocabulario do usuario: "depois de
// qualquer ruptura, alguma coisa precisa sobreviver". Uma vez rasgada,
// uma fibra NUNCA volta ao estado coerente -- a "cicatriz" real e
// permanente do vocabulario do usuario (secao 10/35 do manifesto), nao
// um efeito temporario que se desfaz.
class RasgoFracture {
public:
    static constexpr int kNumFibers = 6;

    void configure(unsigned seed) {
        std::mt19937 rng(seed);
        std::uniform_real_distribution<double> unit(1e-9, 1.0 - 1e-9);
        // Amostragem por inversao da CDF de Weibull:
        // threshold = lambda * (-ln(1-u))^(1/k). k (forma) 2.0-3.5 e
        // lambda=1.0 (escala) sao parametros reais da faixa que a
        // literatura de FBM usa pra resistencia de fibra -- k>1
        // concentra a maioria das fibras perto de uma resistencia
        // tipica, com uma cauda real de fibras mais fracas/mais fortes.
        shapeK_ = 2.0 + unit(rng) * 1.5;
        for (int i = 0; i < kNumFibers; ++i) {
            if (i == 0) {
                threshold_[static_cast<size_t>(i)] = 1e9; // fio sobrevivente -- nunca rasga
                continue;
            }
            double u = unit(rng);
            threshold_[static_cast<size_t>(i)] = std::pow(-std::log(1.0 - u), 1.0 / shapeK_);
        }
        load_.fill(0.0);
        broken_.fill(false);
        totalBreaks_ = 0;
    }

    // Tensao ambiente real -- chamada em fronteiras reais de secao,
    // cresce com a proximidade do climax. Retorna quantas fibras novas
    // rasgaram nesta aplicacao (0 se nenhuma, >1 se uma avalanche real
    // aconteceu).
    int applyAmbientStress(double amount) {
        int before = totalBreaks_;
        for (int i = 0; i < kNumFibers; ++i) {
            if (!broken_[static_cast<size_t>(i)]) breakFiberIfNeeded(i, amount);
        }
        return totalBreaks_ - before;
    }

    bool isBroken(int i) const { return broken_[static_cast<size_t>(i)]; }
    int totalBreaks() const { return totalBreaks_; }
    double shapeK() const { return shapeK_; }

private:
    void breakFiberIfNeeded(int i, double stress) {
        if (broken_[static_cast<size_t>(i)]) return;
        load_[static_cast<size_t>(i)] += stress;
        if (load_[static_cast<size_t>(i)] < threshold_[static_cast<size_t>(i)]) return;
        broken_[static_cast<size_t>(i)] = true;
        ++totalBreaks_;
        // Local load sharing: a carga que essa fibra suportava (seu
        // proprio limiar) vai so pras vizinhas imediatas ainda intactas,
        // dividida entre elas -- pode desencadear avalanche real,
        // recursivamente.
        double released = threshold_[static_cast<size_t>(i)];
        int neighbors[2] = { i - 1, i + 1 };
        int liveNeighbors = 0;
        for (int n : neighbors) {
            if (n >= 0 && n < kNumFibers && !broken_[static_cast<size_t>(n)]) ++liveNeighbors;
        }
        if (liveNeighbors == 0) return;
        double share = released / liveNeighbors;
        for (int n : neighbors) {
            if (n >= 0 && n < kNumFibers && !broken_[static_cast<size_t>(n)]) {
                breakFiberIfNeeded(n, share);
            }
        }
    }

    double shapeK_ = 2.5;
    std::array<double, kNumFibers> threshold_{};
    std::array<double, kNumFibers> load_{};
    std::array<bool, kNumFibers> broken_{};
    int totalBreaks_ = 0;
};

} // namespace rasgo
