#pragma once
#include <algorithm>
#include <random>

namespace rasgo {

// Fase 115 -- jogo como campo de forcas: blefe e confianca (task #6,
// primeiro item do territorio conceitual de jogo). Pedido explicito do
// usuario de tratar jogo a serio como materia composicional -- nao
// "gamificacao", pontuacao ou sorteio de notas, mas regras, informacao,
// confianca e transgressao como OPERACOES MUSICAIS de verdade.
//
// Esta engine renderiza uma peca sozinha, offline, sem musicos humanos ao
// vivo -- entao "jogadores" aqui sao as proprias vozes/mecanismos ja
// existentes deste motor. O mais implementavel e honesto dos varios eixos
// que o usuario descreveu (blefe, confianca, traicao, alianca, trapaca,
// jurisprudencia, arbitro nao confiavel, memoria imperfeita, jogador
// fantasma, jogo ilegivel) e o proprio nucleo que o usuario definiu:
//
//   "blefe exige tres coisas: informacao incompleta + expectativa +
//   risco" -- diferente de aleatoriedade pura porque "um numero aleatorio
//   nao possui intencao. o blefe possui intencao percebida."
//
// Modelo: um SINAL soa identico toda vez que dispara, genuino ou blefe --
// informacao genuinamente incompleta, o ouvinte (ou qualquer mecanismo
// reagindo a ele) nao pode distinguir os dois pelo som isolado. Cada
// disparo e internamente decidido genuino ou blefe por uma taxa de
// veracidade real, fixada por peca (o mesmo espectro do exemplo do
// usuario -- "jogador A: sinais verdadeiros 90%, B: 65%, C: pode blefar
// livremente"). Um sinal genuino e seguido, depois de uma janela real de
// expectativa, por um segundo gesto real de confirmacao; um blefe nao e
// seguido por nada. A CONFIANCA e uma media movel exponencial real
// atualizada a cada resolucao -- comeca neutra (sem historia), sobe com
// sinais genuinos, cai com blefes. Uma TRAICAO real so pode acontecer
// depois que a confianca ja tiver sido genuinamente construida (a
// dramaturgia do proprio usuario: "aproximacao -> cooperacao ->
// dependencia -> traicao -> reorganizacao" -- a traicao pressupoe
// confianca previa, nao existe sem ela).
class BluffSignal {
public:
    void configure(double truthfulness, unsigned seed) {
        truthfulness_ = std::clamp(truthfulness, 0.0, 1.0);
        rng_.seed(seed);
    }

    // Chamado em cada oportunidade real (ex.: todo compasso), com a
    // chance desta oportunidade especifica realmente disparar um sinal
    // novo. Se disparar, decide internamente (genuino/blefe) e retorna
    // true -- o chamador deve tocar o gesto de sinal (sempre o MESMO som,
    // goste ou nao do resultado) e agendar a resolucao pra depois.
    bool maybeSignal(double chance) {
        std::uniform_real_distribution<double> unit(0.0, 1.0);
        if (unit(rng_) >= chance) return false;
        pendingIsGenuine_ = unit(rng_) < truthfulness_;
        ++signalCount_;
        if (pendingIsGenuine_) ++genuineCount_; else ++bluffCount_;
        return true;
    }

    // Se o sinal pendente e genuino -- o chamador usa isso pra decidir se
    // toca o gesto de confirmacao quando a janela fechar. NAO deve ser
    // usado pra nada alem disso (nenhum mecanismo "sabe" disso com
    // antecedencia -- so o proprio BluffSignal, que e quem decidiu).
    bool pendingIsGenuine() const { return pendingIsGenuine_; }

    // Chamado quando a janela de expectativa do sinal pendente se fecha
    // (genuino ou blefe, sempre chamado, e essa e a unica forma de
    // confianca mudar) -- atualiza a confianca via EMA real e verifica
    // uma real quebra de confianca, so possivel depois que a confianca ja
    // tiver passado de um limiar real de "confianca genuina construida".
    void resolveSignal(double emaAlpha, double betrayalThreshold) {
        double outcome = pendingIsGenuine_ ? 1.0 : 0.0;
        trust_ += emaAlpha * (outcome - trust_);
        trust_ = std::clamp(trust_, 0.0, 1.0);
        ++resolvedCount_;
        if (trust_ > 0.65) everTrusted_ = true;
        if (!betrayed_ && everTrusted_ && trust_ < betrayalThreshold) {
            betrayed_ = true;
        }
    }

    double trust() const { return trust_; }
    bool betrayed() const { return betrayed_; }
    bool everTrusted() const { return everTrusted_; }
    int signalCount() const { return signalCount_; }
    int genuineCount() const { return genuineCount_; }
    int bluffCount() const { return bluffCount_; }
    int resolvedCount() const { return resolvedCount_; }
    double truthfulness() const { return truthfulness_; }

    // Ganho real de resposta pra qualquer voz "respondente" que reaja ao
    // sinal -- zero depois de uma traicao real (a voz se recusa a
    // responder, uma consequencia audivel real, nao decorativa), caso
    // contrario escala diretamente com a confianca atual.
    double responseGain() const { return betrayed_ ? 0.0 : trust_; }

private:
    double truthfulness_ = 0.7;
    double trust_ = 0.5;
    bool everTrusted_ = false;
    bool betrayed_ = false;
    bool pendingIsGenuine_ = false;
    int signalCount_ = 0;
    int genuineCount_ = 0;
    int bluffCount_ = 0;
    int resolvedCount_ = 0;
    std::mt19937 rng_;
};

} // namespace rasgo
