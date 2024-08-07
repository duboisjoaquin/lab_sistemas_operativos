#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include "command.h"
#include "execute.h"
#include "parser.h"
#include "parsing.h"
#include "builtin.h"
#include "limits.h"

/*Muestra PROMPT del bash*/
static void show_prompt(void) {
    printf ("\033[0;32m");
    printf ("KBB@%s:",getlogin());
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf ("\033[0;36m");
        printf("%s> ", cwd);
    } 
    printf("\033[0m");
    fflush (stdout);
}

/*Ejecucion del programa*/
int main(int argc, char *argv[]) {
    pipeline pipe;
    Parser input;
    quit_mybash = false;

    input = parser_new(stdin);
    while (!quit_mybash) {
        show_prompt();
        after_background = false;
        syntax_error = false;
        pipe = parse_pipeline(input);
        quit_mybash = parser_at_eof(input);
        if(parser_at_eof(input)&&!after_background){
            printf("\n");
        }
        if(pipe!=NULL && pipeline_length(pipe)!=0 && !syntax_error){
            execute_pipeline(pipe);
            pipe = pipeline_destroy(pipe);
        }
    }
    input = parser_destroy(input); 
    input = NULL;
    return EXIT_SUCCESS;
}
