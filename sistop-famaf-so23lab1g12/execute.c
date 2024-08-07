#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "builtin.h"
#include "command.h"
#include "execute.h"

#include "tests/syscall_mock.h"

#define READ_END 0 // extremo escritura
#define WRITE_END 1 // extremo lectura

bool after_background = false;

static void set_input_file(scommand scm){
    assert(scm!=NULL);
    char *input = scommand_get_redir_in(scm);
    if (input != NULL) {
        int file_input = open(input, O_RDONLY,0);
        if (file_input==-1){
            perror("Open input file");
            exit(EXIT_FAILURE);
        }
        if (dup2(file_input, 0)==-1){
            perror("Dup2 Input file");
        }
        if (close(file_input)==-1){
            perror("Close input file");
        }
    }
}

static void set_output_file(scommand scm){
    assert(scm!=NULL);
    char *output = scommand_get_redir_out(scm);
    if(output!=NULL){
        int file_output = open(output, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        if (file_output==-1){
            perror("Open ouput file");
            exit(EXIT_FAILURE);
        }
        if (dup2(file_output, 1)==-1){
            perror("Dup2 output file");
        }
        if (close(file_output)==-1){
            perror("Close ouput file");
        }
    }
}

static void execute_external_command(scommand scmd){
    set_input_file(scmd);
    set_output_file(scmd);
    char* comando = scommand_front(scmd);
    char** argv= scommand_to_argv(scmd);
    int error_return = execvp(comando, argv);
    if(error_return<0){
        perror(argv[0]);
        exit(EXIT_FAILURE);
    }
    //printf("\n");
    free(argv);
}

static void exec_single_cmd(pipeline apipe, int pid){
    if(pid < 0){
        //error en el fork
        perror("fork");
    }
    else if(pid == 0){
        //proceso hijo
        execute_external_command(pipeline_front(apipe));
    }
    else{
        waitpid(pid,NULL,0);
    }
}


static void exec_multiple_cmd(pipeline apipe){
    /*
    unsigned int number_of_pipes = pipeline_length(apipe) - 1u;
    for(unsigned int i = 0; i < number_of_pipes-1 ; i++){
    }
    */ 
    int fd1[2];
    int pid;
    
    if (pipe(fd1)<0){/* comunica ls y wc */
        perror("pipe");
    } 
    
    pid = fork(); 
    if(pid < 0){
        //error en el fork
        perror("fork");
    }

    if(pid == 0){              
        close(fd1[READ_END]);   /* cerrar extremo no necesario */
        
        if(dup2(fd1[WRITE_END], STDOUT_FILENO)==-1){
            perror("Dup2");
        } 
        if(close(fd1[WRITE_END])==-1){
            perror("File descriptor");
        }
        
        execute_external_command(pipeline_front(apipe));
        pipeline_pop_front(apipe);
    }
    else if(pid > 0)                     /* padre */
    { 
        close(fd1[WRITE_END]);    /* extremo no necesario ya */
        
        pid = fork();
        if(pid == 0){               /* hijo 2, ejecuta "wc" */
            if (dup2(fd1[READ_END], STDIN_FILENO)==-1){
                perror("Dup2");
            } 
            if (close(fd1[READ_END])==-1){
                perror("File descriptor");
            }  
            
            pipeline_pop_front(apipe);
            execute_external_command(pipeline_front(apipe));
        }
        else if(pid > 0) /* padre */
        {
            close(fd1[READ_END]);      /* cerrar extremo no necesario */    
        }                
    }
   /* wait para cada hijo */
    wait(NULL);
    wait(NULL);
}


static void execute_foreground(pipeline apipe){
    if(pipeline_length(apipe) == 1){
        if(builtin_is_internal(pipeline_front(apipe))){
            int stdout_copy = dup(1);
            set_output_file(pipeline_front(apipe));
            builtin_run(pipeline_front(apipe));
            dup2(stdout_copy, 1);
            close(stdout_copy);
        }
        else{
            int pid = fork();
            exec_single_cmd(apipe, pid);
        }
    }
    else if(pipeline_length(apipe) > 1){
        exec_multiple_cmd(apipe);
    }
}


void execute_pipeline(pipeline apipe){
    if(pipeline_get_wait(apipe)){
            execute_foreground(apipe);
    }else{
        int id = fork();
        if (id < 0){
            //error en el fork
            perror("fork");
        }
        if(id==0){
            int fd_pipe[2];
            if(pipe(fd_pipe)<0){
                perror("pipe");
            } 
            close(fd_pipe[1]);
            int res_dup2 = dup2(fd_pipe[0], STDIN_FILENO);
            if (res_dup2 < 0){
                perror("Dup2");
                exit(EXIT_FAILURE);
            } 
            execute_foreground(apipe);
            exit(EXIT_SUCCESS);
        }

        after_background = true;
    }
    //sleep(1);
}
/*
 * Ejecuta un pipeline, identificando comandos internos, forkeando, y
 *   redirigiendo la entrada y salida. puede modificar `apipe' en el proceso
 *   de ejecución.
 *   apipe: pipeline a ejecutar
 * Requires: apipe!=NULL
 */

