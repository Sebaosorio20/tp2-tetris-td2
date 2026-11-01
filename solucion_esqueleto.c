// Uso:  ./tetris           corre los tests y despues el juego
//       ./tetris --test    corre solo los tests (valgrind --leak-check=full ./tetris --test)

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// ========= CONSTANTES DEL JUEGO =========
#define SCREEN_WIDTH 500
#define SCREEN_HEIGHT 800
#define GRID_ROWS 20
#define GRID_COLS 10
#define CELL_SIZE 35
#define OFFSET_X 75
#define OFFSET_Y 50
#define TICK_RATE 30

const int TETROMINOS[7][4][4][4] = {
    { {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}}, {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}}, {{0,0,0,0},{0,0,0,0},{1,1,1,1},{0,0,0,0}}, {{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}} },
    { {{2,0,0,0},{2,2,2,0},{0,0,0,0},{0,0,0,0}}, {{0,2,2,0},{0,2,0,0},{0,2,0,0},{0,0,0,0}}, {{0,0,0,0},{2,2,2,0},{0,0,2,0},{0,0,0,0}}, {{0,2,0,0},{0,2,0,0},{2,2,0,0},{0,0,0,0}} },
    { {{0,0,3,0},{3,3,3,0},{0,0,0,0},{0,0,0,0}}, {{0,3,0,0},{0,3,0,0},{0,3,3,0},{0,0,0,0}}, {{0,0,0,0},{3,3,3,0},{3,0,0,0},{0,0,0,0}}, {{3,3,0,0},{0,3,0,0},{0,3,0,0},{0,0,0,0}} },
    { {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}}, {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}}, {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}}, {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}} },
    { {{0,5,5,0},{5,5,0,0},{0,0,0,0},{0,0,0,0}}, {{0,5,0,0},{0,5,5,0},{0,0,5,0},{0,0,0,0}}, {{0,0,0,0},{0,5,5,0},{5,5,0,0},{0,0,0,0}}, {{5,0,0,0},{5,5,0,0},{0,5,0,0},{0,0,0,0}} },
    { {{0,6,0,0},{6,6,6,0},{0,0,0,0},{0,0,0,0}}, {{0,6,0,0},{0,6,6,0},{0,6,0,0},{0,0,0,0}}, {{0,0,0,0},{6,6,6,0},{0,6,0,0},{0,0,0,0}}, {{0,6,0,0},{6,6,0,0},{0,6,0,0},{0,0,0,0}} },
    { {{7,7,0,0},{0,7,7,0},{0,0,0,0},{0,0,0,0}}, {{0,0,7,0},{0,7,7,0},{0,7,0,0},{0,0,0,0}}, {{0,0,0,0},{7,7,0,0},{0,7,7,0},{0,0,0,0}}, {{0,7,0,0},{7,7,0,0},{7,0,0,0},{0,0,0,0}} }
};

SDL_Color colors[8] = {
    {30, 30, 30, 255}, {0, 255, 255, 255}, {0, 0, 255, 255}, {255, 165, 0, 255},
    {255, 255, 0, 255}, {0, 255, 0, 255}, {128, 0, 128, 255}, {255, 0, 0, 255}
};

// ========= ESTRUCTURAS DE DATOS =========

// 1. Nodo para la fila del tablero (Lista Enlazada)
typedef struct RowNode {
    int cells[GRID_COLS]; 
    struct RowNode* next; 
} RowNode;

// 2. Nodo para la cola de próximas piezas (Queue)
typedef struct PieceQueueNode {
    int type; 
    struct PieceQueueNode* next;
} PieceQueueNode;

// 3. Nodo para el sistema de partículas 
typedef struct ParticleNode {
    float x, y;
    float vel_x, vel_y;
    int color;
    int life_timer;
    struct ParticleNode* next;
} ParticleNode;

typedef struct ActivePiece {
    int col, row;
    int type;      
    int rotation;  
    int active;
} ActivePiece;

typedef struct GameBoard {
    RowNode* first_row;             
    PieceQueueNode* queue_head;     
    PieceQueueNode* queue_tail;     
    ParticleNode* first_particle;   
    ActivePiece current_piece;
    int score;
    int tick_timer;
} GameBoard;

// ========= VARIABLES GLOBALES =========
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
GameBoard* game_board = NULL;

// ========= EJERCICIO 1: STRINGS (NO USAR string.h) =========

// Función auxiliar para contar el largo del string
// Recorro hasta encontrar el '\0' y voy contando (el '\0' no se cuenta)

int strLarge(char* a) {
    int total = 0;
    while (a[total] != '\0') {
        total++;
    }
    return total;
}

char* strDuplicate(char* src) {
    int total  = strLarge(src);

    // pido total+1 bytes, los caracteres mas uno para el '\0' del final
    char* res = (char*)malloc(sizeof(char)*(total+1));

    //Si malloc falla devuelvo NULL
    if (res == NULL){
        return NULL;
    }

    //copio cada uno de los caracteres
    for (int i=0; i<total; i++){
        res[i] = src[i];
    }

    //agrego el '\0'
    res[total] = '\0';

    return res;
}

int strCompare(char* s1, char* s2) {
    // TODO: Devolver 0 si son iguales, 1 si s1 < s2, -1 si s1 > s2.
    //       Ojo: es la convencion INVERSA a strcmp.

    //avanzo mientras los caracteres sean iguales y no haya terminado ninguno
    int i = 0;
    while (s1[i] != '\0' && s2[i] != '\0' && s1[i] == s2[i]) {
        i++;
    }

    if (s1[i] == s2[i]) {
        return 0;
    }
    if (s1[i] < s2[i]) {
        return 1;
    }
    return -1;
}


char* strConcatenate(char* src1, char* src2) {
    // TODO: Reservar un string nuevo con src1 seguido de src2, y liberar src1 y src2.
    //       Precondicion: src1 y src2 vienen de malloc y son punteros distintos.
    int total1 = strLarge(src1);
    int total2 = strLarge(src2);

    //total1 + total2 caracteres mas el '\0'
    char* res = (char*)malloc(sizeof(char)*(total1 + total2 +1));

    // si falla devuelvo NULL y NO libero src1 ni src2
    if (res == NULL){
        return NULL;
    }

    //copio src1
    for( int i = 0; i < total1; i++){
        res[i] = src1[i];
    }

    for (int i = 0; i < total2; i++){
        res[total1 + i] = src2[i];                //res[total1 + i] para no pisar src1
    }
    
    res[total1 + total2] = '\0';

    free(src1);
    free(src2);

    return res;
}

// ========= EJERCICIO 2: FUNCIONES A IMPLEMENTAR (EL TP) =========

GameBoard* gameBoardNew() {
    // TODO: Reservar memoria para el GameBoard.
    // TODO: Usar un bucle para hacer malloc de las 20 filas (RowNode) y conectarlas.
    // TODO: El puntero first_row debe apuntar al primero de esos nodos.
    // TODO: Inicializar queue_head, queue_tail y first_particle en NULL.
    GameBoard* board = (GameBoard*)malloc(sizeof(GameBoard));
    if(board == NULL){
        return NULL;
    }
    // inicializo todo antes de reservar las filas
    board ->first_row = NULL;
    board ->queue_head = NULL;
    board ->queue_tail = NULL;
    board ->first_particle = NULL;
    board ->score = 0;
    board ->tick_timer = 0;
    board ->current_piece.col = 0;
    board ->current_piece.row = 0;
    board -> current_piece.type = 0;
    board ->current_piece.rotation = 0;
    board ->current_piece.active = 0;


    // aca hacemos un malloc por cada fila y las vamos enlazando.
    // 'ultima' apunta a la ultima fila creada para poder ?engancharle' la siguiente
    RowNode* ultima = NULL;
    for(int i = 0; i<GRID_ROWS; i++){
        RowNode* nueva = (RowNode*)malloc(sizeof(RowNode));
        if (nueva == NULL){                                       //si nueva == NULL libero las filas
            RowNode* curr = board -> first_row;
            while (curr != NULL){
                RowNode* sig = curr-> next;
                free(curr);
                curr = sig;
            }
            free(board);
            return NULL;
        }

        for (int c= 0; c<GRID_COLS; c++){
            nueva->cells[c] = 0;
        }
        nueva -> next = NULL;
        if (ultima == NULL){
            board -> first_row = nueva;             //la primera es la 'cima'
        }else{
            ultima -> next = nueva;                 //enlazo con la anterior
        }
        ultima = nueva;
    }
    return board;
}


void gameBoardDelete(GameBoard* board) {
    // TODO: Debe tolerar board == NULL.
    // TODO: Recorrer y liberar todos los RowNode con free().
    // TODO: Recorrer y liberar todos los PieceQueueNode.
    // TODO: Recorrer y liberar todos los ParticleNode.
    // TODO: Finalmente, liberar la estructura GameBoard.
    if (board == NULL){
        return ;
    }

    //guardo la 'sguiente' antes de hacer free para no perder el enlace al siguiente
    RowNode* fila = board ->first_row;
    while(fila != NULL){
        RowNode* siguiente = fila->next;
        free(fila);
        fila = siguiente;
    }

    PieceQueueNode* pieza = board -> queue_head;
    while (pieza != NULL){
        PieceQueueNode* siguiente = pieza->next;
        free(pieza);
        pieza = siguiente;
    }

    ParticleNode* particula = board -> first_particle;
    while(particula != NULL){
        ParticleNode* siguiente = particula-> next;
        free(particula);
        particula = siguiente;
    }
    free(board);

}

void gameBoardAddParticle(GameBoard* board, float x, float y, int color) {
    // TODO: Crear un ParticleNode con memoria dinámica.
    // TODO: Inicializar x, y, velocidades aleatorias y un life_timer.
    // TODO: Agregarlo al principio de la lista enlazada simple de partículas.
    if (board == NULL){
        return ;
    }

    ParticleNode* nuevo = (ParticleNode*)malloc(sizeof(ParticleNode));
    if (nuevo == NULL){
        return ;
    }
    nuevo ->x = x;
    nuevo-> y= y;
    nuevo->vel_x = ((rand()%100)/10.0f)-5.0f;
    nuevo->vel_y = ((rand()%100)/10.0f)-5.0f;
    nuevo-> color = color;
    nuevo-> life_timer = 30 + (rand()%30);            
    nuevo->next = board ->first_particle;
    board->first_particle = nuevo;
}

int gameBoardCheckAndClearLines(GameBoard* board) {
    // TODO: Recorrer la lista de RowNode.
    // TODO: Si un RowNode tiene todas sus celdas > 0, desenlazarlo y hacer free().
    // TODO: Crear un nuevo RowNode vacío (lleno de ceros).
    // TODO: Insertar el nuevo RowNode en la CABEZA del tablero (board->first_row).
    // TODO: Por cada celda destruida, invocar a gameBoardAddParticle().
    if (board == NULL) {
        return 0;
    }

    // recorro la lista con dos punteros: current (fila actual) y prev (la anterior),
    // necesito prev para poder desenlazar current. 'indice' es la posicion de current
    int lineas = 0;
    int indice = 0;
    RowNode* prev = NULL;
    RowNode* current = board->first_row;

    while (current != NULL) {
        int completa = 1;
        for (int t = 0; t <GRID_COLS; t++) {
            if (current->cells[t] == 0) {
                completa = 0;
            }
        }
        if (completa) {
            RowNode* nueva = (RowNode*)malloc(sizeof(RowNode));
            if (nueva == NULL) {
                // No pude crear la fila de reemplazo: dejo esta fila y sigo.
                prev = current;
                current = current->next;
                indice++;
            } else {
                for (int t = 0; t< GRID_COLS; t++) {
                    nueva->cells[t] = 0;
                    gameBoardAddParticle(board, OFFSET_X + t * CELL_SIZE + CELL_SIZE / 2, OFFSET_Y + indice * CELL_SIZE + CELL_SIZE / 2, current->cells[t]);
                }
                // desenlazo current: si no tiene anterior era la primera, entonces cambia first_row;
                // si no, el anterior pasa a apuntar al siguiente. Despues recien hago el free
                
                RowNode* siguiente = current->next;
                if (prev == NULL) {
                    board->first_row = siguiente;
                } else {
                    prev->next = siguiente;
                }
                free(current);
                nueva->next = board->first_row;        //agrego la fila vacia arriba de todo
                board->first_row = nueva;
                if (prev == NULL) {
                    prev = nueva;
                }
                lineas++;
                current = siguiente;
                indice++;
            }
        } else {
            prev = current;
            current = current->next;
            indice++;
        }
    }
    return lineas;
}

void gameBoardEnqueuePiece(GameBoard* board, int type) {
    if (board == NULL) {
        return;
    }

    PieceQueueNode* nuevo = (PieceQueueNode*)malloc(sizeof(PieceQueueNode));
    if (nuevo == NULL) {
        return;
    }
    // el nuevo va a ser el ultimo, entonces su next es NULL
    nuevo->type = type;
    nuevo->next = NULL;

    if (board->queue_tail == NULL) {
        board->queue_head = nuevo;
        board->queue_tail = nuevo;
    } else {
        board->queue_tail->next = nuevo;
        board->queue_tail = nuevo;
    }
}


int gameBoardDequeuePiece(GameBoard* board, int* out_type) {
    // TODO: Si la cola está vacía, retornar 0.
    // TODO: Extraer el primer nodo (queue_head).
    // TODO: Guardar el type en la variable out_type, liberar el nodo y actualizar el head.
    if(board == NULL || board->queue_head == NULL){
        return 0;
    }

    // guardo el nodo de la cabeza en 'nodo' para poder liberarlo despues de mover el head

    PieceQueueNode* nodo = board -> queue_head;
    int tipo = (*nodo).type;
    *out_type = tipo;
    board->queue_head= nodo->next;
    // si la cola quedo vacia tengo que poner tambien tail en NULL,
    // si no quedaria apuntando a un nodo ya liberado
    if(board->queue_head == NULL){
        board ->queue_tail = NULL;
    }
    free(nodo);
    
    return 1;
}

// ========= LOGICA DE DIBUJADO Y COLISIONES (NO MODIFICAR) =========

RowNode* getRowAt(GameBoard* board, int index) {
    RowNode* curr = board->first_row;
    for(int i = 0; i < index && curr != NULL; i++) curr = curr->next;
    return curr;
}

int canMove(GameBoard* board, int target_col, int target_row, int target_rot) {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (TETROMINOS[board->current_piece.type][target_rot][r][c] != 0) {
                int board_col = target_col + c;
                int board_row = target_row + r;
                if (board_col < 0 || board_col >= GRID_COLS || board_row >= GRID_ROWS) return 0;
                if (board_row >= 0) {
                    RowNode* row_node = getRowAt(board, board_row);
                    if (row_node && row_node->cells[board_col] != 0) return 0;
                }
            }
        }
    }
    return 1;
}

void lockPiece(GameBoard* board) {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            int val = TETROMINOS[board->current_piece.type][board->current_piece.rotation][r][c];
            if (val != 0) {
                RowNode* row_node = getRowAt(board, board->current_piece.row + r);
                if (row_node) row_node->cells[board->current_piece.col + c] = val;
            }
        }
    }
    board->current_piece.active = 0;
    board->score += gameBoardCheckAndClearLines(board) * 100;
}

// Devuelve 0 si la pieza nueva no entra (GAME OVER), 1 en otro caso.
int spawnNextPiece(GameBoard* board) {
    int type;
    if (gameBoardDequeuePiece(board, &type)) {
        board->current_piece.type = type;
        board->current_piece.rotation = 0;
        board->current_piece.col = GRID_COLS / 2 - 2;
        board->current_piece.row = 0;
        board->current_piece.active = 1;
        gameBoardEnqueuePiece(board, rand() % 7);
        if (!canMove(board, board->current_piece.col, board->current_piece.row, board->current_piece.rotation)) {
            printf("GAME OVER. Puntaje final: %d\n", board->score);
            board->current_piece.active = 0;
            return 0;   // main sale del loop y libera tablero y SDL
        }
    }
    return 1;
}

// Devuelve 0 cuando termina la partida.
int gameBoardUpdate(GameBoard* board) {
    if (!board) return 0;
    if (!board->current_piece.active && !spawnNextPiece(board)) return 0;

    board->tick_timer++;
    if (board->tick_timer >= TICK_RATE) {
        board->tick_timer = 0;
        if (canMove(board, board->current_piece.col, board->current_piece.row + 1, board->current_piece.rotation)) {
            board->current_piece.row++;
        } else {
            lockPiece(board);
        }
    }

    ParticleNode* curr_part = board->first_particle;
    ParticleNode* prev_part = NULL;
    while (curr_part) {
        curr_part->x += curr_part->vel_x;
        curr_part->y += curr_part->vel_y;
        curr_part->vel_y += 0.5f;
        curr_part->life_timer--;

        if (curr_part->life_timer <= 0) {
            ParticleNode* to_delete = curr_part;
            if (prev_part == NULL) {
                board->first_particle = curr_part->next;
                curr_part = board->first_particle;
            } else {
                prev_part->next = curr_part->next;
                curr_part = prev_part->next;
            }
            free(to_delete);
        } else {
            prev_part = curr_part;
            curr_part = curr_part->next;
        }
    }
    return 1;
}

void gameBoardDraw(GameBoard* board) {
    if (!board) return;
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect board_rect = {OFFSET_X - 2, OFFSET_Y - 2, GRID_COLS * CELL_SIZE + 4, GRID_ROWS * CELL_SIZE + 4};
    SDL_RenderDrawRect(renderer, &board_rect);

    RowNode* current_row = board->first_row;
    int r = 0;
    while (current_row) {
        for (int c = 0; c < GRID_COLS; c++) {
            if (current_row->cells[c] > 0) {
                SDL_Color col = colors[current_row->cells[c]];
                SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
                SDL_Rect cell_rect = {OFFSET_X + c * CELL_SIZE, OFFSET_Y + r * CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1};
                SDL_RenderFillRect(renderer, &cell_rect);
            }
        }
        current_row = current_row->next;
        r++;
    }

    if (board->current_piece.active) {
        for(int pr = 0; pr < 4; pr++) {
            for(int pc = 0; pc < 4; pc++) {
                int cell_val = TETROMINOS[board->current_piece.type][board->current_piece.rotation][pr][pc];
                if (cell_val != 0) {
                    SDL_Color col = colors[cell_val];
                    SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
                    SDL_Rect p_rect = {OFFSET_X + (board->current_piece.col + pc) * CELL_SIZE, 
                                       OFFSET_Y + (board->current_piece.row + pr) * CELL_SIZE, 
                                       CELL_SIZE - 1, CELL_SIZE - 1};
                    SDL_RenderFillRect(renderer, &p_rect);
                }
            }
        }
    }

    ParticleNode* curr_part = board->first_particle;
    while (curr_part) {
        SDL_Color col = colors[curr_part->color];
        int alpha = curr_part->life_timer * 8;   // se satura: SDL recibe un Uint8
        if (alpha > 255) alpha = 255;
        SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, alpha);
        SDL_Rect part_rect = {(int)curr_part->x, (int)curr_part->y, 6, 6};
        SDL_RenderFillRect(renderer, &part_rect);
        curr_part = curr_part->next;
    }
    SDL_RenderPresent(renderer);
}

int inicializar() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 0;
    window = SDL_CreateWindow("Tetris Base - TP", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) return 0;
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) return 0;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    return 1;
}

// ========= TESTS (COMPLETAR) =========

void correrTests() {
    // TODO: Implementar los casos minimos del enunciado. Por ejemplo:
    //  - strDuplicate: "", "x", un string con distintos caracteres.
    //  - strCompare: "" vs "", "a" vs "a", y una desigualdad en ambos sentidos (s1/s2 y s2/s1).
    //  - strConcatenate: "" + "abc" y "hola " + "mundo". Los argumentos deben crearse con
    //    strDuplicate, porque strConcatenate los libera (no pasar literales).
    //  - Cola: 5 push y 6 pop; el 6to debe devolver 0 y dejar queue_head y queue_tail en NULL.
    //    Verificar tambien que un push posterior funcione.
    //  - Tablero: llenar la fila de indice 19 (la de abajo), llamar a
    //    gameBoardCheckAndClearLines y comprobar que first_row es la fila nueva vacia y que
    //    la lista sigue teniendo 20 filas.
    // Liberar toda la memoria pedida en los tests (strings y tableros).
    
    printf("===== TESTS =====\n");
 
    // strDuplicate
    // 1. String vacio
    char* origen = "";
    char* dup = strDuplicate(origen);
    if (dup != NULL && dup != origen && strCompare(dup, "") == 0) {
        printf("[OK]    strDuplicate: string vacio\n");
    } else {
        printf("[FALLO] strDuplicate: string vacio\n");
    }
    free(dup);
 
    // 2. String de un caracter
    dup = strDuplicate("x");
    if (dup != NULL && strCompare(dup, "x") == 0) {
        printf("[OK]    strDuplicate: un caracter\n");
    } else {
        printf("[FALLO] strDuplicate: un caracter\n");
    }
    free(dup);
 
    // 3. String con distintos caracteres
    dup = strDuplicate("Hola, mundo 123!");
    if (dup != NULL && strCompare(dup, "Hola, mundo 123!") == 0) {
        printf("[OK]    strDuplicate: distintos caracteres\n");
    } else {
        printf("[FALLO] strDuplicate: distintos caracteres\n");
    }
    free(dup);
 
    // strCompare 
    // 1. Dos strings vacios
    if (strCompare("", "") == 0) {
        printf("[OK]    strCompare: dos vacios -> 0\n");
    } else {
        printf("[FALLO] strCompare: dos vacios -> 0\n");
    }
 
    // 2. Dos strings de un caracter
    if (strCompare("a", "a") == 0) {
        printf("[OK]    strCompare: 'a' vs 'a' -> 0\n");
    } else {
        printf("[FALLO] strCompare: 'a' vs 'a' -> 0\n");
    }
    if (strCompare("a", "b") == 1) {
        printf("[OK]    strCompare: 'a' vs 'b' -> 1\n");
    } else {
        printf("[FALLO] strCompare: 'a' vs 'b' -> 1\n");
    }
    if (strCompare("b", "a") == -1) {
        printf("[OK]    strCompare: 'b' vs 'a' -> -1\n");
    } else {
        printf("[FALLO] strCompare: 'b' vs 'a' -> -1\n");
    }
 
    // 3. Dos strings distintos, en ambos ordenes
    if (strCompare("hola", "mundo") == 1) {
        printf("[OK]    strCompare: s1 vs s2 -> 1\n");
    } else {
        printf("[FALLO] strCompare: s1 vs s2 -> 1\n");
    }
    if (strCompare("mundo", "hola") == -1) {
        printf("[OK]    strCompare: s2 vs s1 -> -1\n");
    } else {
        printf("[FALLO] strCompare: s2 vs s1 -> -1\n");
    }
 
    // un prefijo es menor
    if (strCompare("casa", "casas") == 1) {
        printf("[OK]    strCompare: 'casa' vs 'casas' -> 1\n");
    } else {
        printf("[FALLO] strCompare: 'casa' vs 'casas' -> 1\n");
    }
    if (strCompare("casas", "casa") == -1) {
        printf("[OK]    strCompare: 'casas' vs 'casa' -> -1\n");
    } else {
        printf("[FALLO] strCompare: 'casas' vs 'casa' -> -1\n");
    }
 
    // strConcatenate 
    // 1. String vacio y string de 3 caracteres 
    char* a = strDuplicate("");
    char* b = strDuplicate("abc");
    char* res = strConcatenate(a, b);   // libera a y b
    if (res != NULL && strCompare(res, "abc") == 0) {
        printf("[OK]    strConcatenate: '' + 'abc'\n");
    } else {
        printf("[FALLO] strConcatenate: '' + 'abc'\n");
    }
    free(res);
 
    // 2. Dos strings de 5 caracteres
    a = strDuplicate("hola ");
    b = strDuplicate("mundo");
    res = strConcatenate(a, b);
    if (res != NULL && strCompare(res, "hola mundo") == 0) {
        printf("[OK]    strConcatenate: 'hola ' + 'mundo'\n");
    } else {
        printf("[FALLO] strConcatenate: 'hola ' + 'mundo'\n");
    }
    free(res);
 
    //Logica de Tablero 
    int filas = 0;
    int vacia = 1;
    int valor = 0;
    RowNode* actual = NULL;
    RowNode* fila = NULL;
 
    // 1. Queue: 5 push, 6 pop (FIFO), el 6to devuelve 0, luego push y pop
    GameBoard* board = gameBoardNew();
    if (board == NULL) {
        printf("[FALLO] gameBoardNew devolvio NULL, no se puede seguir\n");
        return;
    }
    filas = 0;
    actual = board->first_row;
    while (actual != NULL) {
        filas++;
        actual = actual->next;
    }
    if (filas == GRID_ROWS) {
        printf("[OK]    gameBoardNew: tiene 20 filas\n");
    } else {
        printf("[FALLO] gameBoardNew: tiene 20 filas\n");
    }
    vacia = 1;
    for (int c = 0; c < GRID_COLS; c++) {
        if (board->first_row->cells[c] != 0) {
            vacia = 0;
        }
    }
    if (vacia == 1) {
        printf("[OK]    gameBoardNew: filas en 0\n");
    } else {
        printf("[FALLO] gameBoardNew: filas en 0\n");
    }
 
    for (int i = 0; i < 5; i++) {
        gameBoardEnqueuePiece(board, i);
    }
    int orden_ok = 1;
    for (int i = 0; i < 5; i++) {
        int extrajo = gameBoardDequeuePiece(board, &valor);
        if (extrajo != 1 || valor != i) {
            orden_ok = 0;
        }
    }
    if (orden_ok == 1) {
        printf("[OK]    Queue: 5 pops respetan el orden FIFO\n");
    } else {
        printf("[FALLO] Queue: 5 pops respetan el orden FIFO\n");
    }
 
    valor = 99;
    int sexto = gameBoardDequeuePiece(board, &valor);
    if (sexto == 0) {
        printf("[OK]    Queue: el sexto pop devuelve 0\n");
    } else {
        printf("[FALLO] Queue: el sexto pop devuelve 0\n");
    }
    if (valor == 99) {
        printf("[OK]    Queue: el sexto pop no modifica out_type\n");
    } else {
        printf("[FALLO] Queue: el sexto pop no modifica out_type\n");
    }
    if (board->queue_head == NULL && board->queue_tail == NULL) {
        printf("[OK]    Queue: head y tail en NULL\n");
    } else {
        printf("[FALLO] Queue: head y tail en NULL\n");
    }
 
    gameBoardEnqueuePiece(board, 6);
    int extrajo6 = gameBoardDequeuePiece(board, &valor);
    if (extrajo6 == 1 && valor == 6) {
        printf("[OK]    Queue: push posterior se puede extraer\n");
    } else {
        printf("[FALLO] Queue: push posterior se puede extraer\n");
    }
    gameBoardDelete(board);
 
    // 2. Llenar la fila 19 y limpiar
    board = gameBoardNew();
    if (board == NULL) {
        return;
    }
    RowNode* primera_vieja = board->first_row;
    fila = getRowAt(board, 19);
    for (int c = 0; c < GRID_COLS; c++) {
        fila->cells[c] = 1;
    }
    int borradas = gameBoardCheckAndClearLines(board);
    if (borradas == 1) {
        printf("[OK]    Tablero: fila 19 completa -> devuelve 1\n");
    } else {
        printf("[FALLO] Tablero: fila 19 completa -> devuelve 1\n");
    }
    vacia = 1;
    for (int c = 0; c < GRID_COLS; c++) {
        if (board->first_row->cells[c] != 0) {
            vacia = 0;
        }
    }
    if (board->first_row != primera_vieja && vacia == 1) {
        printf("[OK]    Tablero: la fila nueva vacia es first_row\n");
    } else {
        printf("[FALLO] Tablero: la fila nueva vacia es first_row\n");
    }
    filas = 0;
    actual = board->first_row;
    while (actual != NULL) {
        filas++;
        actual = actual->next;
    }
    if (filas == GRID_ROWS) {
        printf("[OK]    Tablero: sigue con 20 filas (fila 19)\n");
    } else {
        printf("[FALLO] Tablero: sigue con 20 filas (fila 19)\n");
    }
    if (board->first_particle != NULL) {
        printf("[OK]    Tablero: se generaron particulas\n");
    } else {
        printf("[FALLO] Tablero: se generaron particulas\n");
    }
    gameBoardDelete(board);
 
    // 3. Llenar las filas 0 y 1 (incluye la primera de la lista)
    board = gameBoardNew();
    if (board == NULL) {
        return;
    }
    fila = getRowAt(board, 0);
    for (int c = 0; c < GRID_COLS; c++) {
        fila->cells[c] = 1;
    }
    fila = getRowAt(board, 1);
    for (int c = 0; c < GRID_COLS; c++) {
        fila->cells[c] = 1;
    }
    borradas = gameBoardCheckAndClearLines(board);
    if (borradas == 2) {
        printf("[OK]    Tablero: filas 0 y 1 completas -> devuelve 2\n");
    } else {
        printf("[FALLO] Tablero: filas 0 y 1 completas -> devuelve 2\n");
    }
    filas = 0;
    actual = board->first_row;
    while (actual != NULL) {
        filas++;
        actual = actual->next;
    }
    if (filas == GRID_ROWS) {
        printf("[OK]    Tablero: sigue con 20 filas (filas 0 y 1)\n");
    } else {
        printf("[FALLO] Tablero: sigue con 20 filas (filas 0 y 1)\n");
    }
    vacia = 1;
    for (int c = 0; c < GRID_COLS; c++) {
        if (board->first_row->cells[c] != 0) {
            vacia = 0;
        }
    }
    if (vacia == 1) {
        printf("[OK]    Tablero: first_row esta vacia\n");
    } else {
        printf("[FALLO] Tablero: first_row esta vacia\n");
    }
    gameBoardDelete(board);
 
    printf("===== FIN DE LOS TESTS =====\n");
}

// Compara contra "--test" sin depender de string.h ni del Ejercicio 1.
int esFlagTest(char* arg) {
    char* flag = "--test";
    int i = 0;
    while (arg[i] != '\0' && arg[i] == flag[i]) i++;
    return arg[i] == '\0' && flag[i] == '\0';
}

int main(int argc, char* args[]) {
    srand(time(NULL));

    // Los tests corren antes de SDL. Con --test termina aca, asi valgrind solo
    // reporta la memoria del TP (SDL tiene bloques propios que no se liberan).
    correrTests();
    if (argc > 1 && esFlagTest(args[1])) return 0;

    if (!inicializar()) return 1;

    game_board = gameBoardNew();
    if (game_board == NULL) {
        printf("ADVERTENCIA: Implementa gameBoardNew() para iniciar el juego.\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    }

    for(int i=0; i<3; i++) gameBoardEnqueuePiece(game_board, rand() % 7);

    SDL_Event e;
    int game_over = 0;

    while (!game_over) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) game_over = 1;
            if (e.type == SDL_KEYDOWN && game_board->current_piece.active) {
                if (e.key.keysym.sym == SDLK_LEFT && canMove(game_board, game_board->current_piece.col - 1, game_board->current_piece.row, game_board->current_piece.rotation)) game_board->current_piece.col--;
                if (e.key.keysym.sym == SDLK_RIGHT && canMove(game_board, game_board->current_piece.col + 1, game_board->current_piece.row, game_board->current_piece.rotation)) game_board->current_piece.col++;
                if (e.key.keysym.sym == SDLK_DOWN && canMove(game_board, game_board->current_piece.col, game_board->current_piece.row + 1, game_board->current_piece.rotation)) game_board->current_piece.row++;
                if (e.key.keysym.sym == SDLK_UP) {
                    int next_rot = (game_board->current_piece.rotation + 1) % 4;
                    if (canMove(game_board, game_board->current_piece.col, game_board->current_piece.row, next_rot)) game_board->current_piece.rotation = next_rot;
                }
                if (e.key.keysym.sym == SDLK_SPACE) {
                     while(canMove(game_board, game_board->current_piece.col, game_board->current_piece.row + 1, game_board->current_piece.rotation)) game_board->current_piece.row++;
                     lockPiece(game_board);
                }
            }
        }
        if (!gameBoardUpdate(game_board)) game_over = 1;
        gameBoardDraw(game_board);
        SDL_Delay(16);
    }
    gameBoardDelete(game_board);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
