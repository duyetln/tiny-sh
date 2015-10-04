enum token_type
{
  WORD,
  INPUT,      // "<"
  OUTPUT,     // ">"
  PIPE,       // "|"
  OPENPAREN,  // "("
  CLOSEPAREN, // ")"
  AND,        // "&&"
  OR,         // "||"
  SEMICOLON,  // ";"
  NEWLINE,    // "\n"
};

struct token
{
  enum token_type type;
  char *value;
};

struct token_node
{
  struct token *value;
  struct token_node *prev;
  struct token_node *next;
};

struct token_stream
{
  struct token_node *head;
  struct token_node *tail;
  struct token_node *curr;
};

typedef struct token *token_t;
typedef struct token_node *token_node_t;
typedef struct token_stream *token_stream_t;

token_stream_t
make_token_stream (int (*get_next_byte) (void *), void *get_next_byte_argument);

void
destroy_token_stream (token_stream_t strm);

token_t
read_token (token_stream_t strm);

token_t
peek_token (token_stream_t strm, int c);

token_t
reset_token_stream (token_stream_t strm);

token_t
forward_token_stream (token_stream_t strm, int c);

token_t
backward_token_stream (token_stream_t strm, int c);

token_t
skip_token (token_stream_t strm, enum token_type t);
