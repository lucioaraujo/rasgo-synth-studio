#include <cassert>
#include <cmath>
#include <cstdio>
#include "../sequencer/CoherenceCollapse.hpp"

using namespace rasgo;

static bool approx(double a, double b, double tol = 0.01) {
    return std::fabs(a - b) < tol;
}

int main() {
    int pass = 0, fail = 0;
    auto check = [&](bool cond, const char *name) {
        if (cond) { ++pass; } else { ++fail; std::printf("FAIL: %s\n", name); }
    };

    // Entropia real: a mesma nota repetida (uma unica classe de altura)
    // deve dar exatamente 0 bits -- coerencia maxima, o caso limite que
    // a formula de Shannon precisa acertar exatamente.
    {
        CoherenceCollapse c;
        c.configure(1.0, 4, 8, 1);
        for (int i = 0; i < CoherenceCollapse::kWindowSize; ++i) c.pushNote(60); // sempre C
        check(approx(c.currentEntropy(), 0.0, 0.001), "entropia: nota unica repetida = exatamente 0 bits");
    }

    // Doze classes de altura em proporcoes iguais devem dar exatamente
    // log2(12) bits -- o maximo teorico, o outro caso limite exato.
    {
        CoherenceCollapse c;
        c.configure(1.0, 4, 8, 2);
        // kWindowSize=24, 12 classes -> 2 de cada, distribuicao
        // perfeitamente uniforme.
        for (int i = 0; i < CoherenceCollapse::kWindowSize; ++i) c.pushNote(60 + (i % 12));
        check(approx(c.currentEntropy(), std::log2(12.0), 0.01), "entropia: 12 classes uniformes = exatamente log2(12) bits");
    }

    // Entropia real e intermediaria pra uma mistura real (nao 0 nem
    // maxima) -- garante que o calculo nao esta sempre saturando nos
    // extremos.
    {
        CoherenceCollapse c;
        c.configure(1.0, 4, 8, 3);
        // metade das notas sao C, metade sao G -- 2 classes, 50/50 ->
        // entropia real = exatamente 1.0 bit (log2(2)).
        for (int i = 0; i < CoherenceCollapse::kWindowSize; ++i) c.pushNote(i % 2 == 0 ? 60 : 67);
        check(approx(c.currentEntropy(), 1.0, 0.01), "entropia: mistura 50/50 de 2 classes = exatamente 1 bit");
    }

    // Colapso exige BARRAS CONSECUTIVAS reais de baixa entropia -- um
    // unico compasso convergente nao dispara nada.
    {
        CoherenceCollapse c;
        c.configure(0.5, 4, 8, 4);
        for (int i = 0; i < CoherenceCollapse::kWindowSize; ++i) c.pushNote(60);
        bool fired = c.checkBarBoundary();
        check(!fired, "colapso: um unico compasso de baixa entropia nao dispara sozinho");
        check(c.consecutiveLowBars() == 1, "colapso: contador de barras consecutivas avanca de verdade");
    }

    // Depois de exatamente sustainedBarsRequired barras consecutivas de
    // baixa entropia, o colapso dispara exatamente uma vez -- nem antes,
    // nem duas vezes seguidas.
    {
        CoherenceCollapse c;
        c.configure(0.5, 4, 8, 5);
        for (int i = 0; i < CoherenceCollapse::kWindowSize; ++i) c.pushNote(60);
        int fires = 0;
        for (int bar = 0; bar < 4; ++bar) {
            if (c.checkBarBoundary()) ++fires;
        }
        check(fires == 1, "colapso: dispara exatamente uma vez apos as barras consecutivas exigidas");
        check(c.collapseCount() == 1, "colapso: collapseCount reflete exatamente um evento real");
    }

    // Uma barra de alta entropia no meio do caminho reseta a contagem --
    // convergencia precisa ser SUSTENTADA, uma unica interrupcao real
    // desfaz o progresso.
    {
        CoherenceCollapse c;
        c.configure(0.5, 4, 8, 6);
        for (int i = 0; i < CoherenceCollapse::kWindowSize; ++i) c.pushNote(60);
        c.checkBarBoundary(); // 1 barra baixa
        c.checkBarBoundary(); // 2 barras baixas
        // Agora uma janela de alta entropia real quebra a sequencia.
        for (int i = 0; i < CoherenceCollapse::kWindowSize; ++i) c.pushNote(60 + (i % 12));
        bool firedAfterBreak = c.checkBarBoundary();
        check(!firedAfterBreak, "colapso: uma interrupcao real de alta entropia reseta o progresso");
        check(c.consecutiveLowBars() == 0, "colapso: contador volta a zero apos a interrupcao");
    }

    // Cooldown real: depois de disparar, nao dispara de novo
    // imediatamente mesmo que a coerencia continue baixa -- um evento
    // real precisa de tempo real antes do proximo.
    {
        CoherenceCollapse c;
        c.configure(0.5, 2, 5, 7);
        for (int i = 0; i < CoherenceCollapse::kWindowSize; ++i) c.pushNote(60);
        int firesInFirstStretch = 0;
        for (int bar = 0; bar < 2; ++bar) if (c.checkBarBoundary()) ++firesInFirstStretch;
        check(firesInFirstStretch == 1, "cooldown: primeiro colapso dispara no momento certo");
        int firesDuringCooldown = 0;
        for (int bar = 0; bar < 5; ++bar) if (c.checkBarBoundary()) ++firesDuringCooldown;
        check(firesDuringCooldown == 0, "cooldown: nenhum novo colapso dispara durante a janela de cooldown real");
        // Depois do cooldown, com coerencia ainda baixa, dispara de novo.
        int firesAfterCooldown = 0;
        for (int bar = 0; bar < 2; ++bar) if (c.checkBarBoundary()) ++firesAfterCooldown;
        check(firesAfterCooldown == 1, "cooldown: um novo colapso real dispara depois que o cooldown termina");
    }

    // Determinismo: o mecanismo inteiro nao depende de nada alem da
    // sequencia real de notas empurrada -- duas instancias configuradas
    // igual e alimentadas com a mesma sequencia produzem exatamente o
    // mesmo historico de disparos.
    {
        CoherenceCollapse a, b;
        a.configure(0.6, 3, 6, 9);
        b.configure(0.6, 3, 6, 9);
        std::vector<int> notes;
        std::mt19937 seqRng(123);
        std::uniform_int_distribution<int> pcDist(0, 3); // so 4 classes -> converge as vezes, diverge as vezes
        for (int i = 0; i < 300; ++i) notes.push_back(60 + pcDist(seqRng));
        for (int n : notes) { a.pushNote(n); b.pushNote(n); }
        for (int bar = 0; bar < 40; ++bar) {
            check(a.checkBarBoundary() == b.checkBarBoundary(), "determinismo: duas instancias identicas disparam identicamente");
        }
    }

    std::printf("\n%d passed, %d failed\n", pass, fail);
    return fail == 0 ? 0 : 1;
}
