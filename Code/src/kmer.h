#pragma once

/* Structure to represent DNA */

typedef struct Kmer {
    int32 size;
    char sequence[FLEXIBLE_ARRAY_MEMBER];
} Kmer;

