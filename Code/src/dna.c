
#include <stdio.h>
#include "postgres.h"
#include <stdlib.h>

#include "varatt.h" 
#include "utils/builtins.h"
#include "libpq/pqformat.h"

#include "dna.h"


PG_MODULE_MAGIC; /*Checks for incompatibilities*/

/**********************************************************/

/*DNA CREATION*/


/*Dna creation from str (internal) (with checks)*/
static Dna*
dna_parse(const char* str)
{
  int32 len = strlen(str);
  Dna *dna;

  if (str == NULL ||str[0] == '\0'){
        ereport(ERROR, (errmsg("Input array cannot be NULL or empty")));
    }


  for (int i = 0; i < len; i++) {
    if (!(str[i]== 'A' || str[i] == 'C' || str[i] == 'G' || str[i]== 'T')) {
			ereport(
            ERROR,
            (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
            errmsg("Error: Invalid nucleotide '%c' in sequence.\n", str[i])));
    }
  }
  
  dna = (Dna *) palloc(VARHDRSZ + len  + 1 );
  SET_VARSIZE(dna, VARHDRSZ +  len  + 1 );
  memcpy(dna->sequence, str, len + 1);

  return dna;
}



/*Dna to str (internal)*/
static char *
dna_to_str(const Dna* dna)
{
  return pstrdup(dna->sequence);
}

/********************************************************/

/*Internal function for Postgre to create the Dna datatype*/

/*In function (str -> Dna)*/
PG_FUNCTION_INFO_V1(dna_in);
Datum
dna_in(PG_FUNCTION_ARGS)
{
  const char * str = PG_GETARG_CSTRING(0);
  PG_RETURN_POINTER(dna_parse(str));
  
}

/*Out function (Dna -> str)*/
PG_FUNCTION_INFO_V1(dna_out);
Datum
dna_out(PG_FUNCTION_ARGS)
{
  const Dna *dna = (Dna *) PG_GETARG_POINTER(0);
  PG_RETURN_CSTRING(dna_to_str(dna));
}

/*Binary in (binary -> Dna)*/

PG_FUNCTION_INFO_V1(dna_recv);
Datum
dna_recv(PG_FUNCTION_ARGS)
{
    StringInfo buf = (StringInfo) PG_GETARG_POINTER(0);
    int32 len = pq_getmsgint(buf, sizeof(int32));
    Dna *dna = (Dna *) palloc(VARHDRSZ + len + 1); 
    SET_VARSIZE(dna, VARHDRSZ + len + 1);  
    pq_copymsgbytes(buf, dna->sequence, len);
    dna->sequence[len] = '\0';
    PG_RETURN_POINTER(dna);
}


/*Binary out (Dna --> out)*/

PG_FUNCTION_INFO_V1(dna_send);
Datum
dna_send(PG_FUNCTION_ARGS)
{
    Dna *dna = (Dna *) PG_GETARG_POINTER(0);
    int32 len = VARSIZE(dna) - VARHDRSZ; 
    StringInfoData buf;
    pq_begintypsend(&buf);
    pq_sendint32(&buf, len);
    pq_sendbytes(&buf, dna->sequence, len);
    PG_FREE_IF_COPY(dna, 0);
    PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

/*************************************************/

/*text -> Dna (external)*/
PG_FUNCTION_INFO_V1(dna_cast_from_text);
Datum
dna_cast_from_text(PG_FUNCTION_ARGS)
{
  text *txt = PG_GETARG_TEXT_P(0);
  char *str = DatumGetCString(DirectFunctionCall1(textout,
               PointerGetDatum(txt))); /*Postgre text to C string*/
  PG_RETURN_POINTER(dna_parse(str));
}

/* Dna -> text (external)*/
PG_FUNCTION_INFO_V1(dna_cast_to_text);
Datum
dna_cast_to_text(PG_FUNCTION_ARGS)
{
  const Dna *dna  = (Dna *) PG_GETARG_POINTER(0); 
  text *out = (text *)DirectFunctionCall1(textin,
            PointerGetDatum(dna_to_str(dna)));
  PG_RETURN_TEXT_P(out);
}

/*******************************************************/

/* Functions */

/* Returns the size in memory of the datatype*/
PG_FUNCTION_INFO_V1(dna_size);
Datum
dna_size(PG_FUNCTION_ARGS)
{
  const Dna *dna  = (Dna *) PG_GETARG_POINTER(0);
  PG_RETURN_INT32(dna->size); 
}

/*Length*/
PG_FUNCTION_INFO_V1(dna_len);
Datum
dna_len(PG_FUNCTION_ARGS)
{
  const Dna *dna  = (Dna *) PG_GETARG_POINTER(0);
  PG_RETURN_INT32(strlen(dna->sequence)); 
}
