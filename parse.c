/**
 * @file parse.c
 * @brief parse() can be called to divide the user command line 
 *        into useful pieces.
 */

#include <stdio.h>
#include <stdarg.h>

#include "parse.h"
#include "taskman.h"
#include "util.h"

/* Helper Functions */
static void parse_n(const char *cmd_line, Instruction *inst, char *argv[], size_t n);
static int initialize_argv_n(char *argv[], size_t n);
static int contains(const char *needle, char *haystack[]);
static int parse_id_token(const char *p_tok, const char *instruct, int *id); 
static int parse_file_token(const char *p_tok, const char *instruct, char **file); 
static void dprintf(const char* fmt, ...); 

/* Reference Data */

// full recognized instruction list
static char *instructs_list_full[] = {"help", "quit", "tasks", "delete", "run", "bg", "cancel", "log", "output", "suspend", "resume", NULL};

// instructions which may use an ID argument
static char *instructs_with_id[] = {"delete", "run", "bg", "cancel", "log", "output", "suspend", "resume", NULL};

// instructions which may use a filename argument
static char *instructs_with_file[] = {"run", "bg", "log", NULL};

/*********
 * Command Parsing Functions
 *********/

void parse(const char *cmd_line, Instruction *inst, char *argv[]) {
    initialize_argv_n(argv, MAXARGS);
    parse_n(cmd_line, inst, argv, MAXARGS-1);
}

static void parse_n(const char *cmd_line, Instruction *inst, char *argv[], size_t n) {
  /* Step 0: ensure a valid input, and quit gracefully if there isn't one. */
    if (!cmd_line || !inst || !argv) return;

  /* Step 0b: ensure initialized data */
    inst->instruct = NULL;
    inst->id = 0;
    inst->file = NULL;

  /* Step 1: Only work on a copy of the original command */ 
    char *p_tok = NULL;
    char buffer[MAXLINE] = {0};
    strncpy(buffer, cmd_line, MAXLINE);

  /* Step 2: Tokenize the inputs (space delim) and parse */
    p_tok = strtok(buffer, " ");
    if (p_tok == NULL) { return; }

    int index = 0;

    while(p_tok != NULL) {
        argv[index] = string_copy(p_tok);
	index++;
        p_tok = strtok(NULL, " ");
	if (index >= n) p_tok = NULL;
    }

    /* Step 2a: Parse the instruction */
    inst->instruct = string_copy(argv[0]);

    /* Step 2b: Parse the Task ID */
    parse_id_token(argv[1], inst->instruct, &inst->id );

    /* Step 2c: Parse the file name */
    parse_file_token(argv[2], inst->instruct, &inst->file);

    /* Step 3: if the instruction is a built-in, clear argv */
    if (contains(inst->instruct, instructs_list_full)) {
        free_argv_str(argv);
    }
}

/**
 * @brief Parse the buffer ID from the current token.  
 *        If the input is a valid number token, 
 *        and it corresponds to an appropriate instruction, 
 *        then return true, else return false.  
 *        The id argument is populated with the ID taken from p_tok.
 */
static int parse_id_token(const char *p_tok, const char *instruct, int *id) {
    if (!p_tok || !instruct || !id) { return 0; }

    if (!contains(instruct, instructs_with_id)) { return 0; }

    char *end = NULL;
    *id = (int) strtol(p_tok, &end, 10);

    // check if we successfully read a valid ID number
    if (!end || (*end) || end == p_tok) {
        *id = 0;
	return 0;
    }

    return 1;
}

/**
 * @brief Parse the file name from the current token.  
 *        If the input is a valid file token, and it corresponds to 
 *        an appropriate instruction, then return true, else return false.
 *        The file argument is populated with the file name taken from p_tok.
 */
static int parse_file_token(const char *p_tok, const char *instruct, char **file) {
    if (!p_tok || !instruct || !file) { return 0; }

    if (!contains(instruct, instructs_with_file)) { return 0; }

    *file = string_copy(p_tok);

    return 1;
}

/* Returns true of false depending on whether the haystack contains the needle. */
static int contains(const char *needle, char *haystack[]) {
    if (!needle || !haystack) return 0;

    for (int i = 0;  haystack[i];  i++) {
        if (strncmp(needle, haystack[i], strlen(haystack[i]) + 1) == 0) { return 1; }
    }
    return 0;
}

/*********
 * String Processing Helpers
 *********/

/* Returns 1 if string is all whitespace, else 0 */
int is_whitespace(const char *str) {
    if (!str) return 1;
    while (isspace(*str)) { str++; }  // skip past every whitespace character
    return *str == '\0';  // decide based on whether the first non-whitespace is the end-of-string
}

/*********
 * Initialization Functions
 *********/

int initialize_instruction(Instruction *inst) {
    if (!inst) return 0;

    inst->instruct = NULL;
    inst->id = 0;
    inst->file = NULL;

    return 1;
}

int initialize_argv(char *argv[]) {
    return initialize_argv_n(argv, MAXARGS);
}

static int initialize_argv_n(char *argv[], size_t n) {
    if (!argv) return 0;

    for(int i = 0; i < n; i++) {
        argv[i] = NULL;
    }
    return 1;
}

int initialize_command(Instruction *inst, char *argv[]) {
    if (! initialize_instruction(inst)) return 0;
    if (! initialize_argv(argv)) return 0;
    return 1;
}


void free_instruction(Instruction *inst) {

    if (inst) {
        free(inst->instruct);
	inst->instruct = NULL;
	free(inst->file);
	inst->file = NULL;
    }

}

void free_command(Instruction *inst, char *argv[]) {
    free_instruction(inst);
    free_argv_str(argv);
}

/*********
 * Debug Functions
 *********/
static void dprintf(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	char buf[256] = {0};
	snprintf(buf, 256, "\033[1;33m[DEBUG] %s\033[0m", fmt);
	vfprintf(stderr, buf, args);
	va_end(args);
}

void debug_print_parse(char *cmdline, Instruction *inst, char *argv[], char *loc) {
    int i = 0;
    fprintf(stderr, "\n");
    dprintf("-----------------------\n");
    if (loc) { dprintf("- %s\n", loc); }
    if (loc) { dprintf("-----------------------\n"); }

    if(cmdline) { dprintf("cmdline     = %s\n", cmdline); }
  
    if(inst) {
        dprintf("instruction = %s\n", inst->instruct);
        if (inst->id) { dprintf("buffer ID  = %d\n", inst->id); }
        else { dprintf("buffer ID   = (default)\n"); }
        if (inst->file) { dprintf("file        = %s\n", inst->file); }
    }

    if(argv) {
        for(i = 0; argv[i]; i++) {
            dprintf("argv[%d] == %s\n", i, argv[i]);
	}
    }
  
    dprintf("-----------------------\n");
}
