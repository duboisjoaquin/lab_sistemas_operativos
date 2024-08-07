/* A partir de man bash, en su sección de SHELL GRAMMAR,
 * se diseñaron dos TAD scommand (comando simple) y
 * pipeline (secuencia de comandos simples separados por
 * pipe).
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> /* para tener bool */
#include <glib.h> // `pkg-config --cflags --libs glib-2.0`
#include <assert.h>

#include "strextra.h"
#include "command.h"

#include "tests/syscall_mock.h"



/* scommand: comando simple.
 * Ejemplo: ls -l ej1.c > out < in
 * Se presenta como una secuencia de cadenas donde la primera se denomina
 * comando y desde la segunda se denominan argumentos.
 * Almacena dos cadenas que representan los redirectores de entrada y salida.
 * Cualquiera de ellos puede estar NULL indicando que no hay redirección.
 *
 * En general, todas las operaciones hacen que el TAD adquiera propiedad de
 * los argumentos que le pasan. Es decir, el llamador queda desligado de la
 * memoria utilizada, y el TAD se encarga de liberarla.
 *
 * Externamente se presenta como una secuencia de strings donde:
 *           _________________________________
 *  front -> | cmd | arg1 | arg2 | ... | argn | <-back
 *           ---------------------------------
 * 
 * #arranca en el cmd el GSlist y sigue hasta el arg numero n el next es null
 *
 * La interfaz es esencialmente la de una cola. A eso se le
 * agrega dos accesores/modificadores para redirección de entrada y salida.
 */

/*
    #GSlist: listas enlazadas
    #Glist: pointer a data, prev Glist, next Glist
*/

// ([char*],char*,char*)
struct scommand_s{
    GSList* cmd_list;
    char* input_redirection;
    char* output_redirection;
};

//Nuevo `scommand', sin comandos o argumentos y los redirectores vacíos
scommand scommand_new(void){
    scommand new_s = malloc(sizeof(struct scommand_s));
    if(new_s == NULL){ //Error in the malloc
        printf("Error in malloc()");
        exit(EXIT_FAILURE);
    }

    new_s->cmd_list = NULL;
    new_s->input_redirection = NULL;
    new_s->output_redirection = NULL;

    assert(new_s != NULL && scommand_is_empty(new_s) && scommand_get_redir_in (new_s) == NULL && scommand_get_redir_out(new_s) == NULL); //ensures: result != NULL && scommand_is_empty (result) && scommand_get_redir_in (result) == NULL && scommand_get_redir_out (result) == NULL
    return new_s;
}

// Destruye el comando simple.
scommand scommand_destroy(scommand self){
    assert(self != NULL); // Requires: self != NULL

    g_slist_free_full(self->cmd_list, free); // function in GSlist for full destroy
    self->cmd_list = NULL; // null asignation

    free(self->input_redirection); // free in
    self->input_redirection = NULL; // null asignation

    free(self->output_redirection); // free out
    self->output_redirection = NULL; // null asignation

    free(self); // free struct
    self = NULL; // null asignation

    assert(self == NULL); // Ensures: result == NULL
    return self;
}

/* Modificadores */

// Agrega por detrás una cadena a la secuencia de cadenas.
void scommand_push_back(scommand self, char * argument){

    assert(self!=NULL && argument!=NULL); // Requires: self!=NULL && argument!=NULL

    self->cmd_list =  g_slist_append(self->cmd_list, argument); // append argument using GSlib fun

    assert(!scommand_is_empty(self)); // Ensures: !scommand_is_empty()
}

// Quita la cadena de adelante de la secuencia de cadenas.
void scommand_pop_front(scommand self){
    assert(self!=NULL && !scommand_is_empty(self)); // Requires: self!=NULL && !scommand_is_empty(self)

    free(self->cmd_list->data);
    self->cmd_list = g_slist_remove(self->cmd_list, self->cmd_list->data); // in theory it return The new start of the GSList
}

// Define la redirección de entrada.
void scommand_set_redir_in(scommand self, char * filename){
    assert(self != NULL); // Requires: self!=NULL
    free(self->input_redirection);
    self->input_redirection = NULL;
    self->input_redirection = filename; // simple asignation
}

// Define la redirección de salida.
void scommand_set_redir_out(scommand self, char * filename){
    assert(self != NULL); // Requires: self!=NULL
    free(self->output_redirection);
    self->output_redirection = NULL;    
    self->output_redirection = filename; // simple asignation
}

/* Proyectores */

// Indica si la secuencia de cadenas tiene longitud 0.
bool scommand_is_empty(const scommand self){
    assert(self!=NULL); // Requires: self!=NULL

    bool res = self->cmd_list == NULL; // checking if the scmd is empty
    return res;
}

// Da la longitud de la secuencia cadenas que contiene el comando simple.
unsigned int scommand_length(const scommand self){
    assert(self!=NULL); // requieres

    unsigned int res = g_slist_length(self->cmd_list); // Checking the length of de scmd list using GSlib fun

    assert((res == 0) == scommand_is_empty(self)); // // Ensures: (scommand_length(self)==0) == scommand_is_empty()
    return res;
}

// Toma la cadena de adelante de la secuencia de cadenas.
char * scommand_front(const scommand self){
    assert(self != NULL && !scommand_is_empty(self)); // Requires: self!=NULL && !scommand_is_empty(self)

    char *res = self->cmd_list->data;

    assert(res != NULL); // Ensures: result!=NULL
    return res;
}

// Obtiene los nombres de archivos a donde redirigir la entrada.
char * scommand_get_redir_in(const scommand self){
    assert(self != NULL); // Requires: self!=NULL

    return (self->input_redirection);
}

// Obtiene los nombres de archivos a donde redirigir la salida.
char * scommand_get_redir_out(const scommand self){
    assert(self != NULL); // Requires: self!=NULL

    return (self->output_redirection);
}

// Genera una representación del comando simple en un string (aka "serializar")
char * scommand_to_string(const scommand self){
    assert(self!=NULL); // Requieres: self!=NULL

    char *res = strdup("");
    
    GSList *l = self->cmd_list; // hace una copia daba mem-leaks

    if(l != NULL){
        res = str_merge_new(res, l->data);
        l = g_slist_next(l);
        while(l != NULL){
            res = str_merge_new(res, " ");
            res = str_merge_new(res, l->data); 
            l = g_slist_next(l);
        }
    }

    if(self->input_redirection != NULL){
        res = str_merge_new(res, " < ");
        res = str_merge_new(res, scommand_get_redir_in(self));
    }
    if(self->output_redirection != NULL){
        res = str_merge_new(res, " > ");
        res = str_merge_new(res, scommand_get_redir_out(self));
    }

    assert(scommand_is_empty(self) || scommand_get_redir_in(self)==NULL || scommand_get_redir_out(self)==NULL || strlen(res)>0); // Ensures: scommand_is_empty(self) || scommand_get_redir_in(self)==NULL || scommand_get_redir_out(self)==NULL || strlen(result)>0
    return res;
}

// funcion para evitar errores en el execvp
static char* scommand_to_argv_aux(scommand self) {
    assert(self != NULL && !scommand_is_empty(self));

    char* result = self->cmd_list->data;
    self->cmd_list = g_slist_remove(self->cmd_list, result);

    assert(result != NULL);
    return (result);
}

char** scommand_to_argv(scommand self){
    unsigned int length = scommand_length(self);
    char** argv = calloc(sizeof(char*),length+1);
    if(argv != NULL){
        for(unsigned int i = 0; i<length; i++){
            argv[i] = scommand_to_argv_aux(self);
        }
        argv[length] = NULL;
    }
    return argv;
}

/*
 * pipeline: tubería de comandos.
 * Ejemplo: ls -l *.c > out < in  |  wc  |  grep -i glibc  &
 * Secuencia de comandos simples que se ejecutarán en un pipeline,
 *  más un booleano que indica si hay que esperar o continuar.
 *
 * Una vez que un comando entra en el pipeline, la memoria pasa a ser propiedad
 * del TAD. El llamador no debe intentar liberar la memoria de los comandos que
 * insertó, ni de los comandos devueltos por pipeline_front().
 * pipeline_to_string() [pide memoria internamente y debe ser liberada
 * externamente]. [q es eso]
 *
 * Externamente se presenta como una secuencia de comandos simples donde:
 *           ______________________________
 *  front -> | scmd1 | scmd2 | ... | scmdn | <-back
 *           ------------------------------
 */

// lsitas enlazadas que el data sea un sscomand

// ([scommand], bool)
struct pipeline_s{
    GSList* smcd;
    bool wait;
};

// Nuevo `pipeline', sin comandos simples y establecido para que espere.
pipeline pipeline_new(void){
    pipeline new_p = malloc(sizeof(struct pipeline_s));

    if(new_p == NULL){
        printf("Error in malloc()");
        exit(EXIT_FAILURE);
    }

    new_p->smcd = NULL;
    new_p->wait = true;

    assert(new_p != NULL && pipeline_is_empty(new_p) && pipeline_get_wait(new_p)); // Ensures: result != NULL && pipeline_is_empty(result) && pipeline_get_wait(result)
    return new_p;
}

// Destruye el pipeline.
pipeline pipeline_destroy(pipeline self){
    assert(self!=NULL); // Requires: self != NULL

    while(!pipeline_is_empty(self)){
        self->smcd->data = scommand_destroy(self->smcd->data);
        self->smcd = self->smcd->next;
    }

    g_slist_free_full(self->smcd, free); // function in GSlist for full destroy
    self->smcd = NULL; // null asignation

    free(self); // free struct
    self = NULL; // null asignation

    assert(self==NULL); // Ensures: result == NULL
    return self;
}

// Copia el pipeline


/* Modificadores */

// Agrega por detrás un comando simple a la secuencia.
void pipeline_push_back(pipeline self, scommand sc){
    assert(self != NULL && sc != NULL); // Requires: self!=NULL && sc!=NULL

    self->smcd = g_slist_append(self->smcd, sc);

    assert(!pipeline_is_empty(self)); //Ensures: !pipeline_is_empty(self)
}

// Quita el comando simple de adelante de la secuencia.
void pipeline_pop_front(pipeline self){ // gconstpointer que es: typedef const void *gconstpointer;
    assert(self != NULL && !pipeline_is_empty(self)); // Requires: self!=NULL && !pipeline_is_empty(self)

    scommand_destroy(self->smcd->data);
    self->smcd = g_slist_remove(self->smcd, self->smcd->data);
}

// Define si el pipeline tiene que esperar o no.
void pipeline_set_wait(pipeline self, const bool w){
    assert(self!=NULL); // Requires: self!=NULL

    self->wait = w;
}

/* Proyectores */

// Indica si la secuencia de comandos simples tiene longitud 0.
bool pipeline_is_empty(const pipeline self){
    assert(self!= NULL); // Requires: self!=NULL

    bool res = self->smcd == NULL;

    return res;
    //return self->smcd == NULL;
}


unsigned int pipeline_length(const pipeline self){
    assert(self != NULL); // Requires: self!=NULL

    unsigned int res = g_slist_length(self->smcd);

    assert((res == 0) == pipeline_is_empty(self)); // Ensures: (pipeline_length(self)==0) == pipeline_is_empty()
    return res;
}

// Devuelve el comando simple de adelante de la secuencia.
scommand pipeline_front(const pipeline self){
    assert(self!=NULL && !pipeline_is_empty(self)); // Requires: self!=NULL && !pipeline_is_empty(self)

    //scommand res = g_slist_nth_data(self->smcd, 0u);
    scommand res = self->smcd->data;

    assert(res != NULL); // Ensures: result!=NULL
    return res;
}

// Consulta si el pipeline tiene que esperar o no.
bool pipeline_get_wait(const pipeline self){
    assert(self!=NULL); // Requires: self!=NULL

    bool res = self->wait;

    return res;
}

// funcion para agilizar la implementacion del pipeline to string evitando leaks
static char* pipeline_to_string_aux(char* chars, const scommand self) {
    assert(chars != NULL && self != NULL);

    char* myChar = scommand_to_string(self);
    chars = str_merge_new(chars, myChar);
    free(myChar);
    myChar = NULL;

    return (chars);
}

// Genera una representación del pipeline en una cadena (aka "serializar")
char * pipeline_to_string(const pipeline self){
    assert(self != NULL); // Requires: self!=NULL

    char * res = strdup("");
    GSList *cmd = self->smcd;

    if (cmd != NULL) { // chequeamos si la lista es vacia
        res = pipeline_to_string_aux(res, g_slist_nth_data(cmd, 0u));
        cmd = g_slist_next(cmd);

        while (cmd != NULL) {
            res = str_merge_new(res, " | ");
            res = pipeline_to_string_aux(res,g_slist_nth_data(cmd, 0u));
            cmd = g_slist_next(cmd);
        }

        if (!pipeline_get_wait(self)) {
            res = str_merge_new(res, " &");
        }
    }

    assert(pipeline_is_empty(self) || pipeline_get_wait(self) || strlen(res)>0); //Ensures: pipeline_is_empty(self) || pipeline_get_wait(self) || strlen(result)>0
    return res;
}