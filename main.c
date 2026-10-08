/* Assignment 2: XML tag nesting validator using an array-based stack. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME 256
#define MAX_DEPTH 1024

typedef struct {
    char name[MAX_NAME];
    size_t line;
} Tag;

typedef struct {
    Tag tags[MAX_DEPTH]; /* Array of structures: the most recent tag is on top. */
    size_t count;
} Stack;

typedef struct {
    const char *text;
    size_t length, position, line, column;
    char error[512];
} Parser;

static int fail(Parser *p, const char *message)
{
    snprintf(p->error, sizeof(p->error), "Line %zu, column %zu: %s",
             p->line, p->column, message);
    return 0;
}

static char peek(const Parser *p)
{
    return p->position < p->length ? p->text[p->position] : '\0';
}

static void advance(Parser *p)
{
    if (p->position >= p->length) return;
    if (p->text[p->position++] == '\n') {
        p->line++;
        p->column = 1;
    } else {
        p->column++;
    }
}

static int starts_with(const Parser *p, const char *text)
{
    size_t n = strlen(text);
    return n <= p->length - p->position &&
           memcmp(p->text + p->position, text, n) == 0;
}

static void consume(Parser *p, size_t n)
{
    while (n-- > 0) advance(p);
}

static int is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static void skip_spaces(Parser *p)
{
    while (is_space(peek(p))) advance(p);
}

/* This assignment supports ASCII names, including namespace-style prefixes. */
static int name_start(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
           c == '_' || c == ':';
}

static int name_character(char c)
{
    return name_start(c) || (c >= '0' && c <= '9') || c == '-' || c == '.';
}

static int read_name(Parser *p, char name[MAX_NAME])
{
    size_t n = 0;
    if (!name_start(peek(p))) return fail(p, "Expected an XML tag or attribute name.");
    while (name_character(peek(p))) {
        if (n == MAX_NAME - 1) return fail(p, "Name exceeds the 255-character limit.");
        name[n++] = peek(p);
        advance(p);
    }
    name[n] = '\0';
    return 1;
}

static int push(Parser *p, Stack *stack, const char *name, size_t line)
{
    if (stack->count == MAX_DEPTH) return fail(p, "Nesting exceeds the 1024-tag limit.");
    strcpy(stack->tags[stack->count].name, name);
    stack->tags[stack->count].line = line;
    stack->count++;
    return 1;
}

static int close_tag(Parser *p, Stack *stack, const char *name)
{
    char message[400];
    Tag *top;
    if (stack->count == 0) return fail(p, "Closing tag has no matching opening tag.");
    top = &stack->tags[stack->count - 1];
    if (strcmp(top->name, name) != 0) {
        snprintf(message, sizeof(message),
                 "Expected </%s> for the tag opened on line %zu; closing tag does not match.",
                 top->name, top->line);
        return fail(p, message);
    }
    stack->count--; /* Pop only when the names match exactly (case-sensitive). */
    return 1;
}

/* Quoted attribute values may contain '>', so do not stop at that character. */
static int read_start_tag(Parser *p, char name[MAX_NAME], int *self_closing)
{
    char attribute[MAX_NAME];
    if (!read_name(p, name)) return 0;
    *self_closing = 0;
    for (;;) {
        int had_space = is_space(peek(p));
        char quote;
        skip_spaces(p);
        if (starts_with(p, "/>")) {
            consume(p, 2);
            *self_closing = 1;
            return 1;
        }
        if (peek(p) == '>') {
            advance(p);
            return 1;
        }
        if (!had_space) return fail(p, "Expected '>', '/>', or whitespace before an attribute.");
        if (!read_name(p, attribute)) return 0;
        skip_spaces(p);
        if (peek(p) != '=') return fail(p, "Expected '=' after an attribute name.");
        advance(p);
        skip_spaces(p);
        quote = peek(p);
        if (quote != '\'' && quote != '"') return fail(p, "Attribute values must be quoted.");
        advance(p);
        while (p->position < p->length && peek(p) != quote) {
            if (peek(p) == '<') return fail(p, "An attribute value cannot contain a literal '<'.");
            advance(p);
        }
        if (peek(p) != quote) return fail(p, "Unterminated attribute value.");
        advance(p);
    }
}

static int skip_section(Parser *p, size_t prefix_length, const char *end,
                        const char *error)
{
    consume(p, prefix_length);
    while (p->position < p->length) {
        if (starts_with(p, end)) {
            consume(p, strlen(end));
            return 1;
        }
        advance(p);
    }
    return fail(p, error);
}

static int skip_comment(Parser *p)
{
    consume(p, 4); /* <!-- */
    while (p->position < p->length) {
        if (starts_with(p, "-->")) {
            consume(p, 3);
            return 1;
        }
        if (starts_with(p, "--")) return fail(p, "A comment cannot contain '--'.");
        advance(p);
    }
    return fail(p, "Unterminated XML comment.");
}

static int validate_xml(Parser *p, Stack *stack)
{
    int root_seen = 0;
    /* Accept a UTF-8 byte-order mark; positions after it begin at column 1. */
    if (starts_with(p, "\xEF\xBB\xBF")) p->position = 3;
    while (p->position < p->length) {
        char name[MAX_NAME];
        size_t tag_line = p->line;
        int self_closing;
        if (peek(p) == '\0') return fail(p, "Unexpected NUL byte in the file.");
        if (peek(p) != '<') {
            if (stack->count == 0 && !is_space(peek(p)))
                return fail(p, "Text is not allowed outside the root element.");
            if (starts_with(p, "]]>")) return fail(p, "']]>' is only allowed to end CDATA.");
            advance(p);
            continue;
        }
        if (starts_with(p, "<!--")) {
            if (!skip_comment(p)) return 0;
        } else if (starts_with(p, "<?")) {
            /* Declarations/processing instructions are skipped for nesting checks. */
            if (!skip_section(p, 2, "?>", "Unterminated processing instruction.")) return 0;
        } else if (starts_with(p, "<![CDATA[")) {
            if (stack->count == 0) return fail(p, "CDATA must be inside an element.");
            if (!skip_section(p, 9, "]]>", "Unterminated CDATA section.")) return 0;
        } else if (starts_with(p, "<!")) {
            return fail(p, "DOCTYPE/DTD and other <! declarations are not supported.");
        } else if (starts_with(p, "</")) {
            consume(p, 2);
            if (!read_name(p, name)) return 0;
            skip_spaces(p);
            if (peek(p) != '>') return fail(p, "Expected '>' after a closing tag.");
            if (!close_tag(p, stack, name)) return 0;
            advance(p);
        } else {
            advance(p); /* < */
            if (!read_start_tag(p, name, &self_closing)) return 0;
            if (stack->count == 0) {
                if (root_seen) return fail(p, "An XML document must have only one root element.");
                root_seen = 1;
            }
            if (!self_closing && !push(p, stack, name, tag_line)) return 0;
        }
    }
    if (stack->count > 0) {
        char message[400];
        Tag *top = &stack->tags[stack->count - 1];
        snprintf(message, sizeof(message), "Missing </%s> for the tag opened on line %zu.",
                 top->name, top->line);
        return fail(p, message);
    }
    if (!root_seen) return fail(p, "The file has no root element.");
    return 1;
}

/* Read in chunks: the input is not restricted to a fixed file size. */
static char *read_file(const char *path, size_t *length)
{
    FILE *file = fopen(path, "rb");
    size_t capacity = 4096;
    char *data;
    if (file == NULL) {
        fprintf(stderr, "Cannot open XML file: %s\n", path);
        return NULL;
    }
    data = malloc(capacity);
    *length = 0;
    if (data == NULL) {
        fclose(file);
        fprintf(stderr, "Not enough memory to read the file.\n");
        return NULL;
    }
    for (;;) {
        size_t n = fread(data + *length, 1, capacity - *length, file);
        *length += n;
        if (ferror(file)) {
            fprintf(stderr, "Error reading XML file: %s\n", path);
            free(data);
            fclose(file);
            return NULL;
        }
        if (feof(file)) break;
        if (*length == capacity) {
            char *grown;
            if (capacity > (size_t)-1 / 2) grown = NULL;
            else grown = realloc(data, capacity * 2);
            if (grown == NULL) {
                fprintf(stderr, "Not enough memory to read the file.\n");
                free(data);
                fclose(file);
                return NULL;
            }
            data = grown;
            capacity *= 2;
        }
    }
    fclose(file);
    return data;
}

int main(int argc, char *argv[])
{
    const char *path = argc == 2 ? argv[1] : "note.xml";
    size_t length;
    char *text;
    Stack *stack;
    Parser parser;
    int valid;
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [XML-file]\n", argv[0]);
        return 2;
    }
    text = read_file(path, &length);
    if (text == NULL) return 2;
    stack = calloc(1, sizeof(*stack));
    if (stack == NULL) {
        fprintf(stderr, "Not enough memory for the tag stack.\n");
        free(text);
        return 2;
    }
    parser.text = text;
    parser.length = length;
    parser.position = 0;
    parser.line = 1;
    parser.column = 1;
    parser.error[0] = '\0';
    valid = validate_xml(&parser, stack);
    puts(valid ? "XML is valid" : "XML is invalid");
    if (!valid) puts(parser.error);
    free(stack);
    free(text);
    return valid ? 0 : 1;
}
