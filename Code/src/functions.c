#include <stdio.h>
#include "postgres.h"
#include <stdlib.h>

#include "varatt.h" 
#include "utils/builtins.h"
#include "libpq/pqformat.h"

#include "funcapi.h"
#include "dna.h"
#include "kmer.h"
#include "qkmer.h"
#include <regex.h>  /* Include the regex library */


typedef struct {
    int k;           // Store the integer k
    char *sequence;  // Store the DNA sequence (or other values you need)
} FuncData;  // Define a struct to hold the values you need

PG_FUNCTION_INFO_V1(generate_kmers);
Datum
generate_kmers(PG_FUNCTION_ARGS)
{
    FuncCallContext     *funcctx;
    int                  k;
    Dna                  *dna;
    char                 *str;
    int                  input_len;
    int                  call_cntr;
    int                  max_calls;
    FuncData             *data;
 
    /* stuff done only on the first call of the function */
    if (SRF_IS_FIRSTCALL())
    {
        MemoryContext   oldcontext;
        funcctx = SRF_FIRSTCALL_INIT();
        oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);
        
        dna  = (Dna *) PG_GETARG_POINTER(0);
        k = PG_GETARG_INT32(1);

        str= dna->sequence;
        input_len = strlen(str);


        if (k <= 0 || k > input_len) {
            ereport(ERROR,
            (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
             errmsg("k must be between 1 and the length of the DNA sequence")));
        }

        data = palloc(sizeof(FuncData));

        data->k = k;
        data->sequence = str;

        funcctx->max_calls = input_len - k + 1;
        funcctx->user_fctx  =  (void *) data;

        MemoryContextSwitchTo(oldcontext);
    }

    /* stuff done on every call of the function */
    funcctx = SRF_PERCALL_SETUP();
    call_cntr = funcctx->call_cntr;
    max_calls = funcctx->max_calls;
    data = (FuncData *) funcctx->user_fctx;
    k = data->k;  
    str = data->sequence;

    if (call_cntr < max_calls)    /* do when there is more left to send */
    {
        Kmer         *kmer;
        char *substring= palloc((k+1) * sizeof(char));
        
        strncpy(substring, str + call_cntr, k);
        substring[k] = '\0'; 

        /* Convert the substring to a PostgreSQL text type */
        kmer = kmer_parse(substring);
        pfree(substring);

        /* Return the result substring */
        SRF_RETURN_NEXT(funcctx, PointerGetDatum(kmer));
    }
    else
    {
        /* No more substrings to return */
        SRF_RETURN_DONE(funcctx);
    }
}



PG_FUNCTION_INFO_V1(contains);
Datum
contains(PG_FUNCTION_ARGS)
{
    text *qkmer_text = PG_GETARG_TEXT_PP(0);
    text *kmer_text = PG_GETARG_TEXT_PP(1);

    /* Convert the PostgreSQL text types to C strings */
    char *qkmer = text_to_cstring(qkmer_text);
    char *kmer = text_to_cstring(kmer_text);
    char regex_pattern[1024] = {0};
    char *p;
    int i = 0;
    regex_t regex;
    int ret;

    /* Check if pattern and kmer have the same length */
    if (strlen(qkmer) != strlen(kmer)) {
        PG_RETURN_BOOL(false);
    }

    /* Convert 'N' in pattern to a regex pattern [ATGC] */
    p = qkmer;

    while (*p != '\0' && i < sizeof(regex_pattern) - 1) {
        if (*p == 'N') {
            /* Append [ATGC] in place of 'N' */
            strncat(regex_pattern, "[ATGC]", sizeof(regex_pattern) - strlen(regex_pattern) - 1);
            i += 6;
        } 
        else if (*p == 'R'){
            /* Append [AG] in place of 'R' */
            strncat(regex_pattern, "[AG]", sizeof(regex_pattern) - strlen(regex_pattern) - 1);
            i += 4;
        }
        else if (*p == 'Y'){
            /* Append [CT] in place of 'Y' */
            strncat(regex_pattern, "[CT]", sizeof(regex_pattern) - strlen(regex_pattern) - 1);
            i += 4;
        }
        else if (*p == 'W'){
            /* Append [AT] in place of 'W' */
            strncat(regex_pattern, "[AT]", sizeof(regex_pattern) - strlen(regex_pattern) - 1);
            i += 4;
        }
        else if (*p == 'S'){
            /* Append [CG] in place of 'S' */
            strncat(regex_pattern, "[CG]", sizeof(regex_pattern) - strlen(regex_pattern) - 1);
            i += 4;
        }
        else if (*p == 'M'){
            /* Append [AC] in place of 'M' */
            strncat(regex_pattern, "[AC]", sizeof(regex_pattern) - strlen(regex_pattern) - 1);
            i += 4;
        }
        else if (*p == 'K'){
            /* Append [GT] in place of 'K' */
            strncat(regex_pattern, "[GT]", sizeof(regex_pattern) - strlen(regex_pattern) - 1);
            i += 4;
        }
        else if (*p == 'B'){
            /* Append [CGT] in place of 'B' */
            strncat(regex_pattern, "[CGT]", sizeof(regex_pattern) - strlen(regex_pattern) - 1);
            i += 5;
        }
        else if (*p == 'D'){
            /* Append [AGT] in place of 'D' */
            strncat(regex_pattern, "[AGT]", sizeof(regex_pattern) - strlen(regex_pattern) - 1);
            i += 5;
        }
        else if (*p == 'H'){
            /* Append [ACT] in place of 'H' */
            strncat(regex_pattern, "[ACT]", sizeof(regex_pattern) - strlen(regex_pattern) - 1);
            i += 5;
        }
        else if (*p == 'V'){
            /* Append [ACG] in place of 'V' */
            strncat(regex_pattern, "[ACG]", sizeof(regex_pattern) - strlen(regex_pattern) - 1);
            i += 5;
        }
        else {
            /* Append the character as-is */
            regex_pattern[i++] = *p;
        }
        p++;
    }

    /* Compile the regular expression */
    ret = regcomp(&regex, regex_pattern, REG_EXTENDED);
    if (ret) {
        ereport(ERROR, (errmsg("Could not compile regex")));
    }

    /* Execute the regex match */
    ret = regexec(&regex, kmer, 0, NULL, 0);
    regfree(&regex);

    /* Return true if it matches, false otherwise */
    PG_RETURN_BOOL(ret == 0);
}

PG_FUNCTION_INFO_V1(kmer_cast_to_dna);
Datum
kmer_cast_to_dna(PG_FUNCTION_ARGS)
{
  const Kmer *kmer  = (Kmer *) PG_GETARG_POINTER(0); 
  Dna *out = dna_parse(kmer_to_str(kmer));
  PG_RETURN_TEXT_P(out);
}

PG_FUNCTION_INFO_V1(dna_cast_to_kmer);
Datum
dna_cast_to_kmer(PG_FUNCTION_ARGS)
{
  const Dna *dna  = (Dna *) PG_GETARG_POINTER(0); 
  Kmer *out = kmer_parse(dna_to_str(dna));
  PG_RETURN_TEXT_P(out);
}
