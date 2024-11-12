#include <stdio.h>
#include "postgres.h"
#include <stdlib.h>

#include "varatt.h" 
#include "utils/builtins.h"
#include "libpq/pqformat.h"

#include "funcapi.h"
#include "dna.h"
#include "kmer.h"


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