#pragma once

/* Structure to represent DNA */

typedef struct Dna {
    int32 size;
    char sequence[FLEXIBLE_ARRAY_MEMBER];
} Dna;


Datum dna_in(PG_FUNCTION_ARGS);
Datum dna_out(PG_FUNCTION_ARGS);
Datum dna_recv(PG_FUNCTION_ARGS);
Datum dna_send(PG_FUNCTION_ARGS);
Datum dna_cast_from_text(PG_FUNCTION_ARGS);
Datum dna_cast_to_text(PG_FUNCTION_ARGS);
Datum dna_size(PG_FUNCTION_ARGS);
Datum dna_len(PG_FUNCTION_ARGS);

