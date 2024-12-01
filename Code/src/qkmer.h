/* Structure to represent Qkmer */

typedef struct Qkmer {
    int32 size;
    char sequence[FLEXIBLE_ARRAY_MEMBER];
} Qkmer;

Qkmer* qkmer_parse(const char* str);
Datum qkmer_in(PG_FUNCTION_ARGS);
Datum qkmer_out(PG_FUNCTION_ARGS);
Datum qkmer_recv(PG_FUNCTION_ARGS);
Datum qkmer_send(PG_FUNCTION_ARGS);
Datum qkmer_cast_from_text(PG_FUNCTION_ARGS);
Datum qkmer_cast_to_text(PG_FUNCTION_ARGS);
Datum qkmer_size(PG_FUNCTION_ARGS);
Datum qkmer_len(PG_FUNCTION_ARGS);

