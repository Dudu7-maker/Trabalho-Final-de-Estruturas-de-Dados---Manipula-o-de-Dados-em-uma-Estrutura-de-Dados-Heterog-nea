#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef enum { RESIDENTE = 1, SENSOR = 2, ALERTA = 3 } RecordType;
typedef long long ll;

typedef struct {
    int id;
    char nome[100];
    char cidade[50];
    char bairro[50];
    char telefone[30];
    int moradores;
} Residente;

typedef struct {
    int id;
    char modelo[50];
    char localizacao[100];
    double leitura; 
    time_t timestamp;
} Sensor;

typedef struct {
    int id;
    char mensagem[200];
    int prioridade; 
    char area_afetada[100];
    time_t timestamp;
} Alerta;

typedef struct {
    RecordType type;
    time_t timestamp;
    union {
        Residente r;
        Sensor s;
        Alerta a;
    } data;
} Record;

typedef struct Node {
    Record rec;
    struct Node* next;
} Node;

Node* criar_no(Record rec);
int inserir_registro(Node** head, Record rec);
int remover_por_id(Node** head, int id);
Record* buscar_por_id(Node* head, int id);
int alterar_registro(Node* head, int id, Record novos);
void listar_todos(Node* head);
void liberar_lista(Node** head);
int exportar_arquivo(Node* head, const char* filename);
int importar_arquivo(Node** head, const char* filename);
int contar_registros(Node* head);

Node* criar_no(Record rec) {
    Node* n = (Node*)malloc(sizeof(Node));
    if(!n) return NULL;
    n->rec = rec;
    n->next = NULL;
    return n;
}


static int cmp_record(const Record* a, const Record* b) {
   
    if (a->type != b->type) {
        if (a->type == ALERTA) return -1;
        if (b->type == ALERTA) return 1;

        return (int)a->type - (int)b->type;
    }
    
    if (a->type == ALERTA) {
        if (a->data.a.prioridade != b->data.a.prioridade)
            return a->data.a.prioridade - b->data.a.prioridade; 
        if (a->timestamp > b->timestamp) return -1;
        if (a->timestamp < b->timestamp) return 1;
        return 0;
    } else {
        if (a->timestamp > b->timestamp) return -1;
        if (a->timestamp < b->timestamp) return 1;
        return 0;
    }
}


int inserir_registro(Node** head, Record rec) {
    if(!head) return -1;
   
    Node* cur = *head;
    while(cur) {
        if ( (cur->rec.type == rec.type) && 
             ( (rec.type==RESIDENTE && cur->rec.data.r.id==rec.data.r.id) ||
               (rec.type==SENSOR && cur->rec.data.s.id==rec.data.s.id) ||
               (rec.type==ALERTA && cur->rec.data.a.id==rec.data.a.id) ) ) {
            return 1; 
        }
        cur = cur->next;
    }
    Node* novo = criar_no(rec);
    if(!novo) return -1;
    
    if (*head == NULL) {
        *head = novo;
        return 0;
    }
    if (cmp_record(&novo->rec, &(*head)->rec) < 0) {
        novo->next = *head;
        *head = novo;
        return 0;
    }
    Node* prev = *head;
    cur = (*head)->next;
    while(cur && cmp_record(&novo->rec, &cur->rec) >= 0) {
        prev = cur;
        cur = cur->next;
    }
    prev->next = novo;
    novo->next = cur;
    return 0;
}

int remover_por_id(Node** head, int id) {
    if(!head || !*head) return 1;
    Node* cur = *head;
    Node* prev = NULL;
    while(cur) {
        int this_id = 0;
        if (cur->rec.type == RESIDENTE) this_id = cur->rec.data.r.id;
        else if (cur->rec.type == SENSOR) this_id = cur->rec.data.s.id;
        else this_id = cur->rec.data.a.id;
        if (this_id == id) {
            if (prev) prev->next = cur->next;
            else *head = cur->next;
            free(cur);
            if (*head == NULL) {
                printf("Aviso: todos os registros foram removidos.\n");
            }
            return 0;
        }
        prev = cur;
        cur = cur->next;
    }
    return 1; 
}

Record* buscar_por_id(Node* head, int id) {
    Node* cur = head;
    while(cur) {
        int this_id = 0;
        if (cur->rec.type == RESIDENTE) this_id = cur->rec.data.r.id;
        else if (cur->rec.type == SENSOR) this_id = cur->rec.data.s.id;
        else this_id = cur->rec.data.a.id;
        if (this_id == id) return &cur->rec;
        cur = cur->next;
    }
    return NULL;
}


int alterar_registro(Node* head, int id, Record novos) {
    Record* r = buscar_por_id(head, id);
    if(!r) return 1;
    
    *r = novos;
    return 0;
}

void listar_todos(Node* head) {
    Node* cur = head;
    char buf[64];
    if (!cur) { printf("Lista vazia.\n"); return; }
    while(cur) {
        struct tm *tm = localtime(&cur->rec.timestamp);
        strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", tm);
        if (cur->rec.type == RESIDENTE) {
            Residente *r = &cur->rec.data.r;
            printf("[RESIDENTE] id:%d nome:%s cidade:%s bairro:%s moradores:%d tel:%s time:%s\n",
                r->id, r->nome, r->cidade, r->bairro, r->moradores, r->telefone, buf);
        } else if (cur->rec.type == SENSOR) {
            Sensor *s = &cur->rec.data.s;
            printf("[SENSOR] id:%d modelo:%s local:%s leitura:%.2f time:%s\n",
                s->id, s->modelo, s->localizacao, s->leitura, buf);
        } else {
            Alerta *a = &cur->rec.data.a;
            printf("[ALERTA] id:%d prioridade:%d area:%s msg:%s time:%s\n",
                a->id, a->prioridade, a->area_afetada, a->mensagem, buf);
        }
        cur = cur->next;
    }
}

void liberar_lista(Node** head) {
    if (!head) return;
    Node* cur = *head;
    while(cur) {
        Node* tmp = cur;
        cur = cur->next;
        free(tmp);
    }
    *head = NULL;
}


int exportar_arquivo(Node* head, const char* filename) {
    FILE* f = fopen(filename, "w");
    if(!f) return -1;
    Node* cur = head;
    while(cur) {
        if (cur->rec.type == RESIDENTE) {
            Residente *r = &cur->rec.data.r;
            fprintf(f, "R|%d|%s|%s|%s|%s|%d|%lld\n",
                r->id, r->nome, r->cidade, r->bairro, r->telefone, r->moradores, (ll)cur->rec.timestamp);
        } else if (cur->rec.type == SENSOR) {
            Sensor *s = &cur->rec.data.s;
            fprintf(f, "S|%d|%s|%s|%.6f|%lld\n",
                s->id, s->modelo, s->localizacao, s->leitura, (ll)cur->rec.timestamp);
        } else {
            Alerta *a = &cur->rec.data.a;
            
            fprintf(f, "A|%d|%s|%d|%s|%lld\n",
                a->id, a->mensagem, a->prioridade, a->area_afetada, (ll)cur->rec.timestamp);
        }
        cur = cur->next;
    }
    fclose(f);
    printf("Base salva em '%s'.\n", filename);
    return 0;
}


int importar_arquivo(Node** head, const char* filename) {
    FILE* f = fopen(filename, "r");
    if(!f) return -1;
    char line[512];
    int line_no = 0;
    while(fgets(line, sizeof line, f)) {
        line_no++;
      
        char *p = strchr(line, '\n'); if(p) *p = 0;
        if (strlen(line) == 0) continue;
        char type = line[0];
        Record rec; memset(&rec,0,sizeof rec);
        rec.timestamp = time(NULL);
        if (type == 'R' && line[1]=='|') {
            Residente r; memset(&r,0,sizeof r);
            
            char nome[100], cidade[50], bairro[50], telefone[30];
            int id, moradores;
            ll ts;
            int ok = sscanf(line+2, "%d|%99[^|]|%49[^|]|%49[^|]|%29[^|]|%d|%lld",
                &id, nome, cidade, bairro, telefone, &moradores, &ts);
            if (ok >= 7) {
                r.id = id;
                strncpy(r.nome, nome, sizeof r.nome-1);
                strncpy(r.cidade, cidade, sizeof r.cidade-1);
                strncpy(r.bairro, bairro, sizeof r.bairro-1);
                strncpy(r.telefone, telefone?telefone:telefone, sizeof r.telefone-1); 
                
                r.moradores = moradores;
                rec.type = RESIDENTE;
                rec.timestamp = (time_t)ts;
                rec.data.r = r;
                int res = inserir_registro(head, rec);
                (void)res;
            } else {
               
            }
        } else if (type == 'S' && line[1]=='|') {
            Sensor s; memset(&s,0,sizeof s);
            int id; double leitura; ll ts;
            char modelo[50], local[100];
            int ok = sscanf(line+2, "%d|%49[^|]|%99[^|]|%lf|%lld",
                &id, modelo, local, &leitura, &ts);
            if (ok >= 5) {
                s.id = id;
                strncpy(s.modelo, modelo, sizeof s.modelo-1);
                strncpy(s.localizacao, local, sizeof s.localizacao-1);
                s.leitura = leitura;
                s.timestamp = (time_t)ts;
                rec.type = SENSOR;
                rec.timestamp = (time_t)ts;
                rec.data.s = s;
                inserir_registro(head, rec);
            }
        } else if (type == 'A' && line[1]=='|') {
            Alerta a; memset(&a,0,sizeof a);
            int id, prioridade; ll ts;
            char mensagem[200], area[100];
            int ok = sscanf(line+2, "%d|%199[^|]|%d|%99[^|]|%lld",
                &id, mensagem, &prioridade, area, &ts);
            if (ok >= 5) {
                a.id = id;
                strncpy(a.mensagem, mensagem, sizeof a.mensagem-1);
                a.prioridade = prioridade;
                strncpy(a.area_afetada, area, sizeof a.area_afetada-1);
                a.timestamp = (time_t)ts;
                rec.type = ALERTA;
                rec.timestamp = (time_t)ts;
                rec.data.a = a;
                inserir_registro(head, rec);
            }
        } else {
        
        }
    }
    fclose(f);
    printf("Importação finalizada (arquivo: %s).\n", filename);
    return 0;
}

int contar_registros(Node* head) {
    int c = 0;
    while(head) { c++; head = head->next; }
    return c;
}


static int read_int(const char* prompt) {
    char buf[100];
    printf("%s", prompt);
    if(!fgets(buf,sizeof buf,stdin)) return 0;
    return atoi(buf);
}
static void read_str(const char* prompt, char* out, int n) {
    printf("%s", prompt);
    if(!fgets(out,n,stdin)) { out[0]=0; return; }
    char *p = strchr(out, '\n'); if(p) *p = 0;
}

int main() {
    Node* head = NULL;
    const char* DB = "enparivis_db.txt";
    importar_arquivo(&head, DB); 

    while(1) {
        printf("\n=== ENPARIVIS - MENU ===\n");
        printf("1 Inserir registro\n2 Listar todos\n3 Buscar por id\n4 Alterar por id\n5 Remover por id\n6 Exportar\n7 Importar\n8 Contar\n9 Sair\nEscolha: ");
        int opt = read_int("");
        if (opt == 1) {
            int t = read_int("Tipo (1-Residente,2-Sensor,3-Alerta): ");
            Record rec; memset(&rec,0,sizeof rec);
            rec.type = (RecordType)t;
            rec.timestamp = time(NULL);
            if (t == RESIDENTE) {
                Residente r; memset(&r,0,sizeof r);
                r.id = read_int("ID (inteiro): ");
                read_str("Nome: ", r.nome, sizeof r.nome);
                read_str("Cidade: ", r.cidade, sizeof r.cidade);
                read_str("Bairro: ", r.bairro, sizeof r.bairro);
                read_str("Telefone: ", r.telefone, sizeof r.telefone);
                r.moradores = read_int("Numero de moradores: ");
                rec.data.r = r;
            } else if (t == SENSOR) {
                Sensor s; memset(&s,0,sizeof s);
                s.id = read_int("ID (inteiro): ");
                read_str("Modelo: ", s.modelo, sizeof s.modelo);
                read_str("Localizacao: ", s.localizacao, sizeof s.localizacao);
                char tmp[50];
                read_str("Leitura (ex: 3.14): ", tmp, sizeof tmp);
                s.leitura = atof(tmp);
                s.timestamp = time(NULL);
                rec.data.s = s;
            } else if (t == ALERTA) {
                Alerta a; memset(&a,0,sizeof a);
                a.id = read_int("ID (inteiro): ");
                read_str("Mensagem: ", a.mensagem, sizeof a.mensagem);
                a.prioridade = read_int("Prioridade (1-alta,2-media,3-baixa): ");
                read_str("Area afetada: ", a.area_afetada, sizeof a.area_afetada);
                a.timestamp = time(NULL);
                rec.data.a = a;
            } else {
                printf("Tipo invalido.\n");
                continue;
            }
            int res = inserir_registro(&head, rec);
            if (res == 0) printf("Insercao bem sucedida.\n");
            else if (res == 1) printf("Erro: ID duplicado. Inclusao abortada.\n");
            else printf("Erro de memoria ou desconhecido.\n");
        } else if (opt == 2) {
            listar_todos(head);
        } else if (opt == 3) {
            int id = read_int("ID a buscar: ");
            Record* r = buscar_por_id(head, id);
            if (!r) printf("Registro nao encontrado.\n");
            else {
                char buf[64];
                strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", localtime(&r->timestamp));
                if (r->type == RESIDENTE) {
                    Residente *x = &r->data.r;
                    printf("RESIDENTE id:%d nome:%s cidade:%s bairro:%s tel:%s moradores:%d time:%s\n",
                        x->id, x->nome, x->cidade, x->bairro, x->telefone, x->moradores, buf);
                } else if (r->type == SENSOR) {
                    Sensor *s = &r->data.s;
                    printf("SENSOR id:%d modelo:%s local:%s leitura:%.2f time:%s\n",
                        s->id, s->modelo, s->localizacao, s->leitura, buf);
                } else {
                    Alerta *a = &r->data.a;
                    printf("ALERTA id:%d pri:%d area:%s msg:%s time:%s\n",
                        a->id, a->prioridade, a->area_afetada, a->mensagem, buf);
                }
            }
        } else if (opt == 4) {
            int id = read_int("ID a alterar: ");
            Record* r = buscar_por_id(head, id);
            if (!r) { printf("Registro nao encontrado.\n"); continue; }
            Record novos = *r;
            if (r->type == RESIDENTE) {
                read_str("Novo nome (enter para manter): ", novos.data.r.nome, sizeof novos.data.r.nome);
                read_str("Nova cidade (enter para manter): ", novos.data.r.cidade, sizeof novos.data.r.cidade);
                read_str("Novo bairro (enter para manter): ", novos.data.r.bairro, sizeof novos.data.r.bairro);
                read_str("Novo telefone (enter para manter): ", novos.data.r.telefone, sizeof novos.data.r.telefone);
                char tmp[20];
                read_str("Novo numero de moradores (enter para manter): ", tmp, sizeof tmp);
                if (strlen(tmp)) novos.data.r.moradores = atoi(tmp);
            } else if (r->type == SENSOR) {
                read_str("Novo modelo (enter para manter): ", novos.data.s.modelo, sizeof novos.data.s.modelo);
                read_str("Nova localizacao (enter para manter): ", novos.data.s.localizacao, sizeof novos.data.s.localizacao);
                char tmp[20];
                read_str("Nova leitura (enter para manter): ", tmp, sizeof tmp);
                if (strlen(tmp)) novos.data.s.leitura = atof(tmp);
                novos.timestamp = time(NULL);
            } else {
                read_str("Nova mensagem (enter para manter): ", novos.data.a.mensagem, sizeof novos.data.a.mensagem);
                char tmp[10];
                read_str("Nova prioridade (enter para manter): ", tmp, sizeof tmp);
                if (strlen(tmp)) novos.data.a.prioridade = atoi(tmp);
                read_str("Nova area (enter para manter): ", novos.data.a.area_afetada, sizeof novos.data.a.area_afetada);
                novos.timestamp = time(NULL);
            }
            alterar_registro(head, id, novos);
            printf("Alteracao efetuada.\n");
        } else if (opt == 5) {
            int id = read_int("ID a remover: ");
            int r = remover_por_id(&head, id);
            if (r == 0) printf("Removido com sucesso.\n");
            else printf("Registro nao encontrado.\n");
        } else if (opt == 6) {
            char fname[200];
            read_str("Nome do arquivo para exportar: ", fname, sizeof fname);
            if (strlen(fname)==0) strcpy(fname, "enparivis_db.txt");
            exportar_arquivo(head, fname);
        } else if (opt == 7) {
            char fname[200];
            read_str("Nome do arquivo para importar: ", fname, sizeof fname);
            if (strlen(fname)==0) strcpy(fname, "enparivis_db.txt");
            importar_arquivo(&head, fname);
        } else if (opt == 8) {
            printf("Total de registros: %d\n", contar_registros(head));
        } else if (opt == 9) {
            char ans[10];
            read_str("Deseja salvar a base antes de sair? (s/n): ", ans, sizeof ans);
            if (ans[0]=='s' || ans[0]=='S') exportar_arquivo(head, DB);
            liberar_lista(&head);
            printf("Saindo...\n");
            break;
        } else {
            printf("Opcao invalida.\n");
        }
    }

    return 0;
}