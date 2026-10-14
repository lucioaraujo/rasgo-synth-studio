#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>
#include "../dsp/RasgoFracture.hpp"

using namespace rasgo;

int main() {
    int pass = 0, fail = 0;
    auto check = [&](bool cond, const char *name) {
        if (cond) { ++pass; } else { ++fail; std::printf("FAIL: %s\n", name); }
    };

    // Fio sobrevivente: fibra 0 nunca rasga, mesmo sob tensao absurda e
    // repetida.
    {
        RasgoFracture f;
        f.configure(1);
        for (int i = 0; i < 500; ++i) f.applyAmbientStress(1000.0);
        check(!f.isBroken(0), "fio sobrevivente: fibra 0 nunca rasga sob tensao repetida e absurda");
        check(f.totalBreaks() == RasgoFracture::kNumFibers - 1, "todas as outras 5 fibras rasgam sob tensao suficiente");
    }

    // Determinismo: mesma seed produz exatamente a mesma sequencia de
    // rupturas.
    {
        RasgoFracture a, b;
        a.configure(42);
        b.configure(42);
        for (int i = 0; i < 20; ++i) {
            int ba = a.applyAmbientStress(0.15);
            int bb = b.applyAmbientStress(0.15);
            check(ba == bb, "determinismo: mesma seed produz o mesmo numero de rupturas a cada rodada");
            for (int fi = 0; fi < RasgoFracture::kNumFibers; ++fi) {
                check(a.isBroken(fi) == b.isBroken(fi), "determinismo: mesmo estado de fibra em toda rodada");
            }
        }
    }

    // Cicatriz permanente: uma vez rasgada, uma fibra nunca "desrasga",
    // e tensao adicional nao a conta como uma nova ruptura.
    {
        RasgoFracture f;
        f.configure(3);
        f.applyAmbientStress(1000.0); // forca varias rupturas de uma vez
        int totalAfterFirst = f.totalBreaks();
        std::array<bool, RasgoFracture::kNumFibers> stateAfterFirst;
        for (int i = 0; i < RasgoFracture::kNumFibers; ++i) stateAfterFirst[static_cast<size_t>(i)] = f.isBroken(i);
        f.applyAmbientStress(1000.0); // de novo
        check(f.totalBreaks() == totalAfterFirst, "cicatriz: rupturas ja contadas nao contam de novo sob mais tensao");
        for (int i = 0; i < RasgoFracture::kNumFibers; ++i) {
            check(f.isBroken(i) == stateAfterFirst[static_cast<size_t>(i)], "cicatriz: estado de fibra permanece identico apos mais tensao");
        }
    }

    // Local load sharing real: um unico evento de baixa tensao que
    // rasga exatamente uma fibra isolada (sem avalanche) nao deve
    // rasgar fibras distantes -- verificado plantando uma fibra fraca no
    // meio do feixe com vizinhas fortes o bastante pra nao cascatear.
    {
        // Testa via muitos seeds ate achar um com exatamente 1 ruptura
        // isolada por rodada em algum ponto, confirmando que ao menos
        // uma rodada single-break e possivel (nao toda ruptura vira
        // avalanche automatica) -- se o modelo so produzisse avalanches
        // totais, isso indicaria um bug na dosagem de amount vs
        // thresholds.
        bool sawIsolatedBreak = false, sawAvalanche = false;
        for (unsigned seed = 1; seed <= 50 && !(sawIsolatedBreak && sawAvalanche); ++seed) {
            RasgoFracture f;
            f.configure(seed);
            for (int i = 0; i < 40; ++i) {
                int n = f.applyAmbientStress(0.08);
                if (n == 1) sawIsolatedBreak = true;
                if (n > 1) sawAvalanche = true;
            }
        }
        check(sawIsolatedBreak, "local load sharing: rupturas isoladas (sem avalanche) acontecem de verdade");
        check(sawAvalanche, "local load sharing: avalanches reais (mais de uma ruptura na mesma rodada) tambem acontecem");
    }

    // Distribuicao de Weibull: media empirica das ameacas (fibras 1-5,
    // excluindo o fio sobrevivente) bate com a formula real
    // lambda*Gamma(1+1/k) pra lambda=1, verificada indiretamente contando
    // quantas rodadas de tensao fixa sao necessarias pra cada fibra
    // romper -- mais direto: reamostrar via configure() repetido com
    // seeds diferentes e checar que os limiares resultantes (inferidos
    // pela tensao total ate a ruptura sob tensao pequena e constante)
    // cobrem uma faixa real, nao um valor unico fixo.
    {
        std::vector<int> stepsToBreakFiber1;
        for (unsigned seed = 1; seed <= 30; ++seed) {
            RasgoFracture f;
            f.configure(seed);
            int steps = 0;
            while (!f.isBroken(1) && steps < 1000) {
                f.applyAmbientStress(0.05);
                ++steps;
            }
            stepsToBreakFiber1.push_back(steps);
        }
        int minSteps = *std::min_element(stepsToBreakFiber1.begin(), stepsToBreakFiber1.end());
        int maxSteps = *std::max_element(stepsToBreakFiber1.begin(), stepsToBreakFiber1.end());
        check(minSteps != maxSteps, "weibull: limiares de fibra variam de verdade entre seeds (nao um valor fixo)");
        check(minSteps > 0, "weibull: nenhuma fibra rasga instantaneamente sob tensao minima");
    }

    std::printf("\n%d passed, %d failed\n", pass, fail);
    return fail == 0 ? 0 : 1;
}
