
#include <stdio.h>
#include "postgres.h"
#include <stdlib.h>

#include "varatt.h" 
#include "utils/builtins.h"
#include "libpq/pqformat.h"

#include "kmer.h"



/**********************************************************/

/*KMER CREATION*/


/*Kmer creation from str (internal) (with checks)*/
Kmer*
kmer_parse(const char* str)
{
  int32 len = strlen(str);
  Kmer *kmer;

  if (str == NULL ||str[0] == '\0'){
        ereport(ERROR, (errmsg("Input array cannot be NULL or empty")));
    }

  if(strlen(str) > 32){
        ereport(ERROR, (errmsg("Input array cannot be longer than 32 nucleotides.")));
    }


  for (int i = 0; i < len; i++) {
    if (!(str[i]== 'A' || str[i] == 'C' || str[i] == 'G' || str[i]== 'T')) {
			ereport(
            ERROR,
            (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
            errmsg("Error: Invalid nucleotide '%c' in sequence.\n", str[i])));
    }
  }
  
  kmer = (Kmer*) palloc(VARHDRSZ + len  + 1 );
  SET_VARSIZE(kmer, VARHDRSZ +  len  + 1 );
  memcpy(kmer->sequence, str, len + 1);

  return kmer;
}


/*Kmer to str (internal)*/
static char *
kmer_to_str(const Kmer* kmer)
{
  return pstrdup(kmer->sequence);
}

/********************************************************/

/*Internal function for Postgre to create the Kmer datatype*/

/*In function (str -> Kmer)*/
PG_FUNCTION_INFO_V1(kmer_in);
Datum
kmer_in(PG_FUNCTION_ARGS)
{
  const char * str = PG_GETARG_CSTRING(0);
  PG_RETURN_POINTER(kmer_parse(str));
  
}

/*Out function (Kmer -> str)*/
PG_FUNCTION_INFO_V1(kmer_out);
Datum
kmer_out(PG_FUNCTION_ARGS)
{
  const Kmer *kmer = (Kmer *) PG_GETARG_POINTER(0);
  PG_RETURN_CSTRING(kmer_to_str(kmer));
}

/*Binary in (binary -> Kmer)*/

PG_FUNCTION_INFO_V1(kmer_recv);
Datum
kmer_recv(PG_FUNCTION_ARGS)
{
    StringInfo buf = (StringInfo) PG_GETARG_POINTER(0);
    int32 len = pq_getmsgint(buf, sizeof(int32));
    Kmer *kmer = (Kmer *) palloc(VARHDRSZ + len + 1); 
    SET_VARSIZE(kmer, VARHDRSZ + len + 1);  
    pq_copymsgbytes(buf, kmer->sequence, len);
    kmer->sequence[len] = '\0';
    PG_RETURN_POINTER(kmer);
}


/*Binary out (Kmer --> out)*/

PG_FUNCTION_INFO_V1(kmer_send);
Datum
kmer_send(PG_FUNCTION_ARGS)
{
    Kmer *kmer = (Kmer *) PG_GETARG_POINTER(0);
    int32 len = VARSIZE(kmer) - VARHDRSZ; 
    StringInfoData buf;
    pq_begintypsend(&buf);
    pq_sendint32(&buf, len);
    pq_sendbytes(&buf, kmer->sequence, len);
    PG_FREE_IF_COPY(kmer, 0);
    PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

/*************************************************/

/*text -> Kmer (external)*/
PG_FUNCTION_INFO_V1(kmer_cast_from_text);
Datum
kmer_cast_from_text(PG_FUNCTION_ARGS)
{
  text *txt = PG_GETARG_TEXT_P(0);
  char *str = DatumGetCString(DirectFunctionCall1(textout,
               PointerGetDatum(txt))); /*Postgre text to C string*/
  PG_RETURN_POINTER(kmer_parse(str));
}

/* Kmer -> text (external)*/
PG_FUNCTION_INFO_V1(kmer_cast_to_text);
Datum
kmer_cast_to_text(PG_FUNCTION_ARGS)
{
  const Kmer *kmer  = (Kmer *) PG_GETARG_POINTER(0); 
  text *out = (text *)DirectFunctionCall1(textin,
            PointerGetDatum(kmer_to_str(kmer)));
  PG_RETURN_TEXT_P(out);
}

/*******************************************************/

/* Functions */

/* Returns the size in memory of the datatype*/
PG_FUNCTION_INFO_V1(kmer_size);
Datum
kmer_size(PG_FUNCTION_ARGS)
{
  const Kmer *kmer  = (Kmer *) PG_GETARG_POINTER(0);
  PG_RETURN_INT32(kmer->size); 
}

/*Length*/
PG_FUNCTION_INFO_V1(kmer_len);
Datum
kmer_len(PG_FUNCTION_ARGS)
{
  const Kmer *kmer  = (Kmer *) PG_GETARG_POINTER(0);
  PG_RETURN_INT32(strlen(kmer->sequence)); 
}
