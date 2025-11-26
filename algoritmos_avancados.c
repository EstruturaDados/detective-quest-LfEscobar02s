/*
 Detective Quest - Sistema final de pistas e acusação
 - Árvore binária de salas (mapa fixo)
 - BST de pistas coletadas (ordenada)
 - Tabela hash associando pista -> suspeito
 - Exploração interativa: e (esquerda), d (direita), s (sair)
 - Ao final: listar pistas coletadas e pedir acusação
 - Verifica se ao menos 2 pistas apontam para o acusado
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_NOME 64
#define MAX_PISTA 128
#define HASH_SIZE 101  /* primo razoável para tabela pequena */

/* ---------------------------
   Estruturas
   --------------------------- */

/* Nó da árvore binária das salas */
typedef struct sala {
    char nome[MAX_NOME];
    char pista[MAX_PISTA]; /* pista associada à sala (pode ser vazia) */
    struct sala *esquerda;
    struct sala *direita;
} Sala;

/* Nó da BST que guarda as pistas coletadas */
typedef struct pistaNode {
    char pista[MAX_PISTA];
    struct pistaNode *esq;
    struct pistaNode *dir;
} PistaNode;

/* Entrada para tabela hash (encadeamento separado) */
typedef struct hashEntry {
    char pista[MAX_PISTA];     /* chave */
    char suspeito[MAX_NOME];   /* valor */
    struct hashEntry *prox;
} HashEntry;

/* ---------------------------
   Protótipos (documentados)
   --------------------------- */

/* criarSala() – cria dinamicamente um cômodo. */
Sala* criarSala(const char *nome, const char *pista);

/* explorarSalas() – navega pela árvore e ativa o sistema de pistas. */
void explorarSalas(Sala *raiz, PistaNode **raizPistas);

/* inserirPista() / adicionarPista() – insere a pista coletada na árvore de pistas. */
PistaNode* inserirPista(PistaNode *raiz, const char *pista);

/* inserirNaHash() – insere associação pista/suspeito na tabela hash. */
void inserirNaHash(HashEntry *tabela[], const char *pista, const char *suspeito);

/* encontrarSuspeito() – consulta o suspeito correspondente a uma pista. */
const char* encontrarSuspeito(HashEntry *tabela[], const char *pista);

/* verificarSuspeitoFinal() – conduz à fase de julgamento final. */
void verificarSuspeitoFinal(PistaNode *raizPistas, HashEntry *tabela[]);

/* Funções utilitárias */
void exibirPistas(PistaNode *raiz);
void liberarPistas(PistaNode *raiz);
void liberarTabelaHash(HashEntry *tabela[]);
unsigned long hash_string(const char *s);
void strip_newline(char *s);
void limparEntradaRestante(void);

/* ---------------------------
   Implementação
   --------------------------- */

/* criarSala() – cria dinamicamente um cômodo. */
Sala* criarSala(const char *nome, const char *pista) {
    Sala *s = (Sala*) malloc(sizeof(Sala));
    if (!s) {
        fprintf(stderr, "Erro de alocacao de memoria para sala.\n");
        exit(EXIT_FAILURE);
    }
    strncpy(s->nome, nome, MAX_NOME-1);
    s->nome[MAX_NOME-1] = '\0';
    if (pista != NULL && pista[0] != '\0') {
        strncpy(s->pista, pista, MAX_PISTA-1);
        s->pista[MAX_PISTA-1] = '\0';
    } else {
        s->pista[0] = '\0';
    }
    s->esquerda = s->direita = NULL;
    return s;
}

/* inserirPista() / adicionarPista() – insere a pista coletada na árvore de pistas.
   Não insere duplicatas idênticas (compara strings).
*/
PistaNode* inserirPista(PistaNode *raiz, const char *pista) {
    if (pista == NULL || pista[0] == '\0') return raiz;
    if (raiz == NULL) {
        PistaNode *n = (PistaNode*) malloc(sizeof(PistaNode));
        if (!n) { fprintf(stderr, "Erro de alocacao BST.\n"); exit(EXIT_FAILURE); }
        strncpy(n->pista, pista, MAX_PISTA-1);
        n->pista[MAX_PISTA-1] = '\0';
        n->esq = n->dir = NULL;
        return n;
    }
    int cmp = strcmp(pista, raiz->pista);
    if (cmp < 0) raiz->esq = inserirPista(raiz->esq, pista);
    else if (cmp > 0) raiz->dir = inserirPista(raiz->dir, pista);
    /* se igual, não insere duplicata */
    return raiz;
}

/* Percorre e imprime pistas em ordem alfabética */
void exibirPistas(PistaNode *raiz) {
    if (!raiz) return;
    exibirPistas(raiz->esq);
    printf(" - %s\n", raiz->pista);
    exibirPistas(raiz->dir);
}

/* liberar memória da BST de pistas */
void liberarPistas(PistaNode *raiz) {
    if (!raiz) return;
    liberarPistas(raiz->esq);
    liberarPistas(raiz->dir);
    free(raiz);
}

/* Hash simples: soma ponderada e módulo */
unsigned long hash_string(const char *s) {
    unsigned long h = 5381;
    int c;
    while ((c = (unsigned char)*s++))
        h = ((h << 5) + h) + c; /* h * 33 + c */
    return h;
}

/* inserirNaHash() – insere associação pista/suspeito na tabela hash. */
void inserirNaHash(HashEntry *tabela[], const char *pista, const char *suspeito) {
    if (!pista || !suspeito) return;
    unsigned long h = hash_string(pista) % HASH_SIZE;
    /* verificar duplicata de chave: se existir, sobrescreve o suspeito */
    HashEntry *at = tabela[h];
    while (at) {
        if (strcmp(at->pista, pista) == 0) {
            strncpy(at->suspeito, suspeito, MAX_NOME-1);
            at->suspeito[MAX_NOME-1] = '\0';
            return;
        }
        at = at->prox;
    }
    /* inserir no início da lista */
    HashEntry *novo = (HashEntry*) malloc(sizeof(HashEntry));
    if (!novo) { fprintf(stderr, "Erro de alocacao hash.\n"); exit(EXIT_FAILURE); }
    strncpy(novo->pista, pista, MAX_PISTA-1);
    novo->pista[MAX_PISTA-1] = '\0';
    strncpy(novo->suspeito, suspeito, MAX_NOME-1);
    novo->suspeito[MAX_NOME-1] = '\0';
    novo->prox = tabela[h];
    tabela[h] = novo;
}

/* encontrarSuspeito() – consulta o suspeito correspondente a uma pista. */
const char* encontrarSuspeito(HashEntry *tabela[], const char *pista) {
    if (!pista) return NULL;
    unsigned long h = hash_string(pista) % HASH_SIZE;
    HashEntry *at = tabela[h];
    while (at) {
        if (strcmp(at->pista, pista) == 0) return at->suspeito;
        at = at->prox;
    }
    return NULL;
}

/* liberar tabela hash */
void liberarTabelaHash(HashEntry *tabela[]) {
    for (int i = 0; i < HASH_SIZE; ++i) {
        HashEntry *p = tabela[i];
        while (p) {
            HashEntry *tmp = p;
            p = p->prox;
            free(tmp);
        }
        tabela[i] = NULL;
    }
}

/* remover \n de fgets */
void strip_newline(char *s) {
    if (!s) return;
    size_t L = strlen(s);
    if (L == 0) return;
    if (s[L-1] == '\n') s[L-1] = '\0';
}

/* limpar buffer stdin restante */
void limparEntradaRestante(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}

/* explorarSalas() – navega pela árvore e ativa o sistema de pistas.
   Ao entrar em uma sala exibe a pista (quando existir) e adiciona à BST de pistas.
*/
void explorarSalas(Sala *raiz, PistaNode **raizPistas) {
    Sala *atual = raiz;
    char opc;
    while (atual) {
        printf("\nVocê entrou na sala: %s\n", atual->nome);
        if (atual->pista[0] != '\0') {
            printf("  Pista encontrada: \"%s\"\n", atual->pista);
            *raizPistas = inserirPista(*raizPistas, atual->pista);
        } else {
            printf("  (Nenhuma pista nesta sala)\n");
        }

        /* Menu */
        printf("\nEscolha: (e) esquerda  (d) direita  (s) sair\n");
        printf("Opcao: ");
        if (scanf(" %c", &opc) != 1) {
            printf("Entrada inválida. Encerrando.\n");
            break;
        }
        limparEntradaRestante();

        if (opc == 'e' || opc == 'E') {
            if (atual->esquerda) atual = atual->esquerda;
            else printf("Não há caminho à esquerda.\n");
        } else if (opc == 'd' || opc == 'D') {
            if (atual->direita) atual = atual->direita;
            else printf("Não há caminho à direita.\n");
        } else if (opc == 's' || opc == 'S') {
            printf("Exploração encerrada pelo jogador.\n");
            break;
        } else {
            printf("Opção inválida. Use e, d ou s.\n");
        }
    }
}

/* Função auxiliar que percorre BST e conta quantas pistas apontam para 'suspeitoAlvo'.
   Utiliza a tabela hash para mapear cada pista -> suspeito.
*/
void contarPistasPorSuspeitoRec(PistaNode *raiz, HashEntry *tabela[], const char *suspeitoAlvo, int *contador) {
    if (!raiz) return;
    contarPistasPorSuspeitoRec(raiz->esq, tabela, suspeitoAlvo, contador);
    const char *s = encontrarSuspeito(tabela, raiz->pista);
    if (s && strcmp(s, suspeitoAlvo) == 0) (*contador)++;
    contarPistasPorSuspeitoRec(raiz->dir, tabela, suspeitoAlvo, contador);
}

/* verificarSuspeitoFinal() – conduz à fase de julgamento final.
   Lista pistas coletadas, pede o nome do suspeito e verifica se há >=2 pistas que o apontam.
*/
void verificarSuspeitoFinal(PistaNode *raizPistas, HashEntry *tabela[]) {
    printf("\n===== Pistas coletadas (ordem alfabética) =====\n");
    if (!raizPistas) {
        printf("Nenhuma pista coletada.\n");
    } else {
        exibirPistas(raizPistas);
    }

    char acusado[MAX_NOME];
    printf("\nQuem você acusa como culpado? (escreva o nome exato): ");
    /* usar fgets para permitir nomes com espaços */
    if (!fgets(acusado, sizeof(acusado), stdin)) {
        printf("Erro na leitura. Encerrando verificação.\n");
        return;
    }
    strip_newline(acusado);
    if (strlen(acusado) == 0) {
        printf("Nenhum nome fornecido. Acusação inválida.\n");
        return;
    }

    int cont = 0;
    contarPistasPorSuspeitoRec(raizPistas, tabela, acusado, &cont);

    printf("\nAcusado: %s\n", acusado);
    printf("Pistas que apontam para %s: %d\n", acusado, cont);

    if (cont >= 2) {
        printf("\nVEREDICTO: Há pistas suficientes! %s é considerado culpado.\n", acusado);
    } else {
        printf("\nVEREDICTO: Pistas insuficientes. %s não pode ser acusado com segurança.\n", acusado);
    }
}

/* ---------------------------
   MAIN: monta mapa, tabela hash e executa jogo
   --------------------------- */
int main(void) {
    /* Montagem do mapa (árvore binária de salas) - fixo */
    Sala *hall = criarSala("Hall de Entrada", "Pegada suja");
    Sala *estar = criarSala("Sala de Estar", "Perfume feminino caro");
    Sala *biblioteca = criarSala("Biblioteca", "Livro rasgado");
    Sala *cozinha = criarSala("Cozinha", "Copo com fragmento de esmalte");
    Sala *jardim = criarSala("Jardim", "Filtro de cigarro");
    Sala *porao = criarSala("Porão", "Luva encharcada");

    /* montar ligações */
    hall->esquerda = estar;
    hall->direita = biblioteca;

    estar->esquerda = cozinha;
    estar->direita = jardim;

    biblioteca->direita = porao;

    /* Preparar tabela hash (inicializa com NULL) */
    HashEntry *tabela[HASH_SIZE];
    for (int i = 0; i < HASH_SIZE; ++i) tabela[i] = NULL;

    /* Inserir associações pista -> suspeito (pré-definido) */
    inserirNaHash(tabela, "Pegada suja", "Carlos");
    inserirNaHash(tabela, "Perfume feminino caro", "Dona Beatriz");
    inserirNaHash(tabela, "Livro rasgado", "Professor Otávio");
    inserirNaHash(tabela, "Copo com fragmento de esmalte", "Dona Beatriz");
    inserirNaHash(tabela, "Filtro de cigarro", "Carlos");
    inserirNaHash(tabela, "Luva encharcada", "Professor Otávio");

    /* Árvore BST de pistas coletadas (inicialmente vazia) */
    PistaNode *raizPistas = NULL;

    printf("=== Detective Quest: Investigacao Final ===\n");
    printf("Explore a mansão e colete pistas. Quando terminar, acuse o suspeito.\n");

    explorarSalas(hall, &raizPistas);

    verificarSuspeitoFinal(raizPistas, tabela);

    /* liberar memória */
    liberarPistas(raizPistas);
    liberarTabelaHash(tabela);
    free(hall); free(estar); free(biblioteca); free(cozinha); free(jardim); free(porao);

    printf("\nObrigado por jogar Detective Quest!\n");
    return 0;
}
