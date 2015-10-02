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
  NEWLINE,     // "\n"
};

struct token
{
  enum token_type type;
  char *value;
};
