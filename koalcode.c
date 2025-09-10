/*
  Interpreter for >_KoalCode
*/

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <GL/gl.h>
#include <pthread.h>
#include <unistd.h>
#include <curl/curl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct { GLuint texture_id; char *filename; int width, height; } Texture;
typedef struct { float x,y,z; } Vertex3D;
typedef struct { int v1,v2,v3; float u1,vt1,u2,vt2,u3,vt3; } Face3D;
typedef struct { Vertex3D *vertices; Face3D *faces; int vertex_count, face_count; char *filename; GLuint texture_id; } Model3D;

static SDL_Window *g_sdl_window = NULL;
static SDL_GLContext g_gl_context = NULL;
static int g_graphics_initialized = 0;
static int g_window_width = 800;
static int g_window_height = 600;

/* Global network */
static CURL *g_curl_handle = NULL;
static int g_network_initialized = 0;

/* Network connection structures */
typedef struct {
    int socket_fd;
    char *host;
    int port;
    int connected;
} SocketConnection;

typedef struct {
    char *url;
    char *method;
    char *headers;
    char *data;
    int timeout;
} HttpRequest;

typedef struct {
    int status_code;
    char *response_data;
    char *headers;
    int success;
} HttpResponse;

/*=====================================================================
 * 1.   TOKENS, OPCODES, AST
 *===================================================================== */

typedef enum {
    TT_IDENTIFIER,
    TT_NUMBER,
    TT_STRING,
    TT_SYMBOL,
    TT_EOF
} TokenType;

typedef struct {
    TokenType type;
    char *lexeme;        /* NULL para números */
    double num;          /* válido somente se type == TT_NUMBER */
} Token;

typedef enum {
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
    OP_POW,

    OP_LT, OP_LE, OP_GT, OP_GE,
    OP_EQ, OP_NE,

    OP_BITAND, OP_BITOR, OP_BITXOR,
    OP_SHL, OP_SHR,

    OP_LOGICAL_AND, OP_LOGICAL_OR,

    OP_ASSIGN,                 /* = */

    OP_NEG, OP_NOT, OP_BITNOT,/* unários */

    OP_UNKNOWN
} OpCode;

typedef enum {
    NODE_BINARY,
    NODE_UNARY,
    NODE_NUMBER,
    NODE_STRING,
    NODE_VAR,
    NODE_CALL,
    NODE_CLASS_DECL,
    NODE_BLOCK,
    NODE_WHILE,
    NODE_IF,
    NODE_THREAD_START,
    NODE_FUNC_DECL,   /* declaração de função com o nome  'fuktion' */
    NODE_RETURN  
} NodeType;

typedef struct Node {
    NodeType type;
    union {
        struct { struct Node *left; struct Node *right; OpCode op; } bin;
        struct { OpCode op; struct Node *operand; } unary;
        double num;
        char *str;
        char *var_name;
        struct {
            char *func_name;
            struct Node **args;
            size_t nargs;
        } call;
        struct {
            char *class_name;
            struct Node *body;
        } class_decl;
        struct {
            struct Node **stmts;
        } block;
        struct {
            struct Node *cond;
            struct Node *body;
        } while_node;
        struct {
            struct Node *cond;
            struct Node *then_body;
            struct Node *else_body;
        } if_node;
        /* FUNCTION DECL */
        struct {
            char *name;
            char **params;
            size_t nparams;
            struct Node *body; /* bloco */
        } func_decl;
        struct {
            struct Node *expr;
        } return_node;
        char *thread_name;
    } data;
} Node;

/*=====================================================================
 * 2.   TOKEN STREAM / LEXER
 *===================================================================== */

typedef struct {
    Token *tokens;
    size_t size;
    size_t capacity;
    size_t pos;
} TokenStream;

static void tokens_init(TokenStream *ts) {
    ts->size = 0;
    ts->capacity = 64;
    ts->pos = 0;
    ts->tokens = calloc(ts->capacity, sizeof(Token));
}
static void tokens_append(TokenStream *ts, Token tk) {
    if (ts->size == ts->capacity) {
        ts->capacity *= 2;
        ts->tokens = realloc(ts->tokens, ts->capacity * sizeof(Token));
    }
    ts->tokens[ts->size++] = tk;
}
static void token_free(Token *tk) {
    if (tk->lexeme) free(tk->lexeme);
}

static void skip_ws_and_comments(const char **src) {
    const char *p = *src;
    while (1) {
        while (isspace((unsigned char)*p)) p++;
        if (p[0] == '-' && p[1] == '-') {
            p += 2;
            while (*p && *p != '\n') p++;
            continue;
        }
        break;
    }
    *src = p;
}

static Token next_token(const char **src) {
    const char *p = *src;
    skip_ws_and_comments(&p);

    if (*p == '\0') {
        *src = p;
        return (Token){ TT_EOF, NULL, 0 };
    }

    if (isalpha((unsigned char)*p) || *p == '_') {
        const char *start = p;
        while (isalnum((unsigned char)*p) || *p == '_' || *p == '.') p++;
        size_t len = p - start;
        char *lex = strndup(start, len);
        *src = p;
        return (Token){ TT_IDENTIFIER, lex, 0 };
    }

    if (isdigit((unsigned char)*p) ||
        (*p == '.' && isdigit((unsigned char)p[1]))) {
        const char *start = p;
        while (isdigit((unsigned char)*p)) p++;
        if (*p == '.') {
            p++;
            while (isdigit((unsigned char)*p)) p++;
        }
        size_t len = p - start;
        char *numstr = strndup(start, len);
        double num = strtod(numstr, NULL);
        free(numstr);
        *src = p;
        return (Token){ TT_NUMBER, NULL, num };
    }

    if (*p == '\"') {
        p++;
        const char *start = p;
        while (*p && *p != '\"') p++;
        size_t len = p - start;
        char *str = strndup(start, len);
        if (*p == '\"') p++;
        *src = p;
        return (Token){ TT_STRING, str, 0 };
    }

    {
        const char *multi[] = {
            "==", "!=", "~=", "<=", ">=", "&&", "||",
            "<<", ">>", "**",
            "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=",
            "<<=", ">>=", "**="
        };
        for (size_t i = 0; i < sizeof(multi)/sizeof(multi[0]); ++i) {
            size_t len = strlen(multi[i]);
            if (strncmp(p, multi[i], len) == 0) {
                char *lex = strndup(p, len);
                p += len;
                *src = p;
                return (Token){ TT_SYMBOL, lex, 0 };
            }
        }
    }

    if (strchr("(){}[];=+-*/%.,<>!&|^~", *p)) {
        char *lex = malloc(2);
        lex[0] = *p;
        lex[1] = '\0';
        p++;
        *src = p;
        return (Token){ TT_SYMBOL, lex, 0 };
    }

    fprintf(stderr, "Lexical error near '%c'\n", *p);
    p++;
    *src = p;
    return next_token(src);
}

static TokenStream tokenize(const char *src) {
    TokenStream ts;
    tokens_init(&ts);
    while (1) {
        Token tk = next_token(&src);
        if (tk.type == TT_EOF) {
            token_free(&tk);
            break;
        }
        tokens_append(&ts, tk);
    }
    Token eof_tok = { TT_EOF, NULL, 0 };
    tokens_append(&ts, eof_tok);
    return ts;
}

/*=====================================================================
 * 3.   PARSER
 *===================================================================== */

static TokenStream *global_ts;
static size_t global_tok_pos;

static Token peek(void)          { return global_ts->tokens[global_tok_pos]; }
static Token consume(void)       { return global_ts->tokens[global_tok_pos++]; }
static Token peek_next(void) {
    if (global_tok_pos + 1 >= global_ts->size) return (Token){ TT_EOF, NULL, 0 };
    return global_ts->tokens[global_tok_pos + 1];
}
static int match(TokenType type, const char *lexeme) {
    Token t = peek();
    if (t.type != type) return 0;
    if (lexeme && (!t.lexeme || strcmp(t.lexeme, lexeme) != 0)) return 0;
    return 1;
}
static void advance(void) {
    if (global_tok_pos < global_ts->size) global_tok_pos++;
}

static void skip_separators(void) {
    while (match(TT_SYMBOL, ";")) consume();
}

static int is_assign_operator(const char *lex) {
    const char *ops[] = { "=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=",
                          "<<=", ">>=", "**=" };
    for (size_t i = 0; i < sizeof(ops)/sizeof(ops[0]); ++i)
        if (strcmp(lex, ops[i]) == 0) return 1;
    return 0;
}

static Node *parse_expression(void);
static Node *parse_primary(void);
static Node *parse_unary(void);
static Node *parse_exponent(void);
static Node *parse_mul_div_mod(void);
static Node *parse_add_sub(void);
static Node *parse_shift(void);
static Node *parse_relational(void);
static Node *parse_equality(void);
static Node *parse_bitwise_and(void);
static Node *parse_bitwise_xor(void);
static Node *parse_bitwise_or(void);
static Node *parse_logical_and(void);
static Node *parse_logical_or(void);
static Node *parse_assignment(void);
static Node *parse_block(void);
static Node *parse_if(void);
static Node *parse_statement(void);
static Node **parse_program(void);
static Node *parse_while(void);
static Node *parse_class_decl(void);
static Node *parse_function_decl(void);
static Node *parse_return_stmt(void);
static void free_node(Node *node);

/* utilidades de OPcode :D */
static OpCode sym_to_binop(const char *lex) {
    if (!lex) return OP_UNKNOWN;
    if (strcmp(lex, "+")  == 0) return OP_ADD;
    if (strcmp(lex, "-")  == 0) return OP_SUB;
    if (strcmp(lex, "*")  == 0) return OP_MUL;
    if (strcmp(lex, "/")  == 0) return OP_DIV;
    if (strcmp(lex, "%")  == 0) return OP_MOD;
    if (strcmp(lex, "**") == 0) return OP_POW;
    if (strcmp(lex, "<")  == 0) return OP_LT;
    if (strcmp(lex, "<=") == 0) return OP_LE;
    if (strcmp(lex, ">")  == 0) return OP_GT;
    if (strcmp(lex, ">=") == 0) return OP_GE;
    if (strcmp(lex, "==") == 0) return OP_EQ;
    if (strcmp(lex, "!=") == 0 || strcmp(lex, "~=") == 0) return OP_NE;
    if (strcmp(lex, "&")  == 0) return OP_BITAND;
    if (strcmp(lex, "|")  == 0) return OP_BITOR;
    if (strcmp(lex, "^")  == 0) return OP_BITXOR;
    if (strcmp(lex, "<<") == 0) return OP_SHL;
    if (strcmp(lex, ">>") == 0) return OP_SHR;
    if (strcmp(lex, "&&") == 0) return OP_LOGICAL_AND;
    if (strcmp(lex, "||") == 0) return OP_LOGICAL_OR;
    if (strcmp(lex, "=")  == 0) return OP_ASSIGN;
    return OP_UNKNOWN;
}

/* ---------- parse_primary ---------- */
static Node *parse_primary(void) {
    Token t = peek();

    if (t.type == TT_NUMBER) {
        consume();
        Node *n = malloc(sizeof(Node));
        n->type = NODE_NUMBER;
        n->data.num = t.num;
        return n;
    }

    if (t.type == TT_STRING) {
        consume();
        Node *n = malloc(sizeof(Node));
        n->type = NODE_STRING;
        n->data.str = strdup(t.lexeme);
        return n;
    }

    if (t.type == TT_IDENTIFIER) {
        Token id = consume();

        if (match(TT_SYMBOL, "(")) {
            consume();
            Node **args = NULL;
            size_t cap = 4, len = 0;
            args = calloc(cap, sizeof(Node *));
            if (!match(TT_SYMBOL, ")")) {
                while (1) {
                    Node *arg = parse_expression();
                    if (len == cap) {
                        cap *= 2;
                        args = realloc(args, cap * sizeof(Node *));
                    }
                    args[len++] = arg;

                    if (match(TT_SYMBOL, ",")) {
                        consume();
                        continue;
                    }
                    break;
                }
            }
            if (!match(TT_SYMBOL, ")")) {
                fprintf(stderr, "Expected ')' after argument list\n");
                exit(1);
            }
            consume();

            Node *call = malloc(sizeof(Node));
            call->type = NODE_CALL;
            call->data.call.func_name = strdup(id.lexeme);
            call->data.call.args = args;
            call->data.call.nargs = len;
            return call;
        }

        Node *var = malloc(sizeof(Node));
        var->type = NODE_VAR;
        var->data.var_name = strdup(id.lexeme);
        return var;
    }

    if (match(TT_SYMBOL, "(")) {
        consume();
        Node *inner = parse_expression();
        if (!match(TT_SYMBOL, ")")) {
            fprintf(stderr, "Expected ')' after expression\n");
            exit(1);
        }
        consume();
        return inner;
    }

    fprintf(stderr, "Parse error (unexpected token \"%s\")\n",
            t.lexeme ? t.lexeme : "<EOF>");
    exit(1);
}

/* ---------- parse_unary ---------- */
static Node *parse_unary(void) {
    if (match(TT_SYMBOL, "!") || match(TT_SYMBOL, "~") ||
        match(TT_SYMBOL, "-") || match(TT_SYMBOL, "+")) {
        Token op = consume();
        Node *operand = parse_unary();
        Node *n = malloc(sizeof(Node));
        n->type = NODE_UNARY;
        n->data.unary.op = (strcmp(op.lexeme, "!") == 0) ? OP_NOT :
                           (strcmp(op.lexeme, "~") == 0) ? OP_BITNOT :
                           (strcmp(op.lexeme, "-") == 0) ? OP_NEG :
                           (strcmp(op.lexeme, "+") == 0) ? OP_NEG : OP_UNKNOWN;
        n->data.unary.operand = operand;
        return n;
    }
    if (match(TT_IDENTIFIER, "not")) {
        consume();
        Node *operand = parse_unary();
        Node *n = malloc(sizeof(Node));
        n->type = NODE_UNARY;
        n->data.unary.op = OP_NOT;
        n->data.unary.operand = operand;
        return n;
    }
    return parse_primary();
}

static Node *parse_exponent(void) {
    Node *node = parse_unary();
    if (match(TT_SYMBOL, "**")) {
        consume();
        Node *right = parse_exponent();
        Node *n = malloc(sizeof(Node));
        n->type = NODE_BINARY;
        n->data.bin.left  = node;
        n->data.bin.right = right;
        n->data.bin.op    = OP_POW;
        return n;
    }
    return node;
}

static Node *parse_mul_div_mod(void) {
    Node *node = parse_exponent();
    while (match(TT_SYMBOL, "*") || match(TT_SYMBOL, "/") ||
           match(TT_SYMBOL, "%")) {
        Token op = consume();
        Node *right = parse_exponent();
        Node *n = malloc(sizeof(Node));
        n->type = NODE_BINARY;
        n->data.bin.left  = node;
        n->data.bin.right = right;
        n->data.bin.op    = sym_to_binop(op.lexeme);
        node = n;
    }
    return node;
}

static Node *parse_add_sub(void) {
    Node *node = parse_mul_div_mod();
    while (match(TT_SYMBOL, "+") || match(TT_SYMBOL, "-")) {
        Token op = consume();
        Node *right = parse_mul_div_mod();
        Node *n = malloc(sizeof(Node));
        n->type = NODE_BINARY;
        n->data.bin.left  = node;
        n->data.bin.right = right;
        n->data.bin.op    = sym_to_binop(op.lexeme);
        node = n;
    }
    return node;
}

static Node *parse_shift(void) {
    Node *node = parse_add_sub();
    while (match(TT_SYMBOL, "<<") || match(TT_SYMBOL, ">>")) {
        Token op = consume();
        Node *right = parse_add_sub();
        Node *n = malloc(sizeof(Node));
        n->type = NODE_BINARY;
        n->data.bin.left  = node;
        n->data.bin.right = right;
        n->data.bin.op    = sym_to_binop(op.lexeme);
        node = n;
    }
    return node;
}

static Node *parse_relational(void) {
    Node *node = parse_shift();
    while (match(TT_SYMBOL, "<")  || match(TT_SYMBOL, "<=") ||
           match(TT_SYMBOL, ">")  || match(TT_SYMBOL, ">=")) {
        Token op = consume();
        Node *right = parse_shift();
        Node *n = malloc(sizeof(Node));
        n->type = NODE_BINARY;
        n->data.bin.left  = node;
        n->data.bin.right = right;
        n->data.bin.op    = sym_to_binop(op.lexeme);
        node = n;
    }
    return node;
}

static Node *parse_equality(void) {
    Node *node = parse_relational();
    while (match(TT_SYMBOL, "==") || match(TT_SYMBOL, "~=") ||
           match(TT_SYMBOL, "!=")) {
        Token op = consume();
        Node *right = parse_relational();
        Node *n = malloc(sizeof(Node));
        n->type = NODE_BINARY;
        n->data.bin.left  = node;
        n->data.bin.right = right;
        n->data.bin.op    = sym_to_binop(op.lexeme);
        node = n;
    }
    return node;
}

static Node *parse_bitwise_and(void) {
    Node *node = parse_equality();
    while (match(TT_SYMBOL, "&")) {
        consume();
        Node *right = parse_equality();
        Node *n = malloc(sizeof(Node));
        n->type = NODE_BINARY;
        n->data.bin.left  = node;
        n->data.bin.right = right;
        n->data.bin.op    = OP_BITAND;
        node = n;
    }
    return node;
}

static Node *parse_bitwise_xor(void) {
    Node *node = parse_bitwise_and();
    while (match(TT_SYMBOL, "^")) {
        consume();
        Node *right = parse_bitwise_and();
        Node *n = malloc(sizeof(Node));
        n->type = NODE_BINARY;
        n->data.bin.left  = node;
        n->data.bin.right = right;
        n->data.bin.op    = OP_BITXOR;
        node = n;
    }
    return node;
}

static Node *parse_bitwise_or(void) {
    Node *node = parse_bitwise_xor();
    while (match(TT_SYMBOL, "|")) {
        consume();
        Node *right = parse_bitwise_xor();
        Node *n = malloc(sizeof(Node));
        n->type = NODE_BINARY;
        n->data.bin.left  = node;
        n->data.bin.right = right;
        n->data.bin.op    = OP_BITOR;
        node = n;
    }
    return node;
}

static Node *parse_logical_and(void) {
    Node *node = parse_bitwise_or();
    while (match(TT_SYMBOL, "&&") || match(TT_IDENTIFIER, "and")) {
        if (match(TT_SYMBOL, "&&")) consume(); else consume();
        Node *right = parse_bitwise_or();
        Node *n = malloc(sizeof(Node));
        n->type = NODE_BINARY;
        n->data.bin.left  = node;
        n->data.bin.right = right;
        n->data.bin.op    = OP_LOGICAL_AND;
        node = n;
    }
    return node;
}

static Node *parse_logical_or(void) {
    Node *node = parse_logical_and();
    while (match(TT_SYMBOL, "||") || match(TT_IDENTIFIER, "or")) {
        if (match(TT_SYMBOL, "||")) consume(); else consume();
        Node *right = parse_logical_and();
        Node *n = malloc(sizeof(Node));
        n->type = NODE_BINARY;
        n->data.bin.left  = node;
        n->data.bin.right = right;
        n->data.bin.op    = OP_LOGICAL_OR;
        node = n;
    }
    return node;
}

static Node *parse_expression(void) {
    return parse_logical_or();
}

static Node *parse_assignment(void) {
    Token id = consume();

    if (!match(TT_SYMBOL, "=") && !is_assign_operator(peek().lexeme)) {
        Node *var = malloc(sizeof(Node));
        var->type = NODE_VAR;
        var->data.var_name = strdup(id.lexeme);
        return var;
    }

    Token opTok = consume();
    Node *right = parse_expression();

    Node *leftVar = malloc(sizeof(Node));
    leftVar->type = NODE_VAR;
    leftVar->data.var_name = strdup(id.lexeme);

    if (strcmp(opTok.lexeme, "=") == 0) {
        Node *assign = malloc(sizeof(Node));
        assign->type = NODE_BINARY;
        assign->data.bin.left  = leftVar;
        assign->data.bin.right = right;
        assign->data.bin.op    = OP_ASSIGN;
        return assign;
    }

    OpCode simpleOp = OP_UNKNOWN;
    if (strcmp(opTok.lexeme, "+=") == 0) simpleOp = OP_ADD;
    else if (strcmp(opTok.lexeme, "-=") == 0) simpleOp = OP_SUB;
    else if (strcmp(opTok.lexeme, "*=") == 0) simpleOp = OP_MUL;
    else if (strcmp(opTok.lexeme, "/=") == 0) simpleOp = OP_DIV;
    else if (strcmp(opTok.lexeme, "%=") == 0) simpleOp = OP_MOD;
    else if (strcmp(opTok.lexeme, "&=") == 0) simpleOp = OP_BITAND;
    else if (strcmp(opTok.lexeme, "|=") == 0) simpleOp = OP_BITOR;
    else if (strcmp(opTok.lexeme, "^=") == 0) simpleOp = OP_BITXOR;
    else if (strcmp(opTok.lexeme, "<<=") == 0) simpleOp = OP_SHL;
    else if (strcmp(opTok.lexeme, ">>=") == 0) simpleOp = OP_SHR;
    else if (strcmp(opTok.lexeme, "**=") == 0) simpleOp = OP_POW;
    else {
        fprintf(stderr, "Operador de atribuição desconhecido \"%s\"\n",
                opTok.lexeme);
        exit(1);
    }

    Node *leftCopy = malloc(sizeof(Node));
    leftCopy->type = NODE_VAR;
    leftCopy->data.var_name = strdup(id.lexeme);

    Node *bin = malloc(sizeof(Node));
    bin->type = NODE_BINARY;
    bin->data.bin.left  = leftCopy;
    bin->data.bin.right = right;
    bin->data.bin.op    = simpleOp;

    Node *assign = malloc(sizeof(Node));
    assign->type = NODE_BINARY;
    assign->data.bin.left  = leftVar;
    assign->data.bin.right = bin;
    assign->data.bin.op    = OP_ASSIGN;
    return assign;
}

/* ---------- bloco ---------- */
static Node *parse_block(void) {
    if (!match(TT_SYMBOL, "{")) {
        fprintf(stderr, "Expected '{' to start a block\n");
        exit(1);
    }
    consume();

    size_t cap = 8, len = 0;
    Node **stmts = calloc(cap, sizeof(Node *));

    while (!match(TT_SYMBOL, "}")) {
        if (match(TT_EOF, NULL)) {
            fprintf(stderr, "Unexpected EOF inside block\n");
            exit(1);
        }
        skip_separators();
        if (match(TT_SYMBOL, "}")) break;

        if (len == cap) {
            cap *= 2;
            stmts = realloc(stmts, cap * sizeof(Node *));
        }
        stmts[len++] = parse_statement();
        skip_separators();
    }
    consume();

    if (len == cap) stmts = realloc(stmts, (cap + 1) * sizeof(Node *));
    stmts[len] = NULL;

    Node *blk = malloc(sizeof(Node));
    blk->type = NODE_BLOCK;
    blk->data.block.stmts = stmts;
    return blk;
}

/* ---------- while/loop ---------- */
static Node *parse_while(void) {
    consume();

    Node *cond = NULL;
    if (match(TT_SYMBOL, "(")) {
        consume();
        cond = parse_expression();
        if (!match(TT_SYMBOL, ")")) {
            fprintf(stderr, "Expected ')' after while condition\n");
            exit(1);
        }
        consume();
    } else {
        cond = parse_expression();
    }

    Node *body = NULL;
    if (match(TT_SYMBOL, "{")) {
        body = parse_block();
    } else {
        body = parse_statement();
    }

    Node *w = malloc(sizeof(Node));
    w->type = NODE_WHILE;
    w->data.while_node.cond = cond;
    w->data.while_node.body = body;
    return w;
}

/* ---------- class decl (kept) ---------- */
static Node *parse_class_decl(void) {
    consume();

    Token className = consume();
    if (className.type != TT_IDENTIFIER) {
        fprintf(stderr, "Expected class name after 'class'\n");
        exit(1);
    }

    if (!match(TT_SYMBOL, "{")) {
        fprintf(stderr, "Expected '{' after class name\n");
        exit(1);
    }
    consume();

    while (!match(TT_SYMBOL, "}")) {
        if (match(TT_EOF, NULL)) {
            fprintf(stderr, "Unexpected EOF inside class body\n");
            exit(1);
        }
        advance();
    }
    consume();

    Node *cl = malloc(sizeof(Node));
    cl->type = NODE_CLASS_DECL;
    cl->data.class_decl.class_name = strdup(className.lexeme);
    cl->data.class_decl.body = NULL;
    return cl;
}

/* ---------- if statement ---------- */
static Node *parse_if(void) {
    consume();

    /* condição (opcional os parênteses) */
    int has_parens = 0;
    if (match(TT_SYMBOL, "(")) {
        consume();
        has_parens = 1;
    }

    Node *cond = parse_expression();

    if (has_parens) {
        if (!match(TT_SYMBOL, ")")) {
            fprintf(stderr, "Expected ')' after if condition\n");
            exit(1);
        }
        consume();
    }

    /* corpo do then */
    Node *then_body;
    if (match(TT_SYMBOL, "{")) {
        then_body = parse_block();
    } else {
        then_body = parse_statement();
    }

    /* else opcional */
    Node *else_body = NULL;
    if (match(TT_IDENTIFIER, "else")) {
        consume();
        if (match(TT_SYMBOL, "{")) {
            else_body = parse_block();
        } else if (match(TT_IDENTIFIER, "if")) {
            else_body = parse_if();  /* else if */
        } else {
            else_body = parse_statement();
        }
    }

    Node *if_node = malloc(sizeof(Node));
    if_node->type = NODE_IF;
    if_node->data.if_node.cond = cond;
    if_node->data.if_node.then_body = then_body;
    if_node->data.if_node.else_body = else_body;
    return if_node;
}

/* ---------- function: 'fuktion name(a,b) { ... }' ---------- */
static Node *parse_function_decl(void) {
    consume();

    Token nameTok = consume();
    if (nameTok.type != TT_IDENTIFIER) {
        fprintf(stderr, "Expected function name after 'fuktion'\n");
        exit(1);
    }

    if (!match(TT_SYMBOL, "(")) {
        fprintf(stderr, "Expected '(' after function name\n");
        exit(1);
    }
    consume(); /* '(' */

    char **params = NULL;
    size_t cap = 4, nparams = 0;
    params = calloc(cap, sizeof(char *));
    if (!match(TT_SYMBOL, ")")) {
        while (1) {
            Token p = consume();
            if (p.type != TT_IDENTIFIER) {
                fprintf(stderr, "Expected parameter name in function declaration\n");
                exit(1);
            }
            if (nparams == cap) { cap *= 2; params = realloc(params, cap * sizeof(char *)); }
            params[nparams++] = strdup(p.lexeme);

            if (match(TT_SYMBOL, ",")) { consume(); continue; }
            break;
        }
    }

    if (!match(TT_SYMBOL, ")")) {
        fprintf(stderr, "Expected ')' after parameter list\n");
        exit(1);
    }
    consume(); /* ')' */

    Node *body = NULL;
    if (match(TT_SYMBOL, "{")) {
        body = parse_block();
    } else {
        fprintf(stderr, "Expected '{' for function body\n");
        exit(1);
    }

    Node *fn = malloc(sizeof(Node));
    fn->type = NODE_FUNC_DECL;
    fn->data.func_decl.name = strdup(nameTok.lexeme);
    fn->data.func_decl.params = params;
    fn->data.func_decl.nparams = nparams;
    fn->data.func_decl.body = body;
    return fn;
}

/* ---------- return statement ---------- */
static Node *parse_return_stmt(void) {
    consume();
    Node *expr = NULL;
    if (!match(TT_SYMBOL, ";") && !match(TT_SYMBOL, "}")) {
        expr = parse_expression();
    }
    Node *ret = malloc(sizeof(Node));
    ret->type = NODE_RETURN;
    ret->data.return_node.expr = expr;
    return ret;
}

static Node *parse_statement(void) {
    skip_separators();

    if (match(TT_SYMBOL, "{"))
        return parse_block();

    if (match(TT_IDENTIFIER, "class"))
        return parse_class_decl();

    if (match(TT_IDENTIFIER, "while"))
        return parse_while();

    if (match(TT_IDENTIFIER, "if"))
        return parse_if();

    if (match(TT_IDENTIFIER, "fuktion"))
        return parse_function_decl();

    if (match(TT_IDENTIFIER, "return"))
        return parse_return_stmt();

    if (peek().type == TT_IDENTIFIER) {
        Token look = peek_next();
        if (look.type == TT_SYMBOL && is_assign_operator(look.lexeme))
            return parse_assignment();
    }

    return parse_expression();
}

/* ---------- programa ;-; ---------- */
static Node **parse_program(void) {
    size_t cap = 32, len = 0;
    Node **stmts = calloc(cap, sizeof(Node *));

    while (!match(TT_EOF, NULL)) {
        skip_separators();
        if (match(TT_EOF, NULL)) break;

        if (len == cap) { cap *= 2; stmts = realloc(stmts, cap * sizeof(Node *)); }
        stmts[len++] = parse_statement();
        skip_separators();
    }

    if (len == cap) stmts = realloc(stmts, (cap + 1) * sizeof(Node *));
    stmts[len] = NULL;
    return stmts;
}

/*=====================================================================
 * 4.   VM:onde vai roda esse treco kk
 *===================================================================== */

typedef struct {
    double *stack;
    size_t size;
    size_t capacity;
} Stack;

static void stack_init(Stack *s) {
    s->size = 0;
    s->capacity = 32;
    s->stack = calloc(s->capacity, sizeof(double));
}
static void stack_push(Stack *s, double v) {
    if (s->size == s->capacity) {
        s->capacity *= 2;
        s->stack = realloc(s->stack, s->capacity * sizeof(double));
    }
    s->stack[s->size++] = v;
}
static double stack_pop(Stack *s) {
    if (s->size == 0) {
        fprintf(stderr, "Stack underflow\n");
        exit(1);
    }
    return s->stack[--s->size];
}

typedef struct VarPair {
    char *name;
    double value;
    struct VarPair *next;
} VarPair;

typedef struct Env {
    VarPair *head;
    struct Env *parent;
} Env;

static void env_init(Env *e, Env *parent) { e->head = NULL; e->parent = parent; }

static void env_set(Env *e, const char *name, double val) {
    VarPair *p = e->head;
    while (p && strcmp(p->name, name) != 0) p = p->next;
    if (p) { p->value = val; return; }
    p = malloc(sizeof(VarPair));
    p->name = strdup(name);
    p->value = val;
    p->next = e->head;
    e->head = p;
}

static double env_get(Env *e, const char *name) {
    Env *cur = e;
    while (cur) {
        VarPair *p = cur->head;
        while (p && strcmp(p->name, name) != 0) p = p->next;
        if (p) return p->value;
        cur = cur->parent;
    }
    fprintf(stderr, "Runtime error: undefined variable '%s'\n", name);
    exit(1);
}

/*=====================================================================
 * 5.   Function table (para funções definidas pelo user)
 *===================================================================== */

typedef struct FuncEntry {
    char *name;
    char **params;
    size_t nparams;
    Node *body;
    /* memory limit support */
    long memlimit_bytes; /* bytes limit, -1 = not set */
    int memlimit_mode;   /* 1 = clear+restart, 0 = FIFO-evict */
    int memlimit_set;    /* 0 = no limit, 1 = set */
    struct FuncEntry *next;
} FuncEntry;

static FuncEntry *func_table = NULL;

/* helper: parse unit strings (kb, mb, gb) -> multiplier */
static long unit_multiplier_from_string(const char *u) {
    if (!u) return 1;
    if (strcasecmp(u, "kb") == 0) return 1024L;
    if (strcasecmp(u, "mb") == 0) return 1024L * 1024L;
    if (strcasecmp(u, "gb") == 0) return 1024L * 1024L * 1024L;
    return 1;
}

/* compute approximate memory used */
static size_t compute_env_mem(Env *e) {
    if (!e) return 0;
    size_t total = 0;
    VarPair *p = e->head;
    while (p) {
        total += strlen(p->name) + 1; /* name bytes */
        total += sizeof(double);      /* value */
        p = p->next;
    }
    return total;
}

static int evict_oldest_var(Env *e) {
    if (!e || !e->head) return 0;
    VarPair *p = e->head;
    VarPair *prev = NULL;
    while (p->next) { prev = p; p = p->next; }
    /* p e o mais antigo */
    if (prev) {
        prev->next = NULL;
        free(p->name);
        free(p);
    } else {
        free(p->name);
        free(p);
        e->head = NULL;
    }
    return 1;
}

static void clear_env_vars(Env *e) {
    if (!e) return;
    VarPair *p = e->head;
    while (p) {
        VarPair *n = p->next;
        free(p->name);
        free(p);
        p = n;
    }
    e->head = NULL;
}

static void scan_memlimit_in_body(Node *body, long *out_bytes, int *out_mode, int *out_set) {
    *out_set = 0;
    *out_bytes = -1;
    *out_mode = -1;
    if (!body || body->type != NODE_BLOCK) return;
    for (Node **p = body->data.block.stmts; *p != NULL; ++p) {
        Node *stmt = *p;
        if (!stmt) continue;

        if (stmt->type == NODE_CALL && stmt->data.call.func_name) {
            if (strcmp(stmt->data.call.func_name, "memlimit") == 0) {

                long bytes = -1;
                int mode = -1;
                long multiplier = 1;
                if (stmt->data.call.nargs >= 1) {
                    Node *n0 = stmt->data.call.args[0];
                    if (n0 && n0->type == NODE_NUMBER) {
                        bytes = (long)(n0->data.num);
                    }
                }
                if (stmt->data.call.nargs >= 2) {
                    Node *n1 = stmt->data.call.args[1];
                    if (n1 && n1->type == NODE_STRING) {
                        multiplier = unit_multiplier_from_string(n1->data.str);
                    }
                }
                if (stmt->data.call.nargs >= 3) {
                    Node *n2 = stmt->data.call.args[2];
                    if (n2 && n2->type == NODE_NUMBER) {
                        mode = (int)(n2->data.num);
                    }
                }
                if (bytes >= 0 && mode != -1) {
                    *out_set = 1;
                    *out_bytes = bytes * multiplier;
                    *out_mode = mode ? 1 : 0;
                    /* use first occurrence */
                    return;
                }
            }
        }

        if (stmt->type == NODE_BLOCK) {
            scan_memlimit_in_body(stmt, out_bytes, out_mode, out_set);
            if (*out_set) return;
        }
    }
}

static void register_function(Node *fn_node) {
    if (!fn_node || fn_node->type != NODE_FUNC_DECL) return;
    /* sempre sobrescrever  */
    FuncEntry **pp = &func_table;
    while (*pp) {
        if (strcmp((*pp)->name, fn_node->data.func_decl.name) == 0) {
            FuncEntry *rem = *pp;
            *pp = rem->next;

            free(rem->name);
            for (size_t i = 0; i < rem->nparams; ++i) free(rem->params[i]);
            free(rem->params);
            free(rem);
            break;
        }
        pp = &(*pp)->next;
    }

    FuncEntry *fe = malloc(sizeof(FuncEntry));
    fe->name = strdup(fn_node->data.func_decl.name);
    fe->nparams = fn_node->data.func_decl.nparams;
    fe->params = calloc(fe->nparams, sizeof(char *));
    for (size_t i = 0; i < fe->nparams; ++i) fe->params[i] = strdup(fn_node->data.func_decl.params[i]);
    fe->body = fn_node->data.func_decl.body;
    /* os volor padrão ok */
    fe->memlimit_set = 0;
    fe->memlimit_bytes = -1;
    fe->memlimit_mode = 0;

    {
        long bytes = -1; int mode = -1; int set = 0;
        scan_memlimit_in_body(fe->body, &bytes, &mode, &set);
        if (set) {
            fe->memlimit_set = 1;
            fe->memlimit_bytes = bytes;
            fe->memlimit_mode = mode;

            if (fe->body && fe->body->type == NODE_BLOCK) {
                Node **stmts = fe->body->data.block.stmts;
                size_t read = 0, write = 0;
                while (stmts[read]) {
                    Node *stmt = stmts[read];
                    int is_memlimit_call = 0;
                    if (stmt && stmt->type == NODE_CALL && stmt->data.call.func_name) {
                        if (strcmp(stmt->data.call.func_name, "memlimit") == 0) {
                            is_memlimit_call = 1;
                        }
                    }
                    if (is_memlimit_call) {
                        free_node(stmt);
                    } else {
                        stmts[write++] = stmts[read];
                    }
                    read++;
                }
                stmts[write] = NULL;
            }
        }
    }

    fe->next = func_table;
    func_table = fe;
}

static FuncEntry *find_function(const char *name) {
    for (FuncEntry *p = func_table; p; p = p->next)
        if (strcmp(p->name, name) == 0) return p;
    return NULL;
}

/*=====================================================================
 * 6.   Network 
 *===================================================================== */

/* chamada  */
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    char **response_data = (char **)userp;
    
    // verificar se response_data é NULL antes de usar strlen
    size_t current_len = (*response_data == NULL) ? 0 : strlen(*response_data);
    
    *response_data = realloc(*response_data, current_len + realsize + 1);
    if (*response_data == NULL) {
        fprintf(stderr, "Not enough memory to store response data\n");
        return 0;
    }
    
    if (current_len == 0) {
        (*response_data)[0] = '\0';
    }
    
    strncat(*response_data, (char *)contents, realsize);
    return realsize;
}

/*=====================================================================
 * 7.   Execution
 *===================================================================== */

static int returning_flag = 0;
static double returning_value = 0.0;

static void exec_node(Node *node, Stack *stack, Env *env);
static void exec_expr(Node *node, Stack *stack, Env *env) {
    if (!node) return;
    if (returning_flag) return;

    switch (node->type) {
        case NODE_NUMBER:
            stack_push(stack, node->data.num);
            break;

        case NODE_VAR:
            stack_push(stack, env_get(env, node->data.var_name));
            break;

        case NODE_STRING:
            fprintf(stderr, "String literals not supported outside of 'print'.\n");
            exit(1);
            break;

        case NODE_UNARY: {
            exec_expr(node->data.unary.operand, stack, env);
            double v = stack_pop(stack);
            double res = 0.0;
            switch (node->data.unary.op) {
                case OP_NEG:   res = -v; break;
                case OP_NOT:   res = (v == 0.0) ? 1.0 : 0.0; break;
                case OP_BITNOT:
                    res = (double)~(int64_t)v;
                    break;
                default:
                    fprintf(stderr, "Unknown unary operator (code %d)\n", node->data.unary.op);
                    exit(1);
            }
            stack_push(stack, res);
            break;
        }

        case NODE_BINARY: {
            OpCode op = node->data.bin.op;
            if (op == OP_ASSIGN) {
                if (node->data.bin.left->type != NODE_VAR) {
                    fprintf(stderr, "Runtime error: left side of '=' must be a variable\n");
                    exit(1);
                }
                exec_expr(node->data.bin.right, stack, env);
                double val = stack_pop(stack);
                env_set(env, node->data.bin.left->data.var_name, val);
                stack_push(stack, val);
                break;
            }
            exec_expr(node->data.bin.left, stack, env);
            exec_expr(node->data.bin.right, stack, env);
            double r = stack_pop(stack);
            double l = stack_pop(stack);
            double res = 0.0;
            switch (op) {
                case OP_ADD:   res = l + r; break;
                case OP_SUB:   res = l - r; break;
                case OP_MUL:   res = l * r; break;
                case OP_DIV:   res = l / r; break;
                case OP_MOD:   res = fmod(l, r); break;
                case OP_POW:   res = pow(l, r); break;
                case OP_LT:    res = (l <  r) ? 1.0 : 0.0; break;
                case OP_LE:    res = (l <= r) ? 1.0 : 0.0; break;
                case OP_GT:    res = (l >  r) ? 1.0 : 0.0; break;
                case OP_GE:    res = (l >= r) ? 1.0 : 0.0; break;
                case OP_EQ:    res = (l == r) ? 1.0 : 0.0; break;
                case OP_NE:    res = (l != r) ? 1.0 : 0.0; break;
                case OP_BITAND:  res = (double)((int64_t)l & (int64_t)r); break;
                case OP_BITOR:   res = (double)((int64_t)l | (int64_t)r); break;
                case OP_BITXOR:  res = (double)((int64_t)l ^ (int64_t)r); break;
                case OP_SHL:     res = (double)((int64_t)l << (int64_t)r); break;
                case OP_SHR:     res = (double)((int64_t)l >> (int64_t)r); break;
                case OP_LOGICAL_AND:
                    res = (l != 0.0 && r != 0.0) ? 1.0 : 0.0; break;
                case OP_LOGICAL_OR:
                    res = (l != 0.0 || r != 0.0) ? 1.0 : 0.0; break;
                default:
                    fprintf(stderr, "Runtime error: unknown binary operator code %d\n", op);
                    exit(1);
            }
            stack_push(stack, res);
            break;
        }

        case NODE_CALL: {
            if (strcmp(node->data.call.func_name, "print") == 0) {
                for (size_t i = 0; i < node->data.call.nargs; ++i) {
                    Node *arg = node->data.call.args[i];
                    if (arg->type == NODE_STRING) {
                        printf("%s ", arg->data.str);
                    } else {
                        exec_expr(arg, stack, env);
                        double v = stack_pop(stack);
                        printf("%g ", v);
                    }
                }
                printf("\n");
                break;
            }

            if (strcmp(node->data.call.func_name, "graphics.init") == 0) {
                /* inicia o SDL2 + OpenGL se for pedido  */
                if (g_graphics_initialized) { stack_push(stack, 1); break; }
                if (SDL_Init(SDL_INIT_VIDEO) != 0) {
                    fprintf(stderr, "graphics.init: SDL_Init failed: %s\n", SDL_GetError());
                    stack_push(stack, 0);
                    break;
                }
                /*simple GL attributes */
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
                SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
                SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
                g_sdl_window = SDL_CreateWindow("KoalCode", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                               g_window_width, g_window_height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
                if (!g_sdl_window) {
                    fprintf(stderr, "graphics.init: SDL_CreateWindow failed: %s\n", SDL_GetError());
                    SDL_Quit();
                    stack_push(stack, 0);
                    break;
                }
                g_gl_context = SDL_GL_CreateContext(g_sdl_window);
                if (!g_gl_context) {
                    fprintf(stderr, "graphics.init: SDL_GL_CreateContext failed: %s\n", SDL_GetError());
                    SDL_DestroyWindow(g_sdl_window);
                    g_sdl_window = NULL;
                    SDL_Quit();
                    stack_push(stack, 0);
                    break;
                }
                SDL_GL_SetSwapInterval(1); /* vsync if available */
                glViewport(0, 0, g_window_width, g_window_height);

                glMatrixMode(GL_PROJECTION);
                glLoadIdentity();
                {
                    /*(aqui vc entende o pq do M_PI nos includekkkk) compute aspect ratio and build a frustum for a ~60deg FOV */
                    double aspect = (double)g_window_width / (double)g_window_height;
                    double fov_deg = 60.0;
                    double fov_rad = fov_deg * M_PI / 180.0;
                    double near = 0.1;
                    double top = near * tan(fov_rad * 0.5);
                    double right = top * aspect;
                    glFrustum(-right, right, -top, top, near, 100.0);
                }
                glMatrixMode(GL_MODELVIEW);

                glEnable(GL_DEPTH_TEST);
                g_graphics_initialized = 1;
                stack_push(stack, 1);
                break;
            }

            if (strcmp(node->data.call.func_name, "graphics.quit") == 0) {
                if (!g_graphics_initialized) { stack_push(stack, 0); break; }
                SDL_GL_DeleteContext(g_gl_context);
                SDL_DestroyWindow(g_sdl_window);
                g_gl_context = NULL;
                g_sdl_window = NULL;
                SDL_Quit();
                g_graphics_initialized = 0;
                stack_push(stack, 1);
                break;
            }

            if (strcmp(node->data.call.func_name, "graphics.clear") == 0) {
                if (!g_graphics_initialized) { fprintf(stderr, "graphics.clear: not initialized\n"); stack_push(stack, 0); break; }
                float r = 0.0f, g = 0.0f, b = 0.0f;
                if (node->data.call.nargs >= 1) { exec_expr(node->data.call.args[0], stack, env); r = (float)stack_pop(stack); }
                if (node->data.call.nargs >= 2) { exec_expr(node->data.call.args[1], stack, env); g = (float)stack_pop(stack); }
                if (node->data.call.nargs >= 3) { exec_expr(node->data.call.args[2], stack, env); b = (float)stack_pop(stack); }
                glClearColor(r, g, b, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                stack_push(stack, 1);
                break;
            }

            if (strcmp(node->data.call.func_name, "graphics.swap") == 0) {
                if (!g_graphics_initialized) { fprintf(stderr, "graphics.swap: not initialized\n"); stack_push(stack, 0); break; }
                SDL_GL_SwapWindow(g_sdl_window);
                stack_push(stack, 1);
                break;
            }

            if (strcmp(node->data.call.func_name, "graphics.color") == 0) {
                if (!g_graphics_initialized) { fprintf(stderr, "graphics.color: not initialized\n"); stack_push(stack, 0); break; }
                float r = 1.0f, g = 1.0f, b = 1.0f;
                if (node->data.call.nargs >= 1) { exec_expr(node->data.call.args[0], stack, env); r = (float)stack_pop(stack); }
                if (node->data.call.nargs >= 2) { exec_expr(node->data.call.args[1], stack, env); g = (float)stack_pop(stack); }
                if (node->data.call.nargs >= 3) { exec_expr(node->data.call.args[2], stack, env); b = (float)stack_pop(stack); }
                glColor3f(r, g, b);
                stack_push(stack, 1);
                break;
            }

            if (strcmp(node->data.call.func_name, "graphics.triangle") == 0) {
                if (!g_graphics_initialized) { fprintf(stderr, "graphics.triangle: not initialized\n"); stack_push(stack, 0); break; }
                double vals[9];
                for (size_t i = 0; i < 9; ++i) {
                    if (i < node->data.call.nargs) {
                        exec_expr(node->data.call.args[i], stack, env);
                        vals[i] = stack_pop(stack);
                    } else vals[i] = 0.0;
                }
                glBegin(GL_TRIANGLES);
                    glVertex3f((GLfloat)vals[0], (GLfloat)vals[1], (GLfloat)vals[2]);
                    glVertex3f((GLfloat)vals[3], (GLfloat)vals[4], (GLfloat)vals[5]);
                    glVertex3f((GLfloat)vals[6], (GLfloat)vals[7], (GLfloat)vals[8]);
                glEnd();
                stack_push(stack, 1);
                break;
            }

            if (strcmp(node->data.call.func_name, "graphics.translate") == 0) {
                if (!g_graphics_initialized) { fprintf(stderr, "graphics.translate: not initialized\n"); stack_push(stack, 0); break; }
                float x = 0.0f, y = 0.0f, z = 0.0f;
                if (node->data.call.nargs >= 1) { exec_expr(node->data.call.args[0], stack, env); x = (float)stack_pop(stack); }
                if (node->data.call.nargs >= 2) { exec_expr(node->data.call.args[1], stack, env); y = (float)stack_pop(stack); }
                if (node->data.call.nargs >= 3) { exec_expr(node->data.call.args[2], stack, env); z = (float)stack_pop(stack); }
                glTranslatef(x, y, z);
                stack_push(stack, 1);
                break;
            }

            if (strcmp(node->data.call.func_name, "graphics.rotate") == 0) {
                if (!g_graphics_initialized) { fprintf(stderr, "graphics.rotate: not initialized\n"); stack_push(stack, 0); break; }
                float ang = 0.0f, x = 0.0f, y = 0.0f, z = 1.0f;
                if (node->data.call.nargs >= 1) { exec_expr(node->data.call.args[0], stack, env); ang = (float)stack_pop(stack); }
                if (node->data.call.nargs >= 2) { exec_expr(node->data.call.args[1], stack, env); x = (float)stack_pop(stack); }
                if (node->data.call.nargs >= 3) { exec_expr(node->data.call.args[2], stack, env); y = (float)stack_pop(stack); }
                if (node->data.call.nargs >= 4) { exec_expr(node->data.call.args[3], stack, env); z = (float)stack_pop(stack); }
                glRotatef(ang, x, y, z);
                stack_push(stack, 1);
                break;
            }

            if (strcmp(node->data.call.func_name, "graphics.loadmatrix") == 0) {
                if (!g_graphics_initialized) { fprintf(stderr, "graphics.loadmatrix: not initialized\n"); stack_push(stack, 0); break; }
                glLoadIdentity();
                stack_push(stack, 1);
                break;
            }

            if (strcmp(node->data.call.func_name, "graphics.events") == 0) {
                if (!g_graphics_initialized) { fprintf(stderr, "graphics.events: not initialized\n"); stack_push(stack, 0); break; }
                int count = 0;
                SDL_Event ev;
                while (SDL_PollEvent(&ev)) {
                    count++;
                    if (ev.type == SDL_QUIT) {
                        stack_push(stack, -1);
                        goto _after_graphics_events;
                    }
                }
                stack_push(stack, count);
                _after_graphics_events:
                break;
            }

            /* ========== NETWORK FUNCTIONS ========== */
            
            if (strcmp(node->data.call.func_name, "network.init") == 0) {
                if (g_network_initialized) { stack_push(stack, 1); break; }
                if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
                    fprintf(stderr, "network.init: curl_global_init failed\n");
                    stack_push(stack, 0);
                    break;
                }
                g_curl_handle = curl_easy_init();
                if (!g_curl_handle) {
                    fprintf(stderr, "network.init: curl_easy_init failed\n");
                    curl_global_cleanup();
                    stack_push(stack, 0);
                    break;
                }
                g_network_initialized = 1;
                stack_push(stack, 1);
                break;
            }

            /* Limpeza do subsystema */
            if (strcmp(node->data.call.func_name, "network.quit") == 0) {
                if (!g_network_initialized) { stack_push(stack, 0); break; }
                if (g_curl_handle) {
                    curl_easy_cleanup(g_curl_handle);
                    g_curl_handle = NULL;
                }
                curl_global_cleanup();
                g_network_initialized = 0;
                stack_push(stack, 1);
                break;
            }

            /* HTTP GET request */
            if (strcmp(node->data.call.func_name, "http.get") == 0) {
                if (!g_network_initialized) { fprintf(stderr, "http.get: network not initialized\n"); stack_push(stack, 0); break; }
                if (node->data.call.nargs < 1) { fprintf(stderr, "http.get: URL required\n"); stack_push(stack, 0); break; }
                
                char *url = NULL;
                if (node->data.call.args[0]->type == NODE_STRING) {
                    url = strdup(node->data.call.args[0]->data.str);
                } else {
                    fprintf(stderr, "http.get: URL must be a string\n");
                    stack_push(stack, 0);
                    break;
                }
                
                CURLcode res;
                long response_code;
                char *response_data = malloc(1);
                response_data[0] = '\0';
                size_t response_size = 0;
                
                curl_easy_setopt(g_curl_handle, CURLOPT_URL, url);
                curl_easy_setopt(g_curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
                curl_easy_setopt(g_curl_handle, CURLOPT_WRITEDATA, &response_data);
                curl_easy_setopt(g_curl_handle, CURLOPT_WRITEHEADER, &response_size);
                curl_easy_setopt(g_curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_setopt(g_curl_handle, CURLOPT_TIMEOUT, 30L);
                
                res = curl_easy_perform(g_curl_handle);
                curl_easy_getinfo(g_curl_handle, CURLINFO_RESPONSE_CODE, &response_code);
                
                if (res == CURLE_OK) {
                    printf("HTTP GET Response (%ld): %s\n", response_code, response_data);
                    stack_push(stack, (double)response_code);
                } else {
                    fprintf(stderr, "http.get failed: %s\n", curl_easy_strerror(res));
                    stack_push(stack, 0);
                }
                
                free(url);
                free(response_data);
                break;
            }

            /* HTTP POST request */
            if (strcmp(node->data.call.func_name, "http.post") == 0) {
                if (!g_network_initialized) { fprintf(stderr, "http.post: network not initialized\n"); stack_push(stack, 0); break; }
                if (node->data.call.nargs < 2) { fprintf(stderr, "http.post: URL and data required\n"); stack_push(stack, 0); break; }
                
                char *url = NULL, *data = NULL;
                if (node->data.call.args[0]->type == NODE_STRING) {
                    url = strdup(node->data.call.args[0]->data.str);
                } else {
                    fprintf(stderr, "http.post: URL must be a string\n");
                    stack_push(stack, 0);
                    break;
                }
                
                if (node->data.call.args[1]->type == NODE_STRING) {
                    data = strdup(node->data.call.args[1]->data.str);
                } else {
                    fprintf(stderr, "http.post: data must be a string\n");
                    free(url);
                    stack_push(stack, 0);
                    break;
                }
                
                CURLcode res;
                long response_code;
                char *response_data = malloc(1);
                response_data[0] = '\0';
                size_t response_size = 0;
                
                curl_easy_setopt(g_curl_handle, CURLOPT_URL, url);
                curl_easy_setopt(g_curl_handle, CURLOPT_POSTFIELDS, data);
                curl_easy_setopt(g_curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
                curl_easy_setopt(g_curl_handle, CURLOPT_WRITEDATA, &response_data);
                curl_easy_setopt(g_curl_handle, CURLOPT_WRITEHEADER, &response_size);
                curl_easy_setopt(g_curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_setopt(g_curl_handle, CURLOPT_TIMEOUT, 30L);
                
                res = curl_easy_perform(g_curl_handle);
                curl_easy_getinfo(g_curl_handle, CURLINFO_RESPONSE_CODE, &response_code);
                
                if (res == CURLE_OK) {
                    printf("HTTP POST Response (%ld): %s\n", response_code, response_data);
                    stack_push(stack, (double)response_code);
                } else {
                    fprintf(stderr, "http.post failed: %s\n", curl_easy_strerror(res));
                    stack_push(stack, 0);
                }
                
                free(url);
                free(data);
                free(response_data);
                break;
            }

            /* Socket functions */
            if (strcmp(node->data.call.func_name, "socket.connect") == 0) {
                if (node->data.call.nargs < 2) { fprintf(stderr, "socket.connect: host and port required\n"); stack_push(stack, 0); break; }
                
                char *host = NULL;
                int port = 0;
                
                if (node->data.call.args[0]->type == NODE_STRING) {
                    host = strdup(node->data.call.args[0]->data.str);
                } else {
                    fprintf(stderr, "socket.connect: host must be a string\n");
                    stack_push(stack, 0);
                    break;
                }
                
                if (node->data.call.args[1]->type == NODE_NUMBER) {
                    port = (int)node->data.call.args[1]->data.num;
                } else {
                    fprintf(stderr, "socket.connect: port must be a number\n");
                    free(host);
                    stack_push(stack, 0);
                    break;
                }
                
                int sock = socket(AF_INET, SOCK_STREAM, 0);
                if (sock < 0) {
                    fprintf(stderr, "socket.connect: socket creation failed\n");
                    free(host);
                    stack_push(stack, 0);
                    break;
                }
                
                struct sockaddr_in server_addr;
                server_addr.sin_family = AF_INET;
                server_addr.sin_port = htons(port);
                
                if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0) {
                    fprintf(stderr, "socket.connect: invalid address\n");
                    close(sock);
                    free(host);
                    stack_push(stack, 0);
                    break;
                }
                
                if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                    fprintf(stderr, "socket.connect: connection failed\n");
                    close(sock);
                    free(host);
                    stack_push(stack, 0);
                    break;
                }
                
                printf("Connected to %s:%d\n", host, port);
                free(host);
                stack_push(stack, (double)sock);
                break;
            }

            if (strcmp(node->data.call.func_name, "socket.send") == 0) {
                if (node->data.call.nargs < 2) { fprintf(stderr, "socket.send: socket and data required\n"); stack_push(stack, 0); break; }
                
                int sock = 0;
                char *data = NULL;
                
                if (node->data.call.args[0]->type == NODE_NUMBER) {
                    sock = (int)node->data.call.args[0]->data.num;
                } else {
                    fprintf(stderr, "socket.send: socket must be a number\n");
                    stack_push(stack, 0);
                    break;
                }
                
                if (node->data.call.args[1]->type == NODE_STRING) {
                    data = strdup(node->data.call.args[1]->data.str);
                } else {
                    fprintf(stderr, "socket.send: data must be a string\n");
                    stack_push(stack, 0);
                    break;
                }
                
                ssize_t bytes_sent = send(sock, data, strlen(data), 0);
                if (bytes_sent < 0) {
                    fprintf(stderr, "socket.send: send failed\n");
                    free(data);
                    stack_push(stack, 0);
                    break;
                }
                
                printf("Sent %zd bytes: %s\n", bytes_sent, data);
                free(data);
                stack_push(stack, (double)bytes_sent);
                break;
            }

            if (strcmp(node->data.call.func_name, "socket.recv") == 0) {
                if (node->data.call.nargs < 1) { fprintf(stderr, "socket.recv: socket required\n"); stack_push(stack, 0); break; }
                
                int sock = 0;
                int buffer_size = 1024;
                
                if (node->data.call.args[0]->type == NODE_NUMBER) {
                    sock = (int)node->data.call.args[0]->data.num;
                } else {
                    fprintf(stderr, "socket.recv: socket must be a number\n");
                    stack_push(stack, 0);
                    break;
                }
                
                if (node->data.call.nargs >= 2 && node->data.call.args[1]->type == NODE_NUMBER) {
                    buffer_size = (int)node->data.call.args[1]->data.num;
                }
                
                char *buffer = malloc(buffer_size);
                ssize_t bytes_received = recv(sock, buffer, buffer_size - 1, 0);
                
                if (bytes_received < 0) {
                    fprintf(stderr, "socket.recv: recv failed\n");
                    free(buffer);
                    stack_push(stack, 0);
                    break;
                }
                
                buffer[bytes_received] = '\0';
                printf("Received %zd bytes: %s\n", bytes_received, buffer);
                free(buffer);
                stack_push(stack, (double)bytes_received);
                break;
            }

            if (strcmp(node->data.call.func_name, "socket.close") == 0) {
                if (node->data.call.nargs < 1) { fprintf(stderr, "socket.close: socket required\n"); stack_push(stack, 0); break; }
                
                int sock = 0;
                if (node->data.call.args[0]->type == NODE_NUMBER) {
                    sock = (int)node->data.call.args[0]->data.num;
                } else {
                    fprintf(stderr, "socket.close: socket must be a number\n");
                    stack_push(stack, 0);
                    break;
                }
                
                if (close(sock) < 0) {
                    fprintf(stderr, "socket.close: close failed\n");
                    stack_push(stack, 0);
                    break;
                }
                
                printf("Socket %d closed\n", sock);
                stack_push(stack, 1);
                break;
            }

            /* Network utility functions */
            if (strcmp(node->data.call.func_name, "network.ping") == 0) {
                if (node->data.call.nargs < 1) { fprintf(stderr, "network.ping: host required\n"); stack_push(stack, 0); break; }
                
                char *host = NULL;
                if (node->data.call.args[0]->type == NODE_STRING) {
                    host = strdup(node->data.call.args[0]->data.str);
                } else {
                    fprintf(stderr, "network.ping: host must be a string\n");
                    stack_push(stack, 0);
                    break;
                }
                
                char command[256];
                snprintf(command, sizeof(command), "ping -c 1 %s > /dev/null 2>&1", host);
                int result = system(command);
                
                free(host);
                stack_push(stack, (result == 0) ? 1.0 : 0.0);
                break;
            }

            FuncEntry *fe = find_function(node->data.call.func_name);
            if (fe) {
                double *argvals = calloc(fe->nparams, sizeof(double));
                for (size_t i = 0; i < fe->nparams; ++i) {
                    if (i < node->data.call.nargs) {
                        exec_expr(node->data.call.args[i], stack, env);
                        argvals[i] = stack_pop(stack);
                    } else {
                        argvals[i] = 0.0;
                    }
                }
                Env local;
                env_init(&local, env);

                for (size_t i = 0; i < fe->nparams; ++i)
                    env_set(&local, fe->params[i], argvals[i]);

                int attempts = 0;
                const int max_attempts = 3;
                double retv = 0.0;
                while (1) {
                    returning_flag = 0;
                    returning_value = 0.0;
                    exec_node(fe->body, stack, &local);

                    if (returning_flag) retv = returning_value;
                    else retv = 0.0;

                    if (fe->memlimit_set) {
                        size_t used = compute_env_mem(&local);
                        if ((long)used > fe->memlimit_bytes) {
                            if (fe->memlimit_mode == 1) {
                                attempts++;
                                if (attempts > max_attempts) {
                                    fprintf(stderr, "Runtime error: memlimit exceeded after %d restarts in function '%s'\n", max_attempts, fe->name);
                                    exit(1);
                                }
                                clear_env_vars(&local);
                                continue;
                            } else { 
                                while ((long)used > fe->memlimit_bytes) {
                                    if (!evict_oldest_var(&local)) break;
                                    used = compute_env_mem(&local);
                                }
                            }
                        }
                    }

                    break;
                }


                returning_flag = 0;
                returning_value = 0.0;
                stack_push(stack, retv);

                free(argvals);

                clear_env_vars(&local);
                break;
            }


            fprintf(stderr, "Runtime error: unknown function '%s'\n", node->data.call.func_name);
            exit(1);
            break;
        }

        default:
            fprintf(stderr, "Runtime: node type desconhecido (%d)\n", node->type);
            exit(1);
    }
}


static void exec_node(Node *node, Stack *stack, Env *env) {
    if (!node) return;
    if (returning_flag) return;

    switch (node->type) {
        case NODE_BLOCK: {
            for (Node **p = node->data.block.stmts; *p != NULL; ++p) {
                if (returning_flag) break;
                exec_node(*p, stack, env);
            }
            break;
        }
        case NODE_WHILE: {
            while (1) {
                exec_expr(node->data.while_node.cond, stack, env);
                double c = stack_pop(stack);
                if (c == 0.0) break;
                exec_node(node->data.while_node.body, stack, env);
                if (returning_flag) break;
            }
            break;
        }
        case NODE_IF: {
            exec_expr(node->data.if_node.cond, stack, env);
            double cond_value = stack_pop(stack);
            if (cond_value != 0.0) {
                exec_node(node->data.if_node.then_body, stack, env);
            } else if (node->data.if_node.else_body) {
                exec_node(node->data.if_node.else_body, stack, env);
            }
            break;
        }
        case NODE_CLASS_DECL:
            fprintf(stderr, "Definição de classe '%s' ainda não implementada.\n",
                    node->data.class_decl.class_name);
            break;
        case NODE_FUNC_DECL:
            /* register function in global table */
            register_function(node);
            break;
        case NODE_RETURN: {
            if (node->data.return_node.expr) {
                exec_expr(node->data.return_node.expr, stack, env);
                double v = stack_pop(stack);
                returning_value = v;
            } else {
                returning_value = 0.0;
            }
            returning_flag = 1;
            break;
        }
        default:

            exec_expr(node, stack, env);

            if (stack->size > 0) stack_pop(stack);
            break;
    }
}

/*=====================================================================
 * 7.   Threading, textures, models, builtins
 *===================================================================== */

typedef struct { char *name; } ThreadData;
static void *thread_func(void *arg) {
    ThreadData *td = (ThreadData *)arg;
    printf("Thread %s entrou na função.\n", td->name);
    for (int i = 0; i < 3; ++i) {
        printf("Thread %s: contador %d\n", td->name, i);
    }
    printf("Thread %s terminou.\n", td->name);
    free(td->name);
    free(td);
    return NULL;
}
static void start_thread(const char *name) {
    pthread_t t;
    ThreadData *td = malloc(sizeof(ThreadData));
    td->name = strdup(name);
    if (pthread_create(&t, NULL, thread_func, td) != 0) {
        perror("pthread_create");
        exit(1);
    }
    pthread_detach(t);
}

static GLuint load_texture_from_file(const char *filename, int *width, int *height) {

    SDL_Surface *surface = IMG_Load(filename);
    if (!surface) { fprintf(stderr, "Erro ao carregar imagem: %s\n", IMG_GetError()); return 0; }
    *width = surface->w; *height = surface->h;
    GLuint texture_id; glGenTextures(1, &texture_id); glBindTexture(GL_TEXTURE_2D, texture_id);
    GLenum format = (surface->format->BytesPerPixel == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels);
    SDL_FreeSurface(surface);
    return texture_id;
}

static int load_obj_model(const char *filename, Model3D *model) {

    (void)filename; (void)model;
    return 0;
}

/*=====================================================================
 * 8.   Freeing nodes, main loop
 *===================================================================== */

static void free_node(Node *node) {
    if (!node) return;
    switch (node->type) {
        case NODE_NUMBER: break;
        case NODE_VAR: free(node->data.var_name); break;
        case NODE_STRING: free(node->data.str); break;
        case NODE_CALL:
            free(node->data.call.func_name);
            for (size_t i = 0; i < node->data.call.nargs; ++i) free_node(node->data.call.args[i]);
            free(node->data.call.args);
            break;
        case NODE_CLASS_DECL:
            free(node->data.class_decl.class_name);
            free_node(node->data.class_decl.body);
            break;
        case NODE_BLOCK:
            for (Node **p = node->data.block.stmts; *p != NULL; ++p) free_node(*p);
            free(node->data.block.stmts);
            break;
        case NODE_WHILE:
            free_node(node->data.while_node.cond);
            free_node(node->data.while_node.body);
            break;
        case NODE_IF:
            free_node(node->data.if_node.cond);
            free_node(node->data.if_node.then_body);
            free_node(node->data.if_node.else_body);
            break;
        case NODE_BINARY:
            free_node(node->data.bin.left);
            free_node(node->data.bin.right);
            break;
        case NODE_UNARY:
            free_node(node->data.unary.operand);
            break;
        case NODE_FUNC_DECL:
            free(node->data.func_decl.name);
            for (size_t i = 0; i < node->data.func_decl.nparams; ++i) free(node->data.func_decl.params[i]);
            free(node->data.func_decl.params);
            free_node(node->data.func_decl.body);
            break;
        case NODE_RETURN:
            free_node(node->data.return_node.expr);
            break;
        default: break;
    }
    free(node);
}

static void free_function_table(void) {
    FuncEntry *p = func_table;
    while (p) {
        FuncEntry *n = p->next;
        free(p->name);
        for (size_t i = 0; i < p->nparams; ++i) free(p->params[i]);
        free(p->params);
        free(p);
        p = n;
    }
    func_table = NULL;
}

/*=====================================================================
 * 9.   MAIN (melhor parte kk)
 *===================================================================== */

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <arquivo.kc>\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) { perror("fopen"); return 1; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *src = malloc(sz + 1);
    fread(src, 1, sz, f);
    src[sz] = '\0';
    fclose(f);

    TokenStream ts = tokenize(src);
    global_ts = &ts;
    global_tok_pos = 0;

    Node **program = parse_program();

    Stack stack;
    stack_init(&stack);
    Env env;
    env_init(&env, NULL);

    for (Node **pn = program; *pn != NULL; ++pn) {
        if ((*pn)->type == NODE_FUNC_DECL) {
            register_function(*pn);
        }
    }

    for (Node **pn = program; *pn != NULL; ++pn) {
        if ((*pn)->type != NODE_FUNC_DECL) {
            exec_node(*pn, &stack, &env);
        }
    }

    /* Cleanup */
    for (size_t i = 0; i < ts.size; ++i) token_free(&ts.tokens[i]);
    free(ts.tokens);

    for (Node **pn = program; *pn != NULL; ++pn) {
        free_node(*pn);
    }
    free(program);
    free(src);
    free(stack.stack);

    VarPair *vp = env.head;
    while (vp) {
        VarPair *next = vp->next;
        free(vp->name);
        free(vp);
        vp = next;
    }

    free_function_table();

    if (g_network_initialized) {
        if (g_curl_handle) {
            curl_easy_cleanup(g_curl_handle);
            g_curl_handle = NULL;
        }
        curl_global_cleanup();
    }

    sleep(1);
    return 0;
}