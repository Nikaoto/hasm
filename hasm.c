/*
 hasm - hack (virtual computer) assembler

 Usage: hasm infile [-o outfile]
 Assembles `infile` and creates an ASCII-encoded hack binary `infile.hack`.

 Options:
     -o outfile      specify output file
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "file.h"

#define MIN_ARGC                           2
#define MAX_ARGC                           4
#define ERR_TEXT_SIZE                      200
#define FILE_PATH_SIZE                     200
#define SYMBOL_TABLE_INITIAL_SIZE          100
#define MAX_COMP_TOKEN_COUNT               3
#define LOG_PARSER_OUTPUT                  0
#define LOG_GENERATOR_OUTPUT               0
#define INST_ARRAY_STARTING_CAPACITY       1024
#define INST_ARRAY_CAPACITY_GROWTH_RATE    1024
#define SYMBOL_PAIRS_SIZE                  4096

// Blankspace is ' ' or '\t'
inline int is_blank(char c)
{
    return c == ' ' || c == '\t';
}

// Whitespace is blankspace or '\n' or '\r'
inline int is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

inline int is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

inline int is_upper_alpha(char c)
{
    return c >= 'A' && c <= 'Z';
}

inline int is_number(char c)
{
    return c >= '0' && c <= '9';
}

// Checks if char is allowed to be the first char of a symbol
int is_valid_symbol_head(char c)
{
    return is_alpha(c) || c == '_' || c == '.' || c == '$' || c  == ':';
}

// Checks if char is allowed in a symbol as a non-first char
int is_valid_symbol_tail(char c)
{
    return is_valid_symbol_head(c) || is_number(c);
}

// Returns 1 if 's' and 't' are the same (case insensitive)
// Returns 0 otherwise
int are_strings_equal_ignore_case(char *s, char *t)
{
    for (; *s != '\0' || *t != '\0'; s++, t++) {
        if (tolower(*s) != tolower(*t)) {
            return 0;
        }
    }

    return 1;
}

// Reverse str in place
void reverse_str(char *str, size_t len)
{
    char temp;
    char *last = str + len - 1;
    // Swap equidistant characters from the middle
    while (last - str > 0) {
        temp = *str;
        *str++ = *last;
        *last-- = temp;
    }
}

// Return index of first occurrence of t in s
// Return -1 if t not found in s
int strindex(char *s, char *t)
{
    char *s_start = s;
    char *t_start = t;
    while (*s) {
        while (*s++ == *t++) {
            if (*t == 0) // Return index
                return s - s_start - (t - t_start);
        }
        t = t_start; // Reset t
    }
    return -1;
}

// Return index of first occurence of 't' in 's'
// Checks only 'n' number of chars
// Return -1 if not found
int strnindex(char *s, char *t, size_t n)
{
    char *s_start = s;
    char *t_start = t;
    while (*s && (n-- > 0)) {
        while (*s++ == *t++) {
            if (*t == 0) // Return index
                return s - s_start - (t - t_start);
        }
        t = t_start; // Reset t
    }
    return -1;
}

// Return pointer to first occurence of 'pat' in 'str'
// Checks only 'n' number of chars
// Return NULL if not found
char *strnstr(char *str, char *pat, size_t n)
{
    char *pat_start = pat;
    while (*str && (n-- > 0)) {
        while (*str++ == *pat++) {
            if (*pat == '\0') {
                // Pattern end reached
                size_t pat_len = pat - pat_start;
                return str - pat_len;
            }
        }
        pat = pat_start; // Reset pat
    }
    return NULL;
}

// Return index of last occurrence of t in s
// Return -1 if t not found in s
int strindex_last(char *s, char *t)
{
    size_t s_len = strlen(s);
    size_t t_len = strlen(t);

    char *s_start = s;
    char *s_end = s + s_len - 1;
    s = s_end;
    char *t_start = t;
    char *t_end = t + t_len - 1;
    t = t_end;

    while (s != s_start) {
        while (*s-- == *t--) {
            if ((t == t_start && *s == *t) || t_end == t_start) // Return index
                return s - s_start + ((t_end == t_start) ? 1 : 0);
        }
        t = t_end; // Reset t
    }
    return -1;
}

// Find next occurence of any char from 'chars' or '\0' in 'str' and
// return pointer to it.
// Both 'chars' and 'str' must be null-terminated, so it doesn't halt
char *find_next_any(char *str, char *chars)
{
    char *chars_start = chars;
    for (; *str != '\0'; str++) {
        for (; *chars != '\0'; chars++) {
            if (*str == *chars)
                return str;
        }
        chars = chars_start;
    }

    return str;
}

// Find next index of any char from 'c' or '\0' after 'str[i]'
// Return last index (length - 1) if not found
// Both 'c' and 'str' must be null-terminated, so it doesn't halt
inline size_t find_next_any_index(char *str, size_t i, char *c)
{
    return find_next_any(str + i, c) - str;
}

// Find first char 'c' between 'start' and 'end' (both inclusive)
// and return pointer to it.
// Return NULL if not found
char *strchr_range(char *start, char *end, char c)
{
    while (start <= end) {
        if (*start == c)
            return start;
        start++;
    }

    return NULL;
}

// Print string from 'start' and 'end' inclusive
void print_str_range(char *start, char* end)
{
    for (; start <= end; start++)
        printf("%c", *start);
}

// Print bytes from 'start' to 'end' inclusive as hex
void print_bytes(char *start, char *end)
{
    for (; start <= end; start++)
        printf("0x%X ", *start);
}

// Return pointer to first occurrence of 'pat' in 'str'
// Checks from 'str' to 'end' inclusive
// Return NULL if not found
char *strstr_range(char *str, char *end, char *pat)
{
    char *pat_start = pat;
    while (*str && (str <= end)) {
        while (*str++ == *pat++) {
            if (*pat == '\0') {
                // Pattern end reached
                size_t pat_len = pat - pat_start;
                return str - pat_len;
            }
        }
        pat = pat_start; // Reset pat
    }

    return NULL;
}

// Replaces last occurrence of sub in str with rep
// Returns 0 on success
// Returns 1 on failure
// str must be large enough to fit rep
int str_replace_last(char *str, char *sub, char *rep)
{
    int index = strindex_last(str, sub);
    if (index == -1) {
        return 1;
    }

    while (*rep)
        str[index++] = *rep++;

    return 0;
}

// Copy from 'src' to 'dest' up to (and excluding) the null terminator
// Returns number of chars copied
size_t copy_str_no_nullterm(char *dest, char *src)
{
    char *dest_start = dest;
    while (*src) {
        *dest++ = *src++;
    }
    return dest - dest_start;
}

// Sets input_file, output_file, and error_text
// Returns 0 on success, 1 on error
int parse_arguments(int argc, char* argv[],
    char *input_file, char *output_file, char *error_text)
{
    if (argc > MAX_ARGC) {
        strcpy(error_text, "error: too many arguments");
        return 1;
    }

    if (argc < MIN_ARGC) {
        strcpy(error_text, "error: input file not given");
        return 1;
    }

    *input_file = 0;
    *output_file = 0;

    for (int i = 1; i < argc; i++) {
        // Handle -o switch
        if (strindex(argv[i], "-o") > -1) {
            // Exit if multiple output files given
            if (*output_file != 0) {
                strcpy(error_text, "error: too many output files");
                return 1;
            }

            // Exit if option not spaced (ex: -ofile)
            // TODO better to separate -ofile into "-o" and "file" before
            //  passing argv to this function
            if (argv[i][2] != 0) {
                snprintf(error_text, ERR_TEXT_SIZE,
                    "error: unrecognized argument '%s'", argv[i]);
                return 1;
            }

            // Grab next arg as output_file
            if (i + 1 >= argc) {
                strcpy(error_text, "error: expected output file after '-o'");
                return 1;
            }
            strncpy(output_file, argv[i + 1], FILE_PATH_SIZE);
            i++;
        } else { // Argument is input_file
            // Exit if multiple input files given
            if (*input_file != 0) {
                strcpy(error_text, "error: too many input files");
                return 1;
            }
            strncpy(input_file, argv[i], FILE_PATH_SIZE);
        }
    }

    if (*input_file == 0) {
        strcpy(error_text, "error: input file not given");
        return 1;
    }

    if (*output_file == 0) {
        strncpy(output_file, input_file, FILE_PATH_SIZE);
        int err = str_replace_last(output_file, ".asm", ".hack");
        if (err != 0) { // input_file doesn't end with .asm
            strncat(output_file, ".hack",
                FILE_PATH_SIZE - strlen(output_file));
        }
    }

    return 0;
}

// String, but defined by a range in memory (inclusive)
// Doesn't have to be null-terminated
typedef struct {
    char *start; // inclusive
    char *end; // inclusive
} Slice;

// TODO move this out
// Copies Slice string into a malloced null-terminated string
// Returns pointer to new string
// NOTE: Only use for debugging & logging!
char *slice_to_str(Slice *slice)
{
    size_t slice_len = slice->end - slice->start + 1;
    char *str = malloc(sizeof(char) * (slice_len + 1));
    memcpy(str, slice->start, sizeof(char) * slice_len);
    str[slice_len] = '\0';
    return str;
}

// TODO move this out
// Return 0 if string and slice are equal. Return 1 otherwise
char cmp_str_slice(char *str, Slice *slice)
{
    char *slice_p = slice->start;
    while (*str && slice_p <= slice->end) {
        if (*str != *slice_p)
            return 1;
        slice_p++;
        str++;
    }

    if (*str == '\0' && slice_p == slice->end + 1)
        return 0;

    return 1;
}

// TODO move this out
void print_slice(Slice *slice)
{
    print_str_range(slice->start, slice->end);
}

typedef struct {
    char *p0;
    int p1;
}  Str_Int_Pair;

// Slow (but simple) associative array
Str_Int_Pair symbol_pairs[SYMBOL_PAIRS_SIZE] = {
    { "SP",     0 },
    { "LCL",    1 },
    { "ARG",    2 },
    { "THIS",   3 },
    { "THAT",   4 },
    { "R0",     0 },
    { "R1",     1 },
    { "R2",     2 },
    { "R3",     3 },
    { "R4",     4 },
    { "R5",     5 },
    { "R6",     6 },
    { "R7",     7 },
    { "R8",     8 },
    { "R9",     9 },
    { "R10",    10 },
    { "R11",    11 },
    { "R12",    12 },
    { "R13",    13 },
    { "R14",    14 },
    { "R15",    15 },
    { "SCREEN", 0x4000 },
    { "KBD",    0x6000 },
};
// Index of last element + 1 in symbol_pairs array
size_t symbol_pairs_i = 23;

// Returns pointer to pair if found. Returns NULL otherwise
Str_Int_Pair *find_pair_by_str(Str_Int_Pair a[], size_t len, char *str)
{
    for (size_t j = 0; j < len; j++) {
        if (strcmp(a[j].p0, str) == 0) {
            return a + j;
        }
    }
    return NULL;
}

// Returns pointer to pair if found. Returns NULL otherwise
Str_Int_Pair *find_pair_by_slice(Str_Int_Pair a[], size_t len, Slice *slice)
{
    for (size_t j = 0; j < len; j++) {
        if (cmp_str_slice(a[j].p0, slice) == 0) {
            return a + j;
        }
    }
    return NULL;
}

int log_pair(Str_Int_Pair *p)
{
    return printf("{ \"%s\", %i }", p->p0, p->p1);
}

// Subinstruction can be 'comp', 'dest', 'jump'...
typedef struct {
    char *str;
    size_t code;
    char *bin;
} Subinst_Code;

// TODO use hash table instead of this jandaba
/*const Subinst_Code comp_codes[] = {
    { "0",   "0101010" },
    { "1",   "0111111" },
    { "-1",  "0111010" },
    { "D",   "0001100" },
    { "A",   "0110000" }, { "M",   "" },
    { "D+A", "0000010" },
    { "D|A", "0010101" }, { "D|M", "1010101" },
};*/

// Comp syntax definition
enum COMP {
    COMP_NULL = 0,
    COMP_0,
    COMP_1,
    COMP_MINUS_1,
    COMP_D,
    COMP_A,          COMP_M,
    COMP_NOT_D,
    COMP_NOT_A,      COMP_NOT_M,
    COMP_MINUS_D,
    COMP_MINUS_A,    COMP_MINUS_M,
    COMP_D_PLUS_1,
    COMP_A_PLUS_1,   COMP_M_PLUS_1,
    COMP_D_MINUS_1,
    COMP_A_MINUS_1,  COMP_M_MINUS_1,
    COMP_D_PLUS_A,   COMP_D_PLUS_M,
    COMP_A_PLUS_D,   COMP_M_PLUS_D,
    COMP_D_MINUS_A,  COMP_D_MINUS_M,
    COMP_A_MINUS_D,  COMP_M_MINUS_D,
    COMP_D_AND_A,    COMP_D_AND_M,
    COMP_D_OR_A,     COMP_D_OR_M,
    COMP_A_AND_D,    COMP_M_AND_D,
    COMP_A_OR_D,     COMP_M_OR_D,
    COMP_PARSE_ERROR,
};

// Comp translation codes
const Subinst_Code comp_codes[] = {
    { "",    COMP_NULL,      "0101010" },
    { "0",   COMP_0,         "0101010" },
    { "1",   COMP_1,         "0111111" },
    { "-1",  COMP_MINUS_1,   "0111010" },
    { "D",   COMP_D,         "0001100" },
    { "A",   COMP_A,         "0110000" }, { "M",   COMP_M,         "1110000" },
    { "!D",  COMP_NOT_D,     "0001101" },
    { "!A",  COMP_NOT_A,     "0110001" }, { "!M",  COMP_NOT_M,     "1110001" },
    { "-D",  COMP_MINUS_D,   "0001111" },
    { "-A",  COMP_MINUS_A,   "0110011" }, { "-M",  COMP_MINUS_M,   "1110011" },
    { "D+1", COMP_D_PLUS_1,  "0011111" },
    { "A+1", COMP_A_PLUS_1,  "0110111" }, { "M+1", COMP_M_PLUS_1,  "1110111" },
    { "D-1", COMP_D_MINUS_1, "0001110" },
    { "A-1", COMP_A_MINUS_1, "0110010" }, { "M-1", COMP_M_MINUS_1, "1110010" },
    { "D+A", COMP_D_PLUS_A,  "0000010" }, { "D+M", COMP_D_PLUS_M,  "1000010" },
    { "A+D", COMP_A_PLUS_D,  "0000010" }, { "M+D", COMP_M_PLUS_D,  "1000010" },
    { "D-A", COMP_D_MINUS_A, "0010011" }, { "D-M", COMP_D_MINUS_M, "1010011" },
    { "A-D", COMP_A_MINUS_D, "0000111" }, { "M-D", COMP_M_MINUS_D, "1000111" },
    { "D&A", COMP_D_AND_A,   "0000000" }, { "D&M", COMP_D_AND_M,   "1000000" },
    { "D|A", COMP_D_OR_A,    "0010101" }, { "D|M", COMP_D_OR_M,    "1010101" },
    { "A&D", COMP_A_AND_D,   "0000000" }, { "M&D", COMP_M_AND_D,   "1000000" },
    { "A|D", COMP_A_OR_D,    "0010101" }, { "M|D", COMP_M_OR_D,    "1010101" },
};
const size_t comp_code_count = sizeof(comp_codes) / sizeof(Subinst_Code);

// Dest syntax definition
enum DEST {
    DEST_NULL = 0b000,
    DEST_M    = 0b001,
    DEST_D    = 0b010,
    DEST_MD   = 0b011,
    DEST_A    = 0b100,
    DEST_AM   = 0b101,
    DEST_AD   = 0b110,
    DEST_AMD  = 0b111,
    DEST_PARSE_ERROR,
};

// Dest translation codes
const Subinst_Code dest_codes[] = {
    { "",    DEST_NULL, "000" },
    { "M",   DEST_M,    "001" },
    { "D",   DEST_D,    "010" },
    { "MD",  DEST_MD,   "011" },
    { "A",   DEST_A,    "100" },
    { "AM",  DEST_AM,   "101" },
    { "AD",  DEST_AD,   "110" },
    { "AMD", DEST_AMD,  "111" },
};
const size_t dest_code_count = sizeof(dest_codes) / sizeof(Subinst_Code);

// Jump syntax definitions
enum JUMP {
    JUMP_NULL = 0,
    JGT, JEQ, JGE,
    JLT, JNE, JLE,
    JMP,
    JUMP_PARSE_ERROR,
};

// Jump translation codes
const Subinst_Code jump_codes[] = {
    { "",    JUMP_NULL, "000" },
    { "JGT", JGT,       "001" },
    { "JEQ", JEQ,       "010" },
    { "JGE", JGE,       "011" },
    { "JLT", JLT,       "100" },
    { "JNE", JNE,       "101" },
    { "JLE", JLE,       "110" },
    { "JMP", JMP,       "111" },
};
const size_t jump_code_count = sizeof(jump_codes) / sizeof(Subinst_Code);

enum INST_TYPE { A_INST, C_INST };

typedef struct {
    enum INST_TYPE type;
    void *inst;
} Instruction;

typedef struct {
    Slice *symbol;
    unsigned int value;
    int eval; // true if .value is correct (or was evaluated)
} A_Instruction;

typedef struct {
    enum DEST dest;
    enum COMP comp;
    enum JUMP jump;
} C_Instruction;

void free_instruction(Instruction *i)
{
    if (i->type == A_INST) {
        A_Instruction* a = (A_Instruction*) i->inst;
        if (a->symbol)
            free(a->symbol);
    }
    free(i->inst);
}

// Returns a^b
// 'b' must be non-negative (given the return type of the function)
int power(int a, int b)
{
    if (a == 0)
        return 0;

    int ans = 1;
    while (b-- > 0)
        ans *= a;

    return ans;
}

// Returns true if the number satisfies the following
// reg expression: ^-?[0-9]+$
// Returns false otherwise
int is_valid_value(char *buf, char *end)
{
    // Swallow leading minus, if present
    if (*buf == '-') {
        buf++;
    }

    if (buf > end)
        return 0;

    for (; buf <= end; buf++) {
        if (!is_number(*buf)) {
            return 0;
        }
    }

    return 1;
}

// Parses and returns first int from 'buf' to 'end' inclusive.
// NOTE: The given range MUST ONLY contain digits or a leading minus,
//  otherwise, undefined behavior
int parse_next_int(char *buf, char *end)
{
    int num = 0;
    int negative = 0;

    // Check if negative
    if (*buf == '-') {
        negative = 1;
        buf++;
    }

    // Skip leading zeros
    if (*buf == '0') {
        while (*(buf + 1) == '0') {
            buf++;
        }
    }

    // Build num up backwards
    char *p = end;
    size_t rpos = 0; // Position of current digit from right
    while (p >= buf) {
        int n = *p - '0';
        num += n * power(10, rpos);
        p--;
        rpos++;
    }

    if (negative)
        return -num;

    return num;
}

// Converts 15-bit int to binary (ascii) string. Returns its pointer
// TODO don't use malloc and write to passed char *str.
char *int15_to_bin_str(unsigned int n)
{
    char *str = malloc(sizeof(char) * 16);
    str[15] = '\0';
    int i = 14;
    while (n > 0 && i >= 0) {
        int rem = n % 2;
        str[i] = '0' + rem;
        n /= 2;
        i--;
    }

    // Pad with zeros
    while (i >= 0) {
        str[i] = '0';
        i--;
    }

    return str;
}

// Parses from 'buf' to 'end' inclusive
// Returns first symbol found as malloced Slice
// Returns NULL on parse error
Slice *parse_next_symbol(char *buf, char *end)
{
    // Check head (first char)
    if (!is_valid_symbol_head(buf[0])) {
        return NULL;
    }

    size_t i = 1; // Continue checking from second char
    for (; buf + i <= end; i++) {
        if (is_valid_symbol_tail(buf[i]))
            continue;
        /* The if below enables the use of the following case in asm:
                @MYVARIABLE       // declare my var\n
                 ^---+----^^--+--^^-----------+---^
                     |        |               |
                  symbol   blankspace      comment

           Here, 'buf' points to 'M' and 'end' points to
           the space before the first '/' */
        if (buf[i] == ' ' || buf[i] == '\t') {
            /* If symbol is followed by anything but whtiespace, exit.
               Ex: '@MYVAR    ASD' should give fatal error.
                     ^          ^
                    buf        end
            */
            size_t j = i;
            for (; buf + j <= end; j++) {
                if (buf[j] == ' ' || buf[j] == '\t')
                    continue;
                return NULL;
            }
        }
        return NULL;
    }

    Slice *symbol = malloc(sizeof(Slice));
    symbol->start = buf;
    symbol->end = buf + i - 1;
    return symbol;
}


// Parses A-instruction from 'buf' to 'end' inclusive
// Returns NULL on parse error
A_Instruction *parse_a_instruction(char *buf, char *end)
{
    A_Instruction tmp = {
        .symbol = NULL,
        .value = 0,
        .eval = 0,
    };

    buf++; // Stand on char after '@'

    // Skip blankspace
    while (*buf == ' ' || *buf == '\t')
        buf++;

    if (is_number(*buf) || *buf == '-') {
        // Values can start with [\-0-9]
        // Check for parse errors
        if (is_valid_value(buf, end)) {
            tmp.value = parse_next_int(buf, end);
            tmp.eval = 1;
        } else {
            return NULL;
        }
    } else if (is_alpha(*buf) || *buf == '_' || *buf == '.' ||
        *buf == '%' || *buf == ':') {
        // Symbols can start with [a-zA-Z_.%:]
        tmp.symbol = parse_next_symbol(buf, end);
        // Check if parse error
        if (tmp.symbol == NULL)
            return NULL;
    } else {
        // Parse error
        return NULL;
    }

    A_Instruction *inst = malloc(sizeof(A_Instruction));
    memcpy(inst, &tmp, sizeof(A_Instruction));
    return inst;
}

// Parses dest in C-instruction between 'buf' and 'end'
// Returns DEST_PARSE_ERROR if parse error encountered
// TODO don't allow multiple occurrences of a,m, or d. Ex: 'AAA', 'MDD'...
enum DEST parse_c_dest(char *buf, char *end)
{
    enum DEST dest = DEST_NULL;
    for (; buf <= end; buf++) {
        // A
        if (*buf == 'a' || *buf == 'A') {
            dest |= DEST_A;
            continue;
        }

        // M
        if (*buf == 'm' || *buf == 'M') {
            dest |= DEST_M;
            continue;
        }

        // D
        if (*buf == 'd' || *buf == 'D') {
            dest |= DEST_D;
            continue;
        }

        // Skip whitespace
        if (*buf == ' ' || *buf == '\t')
            continue;

        // Invalid token
        return DEST_PARSE_ERROR;
    }
    return dest;
}

// Parses comp in C-instruction from 'buf' to 'end' inclusive
// Returns COMP_PARSE_ERROR if parse error encountered
enum COMP parse_c_comp(char *buf, char *end)
{
    // Gather tokens and push them upcased into comp_str
    char comp_str[MAX_COMP_TOKEN_COUNT + 1];
    size_t i = 0; // index for comp_str
    for (; buf <= end; buf++) {
        switch (*buf) {
        // Skip whitespace
        case ' ':
        case '\t':
            break;
        // Valid tokens
        case 'A':
        case 'a':
        case 'M':
        case 'm':
        case 'D':
        case 'd':
        case '0':
        case '1':
        case '+':
        case '-':
        case '!':
        case '&':
        case '|':
            if (i >= MAX_COMP_TOKEN_COUNT) {
                printf("Too many tokens in comp section\n");
                return COMP_PARSE_ERROR;
            }
            comp_str[i] = toupper(*buf);
            i++;
            break;
        // Everything else is invalid
        default:
            printf("Invalid token '%c' in comp\n", *buf);
            return COMP_PARSE_ERROR;
        }
    }

    // No tokens
    if (i == 0) {
        return COMP_NULL;
    }

    // Terminate comp_str
    comp_str[i] = '\0';

    // Compare comp_str to comp_codes table
    for (size_t j = 0; j < comp_code_count; j++) {
        if (strcmp(comp_codes[j].str, comp_str) == 0) {
            return comp_codes[j].code;
        }
    }

    return COMP_NULL;
}

// Parses jump in C-instruction between 'buf' and 'end
// Returns JUMP_PARSE_ERROR if parse error encountered
enum JUMP parse_c_jump(char *buf, char *end)
{
    if (buf == end)
        return JUMP_NULL;

    for (size_t i = 0; i < jump_code_count; i++) {
        if (strstr_range(buf, end, jump_codes[i].str) != NULL) {
            return jump_codes[i].code;
        }
    }

    return JUMP_PARSE_ERROR;
}

// Parses C-instruction from 'buf' to 'end' inclusive
// Returns NULL on fatal parse error
// Examples of instructions allowed: 'JMP', '0;JMP', ';JMP', '=JMP',
//    '=;JMP', 'D', 'AM=0;JEQ', 'comp', 'jump', '=comp'
// TODO make parse_c_subinst functions return Subinstruction structs
C_Instruction *parse_c_instruction(char *buf, char* end)
{
    C_Instruction tmp = {
        .dest = DEST_NULL,
        .comp = COMP_NULL,
        .jump = JUMP_NULL,
    };

    // Dest
    char *dest_start = buf;
    char *eq = strchr_range(buf, end, '=');
    if (eq == NULL) {
        // Dest is empty
        tmp.dest = DEST_NULL;
        buf = dest_start; // comp starts parsing from i
    } else {
        tmp.dest = parse_c_dest(dest_start, eq - 1);
        buf = eq + 1; // stand on char after '='
    }

    // Skip blankspace between 'dest=' and 'comp'
    while (*buf == ' ' || *buf == '\t')
        buf++;

    // Comp
    char *comp_start = buf;
    char *semicolon = strchr_range(comp_start, end, ';');
    char *comp_end;
    if (semicolon == NULL) {
        // No ';' found, parse to end of line
        comp_end = end;
    } else {
        comp_end = semicolon - 1;
        buf = semicolon + 1; // stand on char after ';'
    }

    tmp.comp = parse_c_comp(comp_start, comp_end);

    // Skip blankspace between 'comp;' and 'jump'
    while (*buf == ' ' || *buf == '\t')
        buf++;

    // Jump
    // If 'comp' and 'jump' overlap
    if (comp_end == end) {
        // Comp takes perendence over jump so if they overlap and
        // comp parses ok, then we don't have a jump
        if (tmp.comp != COMP_NULL && tmp.comp != COMP_PARSE_ERROR) {
            tmp.jump = JUMP_NULL;
        } else {
            tmp.jump = parse_c_jump(buf, end);
        }
    } else {
        tmp.jump = parse_c_jump(buf, end);
    }

    // Exit if parse error
    if (tmp.jump == JUMP_PARSE_ERROR) {
        return NULL;
    }

    C_Instruction *inst = malloc(sizeof(C_Instruction));
    memcpy(inst, &tmp, sizeof(C_Instruction));
    return inst;
}

void log_a_inst(A_Instruction *inst)
{
    printf("A_Instruction {\n");
    // Symbol
    printf("\t.symbol = ");
    if (inst->symbol) {
        printf("\"");
        print_slice(inst->symbol);
        printf("\"");
    } else {
        printf("NULL");
    }
    printf("\n");
    // Value
    printf("\t.value = %i\n", inst->value);
    // Evaluated
    printf("\t.eval = %i\n}\n", inst->eval);
}

void log_c_inst(C_Instruction *inst)
{
    printf("C_Instruction {\n"
        "\t.dest = %s (0x%X)\n"
        "\t.comp = %s (0x%X)\n"
        "\t.jump = %s (0x%X)\n}\n",
        (inst->dest >= DEST_PARSE_ERROR) ?
            "DEST_PARSE_ERROR" : dest_codes[inst->dest].str,
        inst->dest,
        (inst->comp >= COMP_PARSE_ERROR) ?
            "COMP_PARSE_ERROR" : comp_codes[inst->comp].str,
        inst->comp,
        (inst->jump >= JUMP_PARSE_ERROR) ?
            "JUMP_PARSE_ERROR" : jump_codes[inst->jump].str,
        inst->jump);
}

void log_inst(Instruction *inst)
{
    if (inst->type == A_INST)
        log_a_inst(inst->inst);
    else if (inst->type == C_INST)
        log_c_inst(inst->inst);
    else
        printf("log_inst: invalid INST_TYPE %i\n", inst->type);
}

int main(int argc, char* argv[])
{
    char input_file_path[FILE_PATH_SIZE];
    char output_file_path[FILE_PATH_SIZE];
    char error_text[ERR_TEXT_SIZE];

    // Parse arguments
    int err = parse_arguments(argc, argv, input_file_path, output_file_path,
        error_text);
    if (err == 1) {
        printf("%s\n", error_text);
        return 1;
    }

    // Completely read file into a buffer
    size_t input_file_size;
    char *input_buf = load_file(input_file_path, &input_file_size);

    // Initialize instruction array
    size_t instructions_capacity = INST_ARRAY_STARTING_CAPACITY;
    Instruction *instructions = malloc(instructions_capacity *
        sizeof(Instruction));

    // Parse code into instruction array and populate symbol table with labels
    size_t inst_count = 0;
    size_t src_line_count = 0; // for pointing out errors
    for (size_t i = 0; input_buf[i] != '\0';) {
        // Skip whitespace
        switch (input_buf[i]) {
        case ' ':
        case '\t':
        case '\r':
            i++;
            continue;
        case '\n':
            i++;
            src_line_count++;
            continue;
        }

        // Handle comment
        if (input_buf[i] == '/' && input_buf[i+1] == '/') {
            // Skip rest of line
            i = find_next_any_index(input_buf, i, "\r\n") + 1;
            continue;
        }

        // Handle A-instruction
        if (input_buf[i] == '@') {
            // TODO write 'i++;' here but check that tests pass
            // Skip blankspace between '@' and symbol
            while (input_buf[i] == ' ' || input_buf[i] == '\t')
                i++;

            // Find end of line
            char *end = find_next_any(input_buf + i, " \r\n");

            // Find comment between token and end of line
            char *comment_start = strstr_range(input_buf + i, end - 1, "//");
            if (comment_start != NULL) {
                // Parse from start of line until comment start
                end = comment_start;
            }

            // Parse
            A_Instruction *ainst = parse_a_instruction(input_buf + i, end - 1);

            // Check for error
            if (ainst == NULL) {
                printf("Parse error at line %li\n", src_line_count + 1);
                return 1;
            }

            // Add parsed instruction to array
            if (inst_count >= instructions_capacity) {
                // Realloc if more space needed
                instructions_capacity += INST_ARRAY_CAPACITY_GROWTH_RATE;
                instructions = realloc(instructions,
                    instructions_capacity * sizeof(Instruction));
            }
            instructions[inst_count] = (Instruction) {
                .type = A_INST,
                .inst = ainst
            };

#if LOG_PARSER_OUTPUT == 1
            // Log
            printf("%li: [%li]", src_line_count + 1, inst_count);
            log_inst(&instructions[inst_count]);
#endif

            inst_count++;

            // Skip rest of line
            i = find_next_any_index(input_buf, i, "\r\n") + 1;
            continue;
        }

        // Handle label (push into symbol table)
        if (input_buf[i] == '(') {
            // Find end of line
            char *end = find_next_any(input_buf + i, "\r\n");

            i++; // Stand on char after '('

            // Find comment between token and end of line
            char *comment_start = strstr_range(input_buf + i, end - 1, "//");
            if (comment_start != NULL) {
                // Parse from start of line until comment start
                end = comment_start;
            }

            // Check head (first char)
            if (!is_valid_symbol_head(input_buf[i])) {
                printf("Parse error at line %li\n", src_line_count + 1);
                printf("Label names must start with a letter\n");
                return 1;
            }

            Slice label = { .start = input_buf + i, .end = 0 };
            i++; // Move to second char
            // Walk to char after label name
            for (; input_buf + i < end; i++) {
                if (!is_valid_symbol_tail(input_buf[i]))
                    break;
            }

            // Skip blankspace between label name and ')'
            while (is_blank(input_buf[i]))
                i++;

            if (input_buf[i] != ')') {
                printf("Parse error at line %li\n", src_line_count + 1);
                printf("Missing ')' for label definition\n");
                return 1;
            }

            // TODO allow colon after label definition

            // Mark end of label slice
            label.end = input_buf + i - 1;

            // Find duplicate
            // TODO remove this conversion
            char *label_str = slice_to_str(&label);
            Str_Int_Pair *p = find_pair_by_str(symbol_pairs, symbol_pairs_i,
                label_str);

            // Duplicate found
            if (p != NULL) {
                printf("Error at line %li\n", src_line_count + 1);
                printf("Duplicate symbol definition of '");
                print_slice(&label);
                printf("'\n");
                free(label_str);
                return 1;
            }

            // Insert into symbol table TODO change to hash map
            symbol_pairs[symbol_pairs_i++] = (Str_Int_Pair) {
                .p0 = label_str,
                .p1 = inst_count,
            };

#if LOG_PARSER_OUTPUT == 1
            // Log
            printf("%li: ", src_line_count + 1);
            printf("Label '");
            print_slice(&label);
            printf("' -> instruction %li\n", inst_count);
#endif

            // Skip rest of line
            i = find_next_any_index(input_buf, i, "\r\n") + 1;
            continue;
        }

        // Handle C-instruction
        if (is_alpha(input_buf[i]) || is_number(input_buf[i]) ||
            input_buf[i] == ';') {
            // Find end of line
            char *end = find_next_any(input_buf + i, "\r\n");

            // Find comment between token and end of line
            char *comment_start = strstr_range(input_buf + i, end - 1, "//");
            if (comment_start != NULL) {
                // Parse from start of line until comment start
                end = comment_start;
            }

            // Parse
            C_Instruction *cinst = parse_c_instruction(input_buf + i, end - 1);
            if (cinst == NULL) {
                printf("Parse error at line %li\n", src_line_count + 1);
                return 1;
            }

            // Add parsed instruction to array
            if (inst_count >= instructions_capacity) {
                instructions_capacity += INST_ARRAY_CAPACITY_GROWTH_RATE;
                instructions = realloc(instructions,
                    instructions_capacity * sizeof(Instruction));
            }
            instructions[inst_count] = (Instruction) {
                .type = C_INST,
                .inst = cinst
            };

#if LOG_PARSER_OUTPUT == 1
            // Log
            printf("%li: [%li]", src_line_count + 1, inst_count);
            log_inst(&instructions[inst_count]);
#endif

            inst_count++;

            // Skip rest of line
            i = find_next_any_index(input_buf, i, "\r\n") + 1;
            continue;
        }
    }

#if LOG_PARSER_OUTPUT == 1
    // Dump symbol table before evals
    printf("symbol_pairs = {\n");
    for (size_t j = 0; j < symbol_pairs_i; j++) {
        printf("\t");
        log_pair(symbol_pairs + j);
        printf("\n");
    }
    printf("}\n");
#endif

    // Allocate memory for output buffer
    // 16 bytes for the instruction code on each line
    // 1 byte for newline on each line
    // 1 byte at the end for null terminator
    size_t output_buf_size = sizeof(char) * 17 * inst_count + 1;
    char *output_buf = malloc(output_buf_size);
    char *output_buf_p = output_buf;

    // First pass
    // Fill in symbol values and set each a_inst.eval to true
    size_t mem = 16;
    for (size_t i = 0; i < inst_count; i++) {
        if (instructions[i].type == C_INST)
            continue;
        
        A_Instruction *a_inst = (A_Instruction*) (instructions[i].inst);

        // Skip if already evaluated
        if (a_inst->eval == 1)
            continue;

        // This could save us a huge headache in the future
        if (a_inst->symbol == NULL) {
            printf("FATAL ERROR: unevaluated A_Instruction symbol "
                "can't be NULL\n");
            printf("Check instruction %li\n", i);
            return 1;
        }

        // Scan symbol table TODO use hash func here (when we have hash map)
        Str_Int_Pair *pair = find_pair_by_slice(symbol_pairs, symbol_pairs_i,
            a_inst->symbol);
        // Symbol found in table, evaluate it
        if (pair != NULL) {
            a_inst->value = pair->p1;
        } else {
            // Symbol not found in table
            // Insert symbol into table and assign unused static memory address
            // TODO change to hash map
            char *symbol_str = slice_to_str(a_inst->symbol);
            symbol_pairs[symbol_pairs_i++] = (Str_Int_Pair) {
                .p0 = symbol_str,
                .p1 = mem, // Assign unused static memory address
            };
            a_inst->value = mem;
            mem++;
        }

        // Mark as evaluated
        a_inst->eval = 1;
    }

#if LOG_PARSER_OUTPUT == 1
    // Dump symbol table after evals
    printf("symbol_pairs (after evals) = {\n");
    for (size_t j = 0; j < symbol_pairs_i; j++) {
        printf("\t");
        log_pair(symbol_pairs + j);
        printf("\n");
    }
    printf("}\n");
#endif

    // Generate code
    for (size_t i = 0; i < inst_count; i++) {
#if LOG_PARSER_OUTPUT == 1
        printf("[%li]: ", i);
        log_inst(&instructions[i]);
#endif
        switch (instructions[i].type) {
        case A_INST: {
            *output_buf_p++ = '0';
            A_Instruction *inst = (A_Instruction*) (instructions[i].inst);
            char *bin = int15_to_bin_str(inst->value);
            output_buf_p += copy_str_no_nullterm(output_buf_p, bin);
            free(bin);
            *output_buf_p++ = '\n';
            break;
        }
        case C_INST: {
            output_buf_p += copy_str_no_nullterm(output_buf_p, "111");
            C_Instruction *inst = (C_Instruction*) (instructions[i].inst);
            output_buf_p += copy_str_no_nullterm(
                output_buf_p, comp_codes[inst->comp].bin);
            output_buf_p += copy_str_no_nullterm(
                output_buf_p, dest_codes[inst->dest].bin);
            output_buf_p += copy_str_no_nullterm(
                output_buf_p, jump_codes[inst->jump].bin);

            *output_buf_p++ = '\n';
            break;
        }
        default:
            printf("Invalid INST_TYPE in instruction %li\n", i);
            return 1;
        }
    }

    *output_buf_p++ = '\0';

    // Write file (size - 1 to exclude null terminator)
    int error = write_file(output_buf, output_file_path, output_buf_size - 1);
    if (error) {
        printf("Error when writing to '%s'\n", output_file_path);
    }

    // Free all memory
    free(input_buf);
    for (size_t i = 0; i < inst_count; i++)
        free_instruction(instructions + i);
    free(instructions);
    free(output_buf);
    return 0;
}
