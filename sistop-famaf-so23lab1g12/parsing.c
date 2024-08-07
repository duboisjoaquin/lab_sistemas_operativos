#include <stdlib.h>
#include <stdbool.h>

#include "parsing.h"
#include "parser.h"
#include "command.h"

bool syntax_error = false;

static bool check_end(Parser p){
    bool res;
    if(!parser_at_eof(p)){
        res = false;
    }
    else{
        res = true;
    }
    return res;
}

static scommand parse_scommand(Parser p){
    scommand cmd = NULL;
    char *arg = NULL;
    arg_kind_t arg_type;
    bool cmd_end = false;

    arg = parser_next_argument(p, &arg_type);
    if(arg == NULL){
        return NULL;
    }
    cmd = scommand_new();

    while(!cmd_end){
        if (arg==NULL && (arg_type==ARG_INPUT||arg_type==ARG_OUTPUT)){
            cmd = scommand_destroy(cmd);
            cmd = NULL;
            cmd_end = true;
            printf("Syntax Error\n");
        }else{
        if(arg == NULL){
            cmd_end = true;
        }
        else{
            if(arg_type == ARG_NORMAL){
                scommand_push_back(cmd, arg);
            }
            else if(arg_type == ARG_INPUT){
                scommand_set_redir_in(cmd, arg);
            }
            else if(arg_type == ARG_OUTPUT){
                scommand_set_redir_out(cmd, arg);
            }
            else{
                return NULL;
            }
        }
        }

        if(!cmd_end){
            arg = parser_next_argument(p, &arg_type);
        }
    }
    return cmd;
}

/*Traduce Parser a pipeline*/
pipeline parse_pipeline(Parser p) {
assert(p != NULL && !parser_at_eof(p)); // Requieres

    pipeline result = NULL;
    scommand cmd = NULL;
    bool error = false, another_pipe=true;
    bool garbage = false;
    bool was_op_background = false;

    cmd = parse_scommand(p);
    error = (cmd==NULL); /* Comando inválido al empezar */

    if(!error){
        result = pipeline_new();
        pipeline_push_back(result,cmd); /*Agrega en result el parser traducido a la scomand*/
        parser_op_pipe(p,&another_pipe); 
    }

    while(another_pipe && !error){
        cmd = parse_scommand(p);
        error = (cmd==NULL);
        if(error){
            pipeline_destroy(result);
            result = NULL;
            printf("Invalid Command\n");
        }else{
            pipeline_push_back(result,cmd);

            if (check_end(p) == true){      /*Chequeo no estar sobre el final*/
                break;
            }
            else{
                parser_op_pipe(p,&another_pipe);
            }
        }

        if (check_end(p) == true){      /*Chequeo no estar sobre el final*/
                break;
            }
            else{
                parser_op_pipe(p,&another_pipe);
            }
        }

    
    parser_op_background(p, &was_op_background);
    if(was_op_background){
        pipeline_set_wait(result, false);
    }
    another_pipe = false;
    parser_op_pipe(p,&another_pipe);
    if(another_pipe){
        printf("Syntax Error\n");
        syntax_error = true;
        result = NULL;
    }

    parser_garbage(p,&garbage);

    
    return result;    
}
/* ENSURES:
 *     No se consumió más entrada de la necesaria
 *     El parser esta detenido justo luego de un \n o en el fin de archivo.
 *     Si lo que se consumió es un pipeline valido, el resultado contiene la
 *     estructura correspondiente.
 */