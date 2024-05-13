#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <float.h>

#include ".\include\cjson\cJSON.h"  //https://github.com/DaveGamble/cJSON
#include ".\include\hash.h"  //https://github.com/edpfacom/libfacom -> Com pequenas modificações

#define PI 3.14159265358979323846

typedef struct
{
    int cod_uf, siafi_id, ddd, capital;
    char nome_municipio[100], fuso[100], cod_ibge[100];
    float latitude, longitude;

} tmunicipio;

typedef struct _tnode
{
    tmunicipio *reg;
    struct _tnode *esq;
    struct _tnode *dir;
} tnode;


//Calculo de distancia usando a formula de haversine
float distancia(tmunicipio muni1, tmunicipio muni2)
{

    float ang = muni1.longitude - muni2.longitude;
    float dist =  acos((sin(muni1.latitude * PI / 180.0) * sin(muni2.latitude* PI / 180.0)) + (cos(muni1.latitude* PI / 180.0) * cos(muni2.latitude* PI / 180.0) * cos(ang* PI / 180.0)))* 180.0 / PI * 60.0 * 1.1515 ;
   
    return round(dist * 1.609344);
}

char *get_key_municipio(void *reg)
{
    return (*((tmunicipio *)reg)).cod_ibge;
}

char *get_key_municipio2(void *reg)
{
    return (*((tmunicipio *)reg)).nome_municipio;
}

void *aloca_municipio(char *cod_ibge, int cod_uf, int siafi_id, int ddd, int capital, char *nome_municipio, char *fuso, float latitude, float longitude)
{

    tmunicipio *municipio = malloc(sizeof(tmunicipio));

    strcpy(municipio->cod_ibge, cod_ibge);
    municipio->cod_uf = cod_uf;
    municipio->siafi_id = siafi_id;
    municipio->ddd = ddd;
    municipio->capital = capital;
    strcpy(municipio->nome_municipio, nome_municipio);
    strcpy(municipio->fuso, fuso);
    municipio->latitude = latitude;
    municipio->longitude = longitude;

    return municipio;
}

tnode *kdtree_insere(tnode *raiz, tmunicipio *municipio, int nivel)
{
    if (raiz == NULL)
    {
        tnode *no = malloc(sizeof(tnode));
        no->reg = municipio;
        no->esq = NULL;
        no->dir = NULL;
        return no;
    }
    // Alternar entre latitude e longitude nos diferentes níveis

    int dimensao = nivel % 2;

    if (dimensao == 0)
    {

        if (municipio->latitude < raiz->reg->latitude)
            raiz->esq = kdtree_insere(raiz->esq, municipio, nivel + 1);
        else
            raiz->dir = kdtree_insere(raiz->dir, municipio, nivel + 1);
    }
    else
    {
        if (municipio->longitude < raiz->reg->longitude)
            raiz->esq = kdtree_insere(raiz->esq, municipio, nivel + 1);
        else
            raiz->dir = kdtree_insere(raiz->dir, municipio, nivel + 1);
    }

    return raiz;
}

//Funcao para buscar as n cidades vizinhas mais proximas cidade de referencia
void busca_vizinhos(tnode *raiz, tmunicipio *muni, int n)
{
 
    // Encontra o município de referência na árvore
    tnode *no_atual = raiz;
    while (no_atual != NULL && strcmp(no_atual->reg->cod_ibge, muni->cod_ibge)== 0)
    {
        if (muni->latitude < no_atual->reg->latitude )
        {   
            no_atual = no_atual->esq;   
        }
        else if (muni->latitude > no_atual->reg->latitude )
        { 
            no_atual = no_atual->dir;
           
        }
    }

    // Cria um vetor para armazenar os vizinhos
    tmunicipio **vizinhos = (tmunicipio **)malloc(5570 * sizeof(tmunicipio *));
   

    // adiciona todos os municipios no vetor vizinhos
    tnode *fila[6000];// Realiza a busca em largura
    int inicio = 0, fim = 0;
    fila[fim++] = raiz;
    int indice_vizinho = 0; 
    while (indice_vizinho <= 5569) {
        tnode *no = fila[inicio++];
        if (no != NULL) {
            vizinhos[indice_vizinho++] = no->reg;
            
            
                if (no->esq != NULL)
                    fila[fim++] = no->esq;
            
                if (no->dir != NULL)
                    fila[fim++] = no->dir;
            
        }
    }

    //insertion sort para ordenar os municipios baseado na distancia do municipio de referencia
    for (int i = 1; i <= 5569; i++) {
        tmunicipio *viz = vizinhos[i];
        int j = i - 1;
        while (j >= 0 && distancia(*muni, *vizinhos[j]) > distancia(*muni, *viz)) {
            vizinhos[j + 1] = vizinhos[j];
            j = j - 1;
        }
        vizinhos[j + 1] = viz;
    }

    // Exibe os municípios vizinhos encontrados
    printf("\nOs municipios mais proximos a %s sao:\n", muni->nome_municipio);

    for (int i = 1; i <= n; i++)
    {   
        char capt[14] = "Nao e capital";
        if (vizinhos[i] != NULL)
        {   
            if (vizinhos[i]->capital == 1)
                strcpy(capt, "e capital");

            printf("\n%d: %s - Codigo IBGE - %s - %s - Distancia: %.2fkm - DDD : %d - Fuso horario - %s\n\n", i, vizinhos[i]->nome_municipio, vizinhos[i]->cod_ibge, capt,distancia(*muni, *vizinhos[i]), vizinhos[i]->ddd, vizinhos[i]->fuso);
        }
    }

    // Libera a memória alocada para o vetor de vizinhos
    free(vizinhos);
}

void lerjson(thash *h,thash *h2, tnode **raiznode, int nivel)
{
    // Abre o arquivo
    FILE *json = fopen("municipios.json", "r");

    // Obter o tamanho do arquivo
    fseek(json, 0, SEEK_END);
    long tamanho = ftell(json);
    fseek(json, 0, SEEK_SET);

    // Ler o conteúdo do arquivo
    char *conteudo = (char *)malloc(tamanho + 1);
    fread(conteudo, 1, tamanho, json);
    fclose(json);

    // Adicionar terminador de string
    conteudo[tamanho] = '\0';

    // Analisar o JSON
    cJSON *raiz = cJSON_Parse(conteudo);
    free(conteudo);

    cJSON *objeto = raiz->child;
    char aux[100];

    while (objeto != NULL)
    {

        cJSON *cod_ibge = cJSON_GetObjectItem(objeto, "codigo_ibge");
        cJSON *nome = cJSON_GetObjectItem(objeto, "nome");
        cJSON *fuso = cJSON_GetObjectItem(objeto, "fuso_horario");
        cJSON *siafi_id = cJSON_GetObjectItem(objeto, "siafi_id");
        cJSON *cod_uf = cJSON_GetObjectItem(objeto, "codigo_uf");
        cJSON *ddd = cJSON_GetObjectItem(objeto, "ddd");
        cJSON *capital = cJSON_GetObjectItem(objeto, "capital");
        cJSON *lati = cJSON_GetObjectItem(objeto, "latitude");
        cJSON *longi = cJSON_GetObjectItem(objeto, "longitude");

        sprintf(aux, "%d", cod_ibge->valueint);

        assert(hash_insere(h, aloca_municipio(aux, cod_uf->valueint, siafi_id->valueint, ddd->valueint, capital->valueint, nome->valuestring, fuso->valuestring, lati->valuedouble, longi->valuedouble)) == EXIT_SUCCESS);
        assert(hash_insere(h2, aloca_municipio(aux, cod_uf->valueint, siafi_id->valueint, ddd->valueint, capital->valueint, nome->valuestring, fuso->valuestring, lati->valuedouble, longi->valuedouble)) == EXIT_SUCCESS);
        *raiznode = kdtree_insere(*raiznode, aloca_municipio(aux, cod_uf->valueint, siafi_id->valueint, ddd->valueint, capital->valueint, nome->valuestring, fuso->valuestring, lati->valuedouble, longi->valuedouble), nivel);
        

        objeto = objeto->next;
    }

    cJSON_Delete(raiz);
}

tmunicipio **colisao(thash h, const char *key, int *num_opcoes)
{
    int i = 0;
    tmunicipio **opcoes = NULL;

    int pos = hashf(key, SEED) % (h.max);
    int hdd = hashf2(key) % (h.max - 1) + 1;
    ;
    while (h.table[pos] != 0)
    {
        if (strcmp(h.get_key((void *)h.table[pos]), key) == 0){
       
            opcoes = realloc(opcoes, (i + 1) * sizeof(tmunicipio *));
            opcoes[i] = (tmunicipio *)h.table[pos];
            i++;
        }
        pos = (pos + hdd) % h.max;
    }
    assert(opcoes != NULL);
    *num_opcoes = i;
    return opcoes;
}

int main()
{
    tnode *raiz = NULL;
    thash h;
    thash h2;
    int nbuckets = 5580;
    tmunicipio *muni;
    tmunicipio **munih2 = NULL;
    int escolha = 999999;
    int n_colisao;

    hash_constroi(&h, nbuckets, get_key_municipio);
    hash_constroi(&h2, nbuckets, get_key_municipio2);
    lerjson(&h, &h2, &raiz, 0);
    //muni = hash_busca(h, "5002704"); // tarefa 1 inserir cod_ibge aqui (string)
    //printf("%s\n\n", muni->nome_municipio);
    //busca_vizinhos(raiz, muni, 3);  // tarefa 2 

    munih2 = colisao(h2,  "Terenos", &n_colisao); // tarefa 3 - inserir nome da cidade aqui
    
    //escolhe cidade em caso de mais de uma com o mesmo nome
    while (escolha == 999999 && n_colisao >= 2){
        printf("\nForam encontradas %d municipios com o mesmo nome!\nEscolha qual voce deseja digitando o numero apresentado na frente\n", n_colisao );
        for (int i = 0 ; i < n_colisao; i++){

            printf("%d - %s - Codigo ibge - %s\n", i, munih2[i]->nome_municipio,munih2[i]->cod_ibge);
        }

        scanf("%d", &escolha);
        if (escolha > n_colisao-1){
            printf("escolha invalida\n");
            escolha = 999999;
        } 
    } 
    if (escolha == 999999 )
        escolha = 0;
    
    busca_vizinhos(raiz, munih2[escolha], 10); // inserir quantidade de vizinhos desejado aqui
    hash_apaga(&h);
    hash_apaga(&h2);
}