/* Structure to represent Kmer */

typedef struct Kmer {
    int32 size;
    char sequence[FLEXIBLE_ARRAY_MEMBER];
} Kmer;

Kmer* kmer_parse(const char* str);
Datum kmer_in(PG_FUNCTION_ARGS);
Datum kmer_out(PG_FUNCTION_ARGS);
Datum kmer_recv(PG_FUNCTION_ARGS);
Datum kmer_send(PG_FUNCTION_ARGS);
Datum kmer_cast_from_text(PG_FUNCTION_ARGS);
Datum kmer_cast_to_text(PG_FUNCTION_ARGS);
Datum kmer_size(PG_FUNCTION_ARGS);
Datum kmer_len(PG_FUNCTION_ARGS);
