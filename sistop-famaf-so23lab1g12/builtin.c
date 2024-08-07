#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <unistd.h>
#include "tests/syscall_mock.h"

#include "builtin.h"
#include "command.h"
#include "execute.h"

#include "tests/syscall_mock.h"

/*Variable global para terminar ejecucion*/
bool quit_mybash;

/*COMANDOS NECESARIOS
 *cd: Se implementa de manera directa con la syscall chdir()
 *help: Debe mostrar un mensaje por la salida estandar indicando el nombre
 *   del shell, el nombre de sus autores y listar los comandos internos 
 *   implementados con una breve descripción de lo que hace cada uno.
 *exit: Es conceptualmente el más sencillo pero requiere un poco de planificación para que el shell termine de manera limpia
 */


//CHEQUEO
/*Funciones que se encargan de correr comprobar si los comandos existen*/

/*COMMAND CD*/
static bool builtin_is_cd(scommand cmd){
    assert(cmd!=NULL);
    return strcmp(scommand_front(cmd), "cd") == 0;
}

static void builtin_run_cd(scommand cmd){
    assert(cmd!=NULL && builtin_is_cd(cmd));
    scommand_pop_front(cmd);
    int i =chdir(scommand_front(cmd));
    if (i!=0){
        perror("cd: e   rror: ");
    }
}
/*
    int chdir(const char *path); (para cambiar directorio)
    Parameter: Here, the path is the Directory path that the user want to make the current working directory.
    Return Value: This command returns zero (0) on success. -1 is returned on an error and errno is set appropriately. 
    Note: It is declared in unistd.h. 
*/

/*COMMAND HELP*/
static bool builtin_is_help(scommand cmd){
    assert(cmd!=NULL);
    return strcmp(scommand_front(cmd), "help") == 0;
}

static void builtin_run_help(scommand cmd){
    assert(cmd!=NULL && builtin_is_help(cmd));
    printf("KKB (Kernel Band Shell) v1.0\n\n");
	printf("Grupo 12:\n\tLautaro Peralta\n\tJoaquin Dubois\n\tLuca Oliva\n\tMateo Ricci Villarruel\n\n");
	printf("Commandos internos:\n\t_help\n\t\tMuestra la version del programa, los integrantes de grupo y los comandos internos\n\n\t_cd\n\t\tCambia directorio\n\n\t_exit\n\t\tCierra el programa\n\n");
}

/*COMMAND EXIT*/
static bool builtin_is_exit(const scommand cmd) {
    assert(cmd != NULL);
    return strcmp(scommand_front(cmd), "exit") == 0;
}

static void builtin_run_exit(scommand cmd){
    assert(cmd!=NULL && builtin_is_exit(cmd));
    quit_mybash=true;
}

//Funciones de "llamado general"

bool builtin_is_internal(scommand cmd){
    assert(cmd!=NULL);

    return (builtin_is_cd(cmd)||builtin_is_exit(cmd)||builtin_is_help(cmd));
}
/*
 * Indica si el comando alojado en `cmd` es un comando interno
 *
 * REQUIRES: cmd != NULL
 *
 */

bool builtin_alone(pipeline p){
    assert(p!=NULL);
    return(pipeline_length(p) == 1 && builtin_is_internal(pipeline_front(p)));
}
/*
 * Indica si el pipeline tiene solo un elemento y si este se corresponde a un
 * comando interno.
 *
 * REQUIRES: p != NULL
 *
 * ENSURES:
 *
 * builtin_alone(p) == pipeline_length(p) == 1 &&
 *                     builtin_is_internal(pipeline_front(p))
 *
 *
 */

void builtin_run(scommand cmd){
    assert(cmd!=NULL && builtin_is_internal(cmd));
    if(builtin_is_cd(cmd)){

        builtin_run_cd(cmd);

    }
    else if(builtin_is_help(cmd)){

        builtin_run_help(cmd);

    }
    else {

        builtin_run_exit(cmd);

    }
}
/*
 * Ejecuta un comando interno
 *
 * REQUIRES: {builtin_is_internal(cmd)}
 *
 */


