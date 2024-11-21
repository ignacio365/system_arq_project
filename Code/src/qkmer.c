
#include <stdio.h>
#include "postgres.h"

#include "access/spgist.h"
#include <stdlib.h>

#include "catalog/pg_type.h" //Data types for the index
#include "utils/datum.h" //Datum operations index

#include "utils/pg_locale.h"
#include "utils/varlena.h"

#include "varatt.h" 
#include "utils/builtins.h"
#include "libpq/pqformat.h"

#include "fmgr.h"
#include <string.h>  // For string manipulation

#include "qkmer.h"
#include "kmer.h"
#include <regex.h>  /* Include the regex library */


/**********************************************************/

/*KMER CREATION*/


/*Kmer creation from str (internal) (with checks)*/
Qkmer*
qkmer_parse(const char* str)
{
  int32 len = strlen(str);
  Qkmer *qkmer;

  if (str == NULL ||str[0] == '\0'){
        ereport(ERROR, (errmsg("Input array cannot be NULL or empty")));
    }

  if(strlen(str) > 32){
        ereport(ERROR, (errmsg("Input array cannot be longer than 32 nucleotides.")));
    }


  for (int i = 0; i < len; i++) {
    if (!(str[i]== 'A' || str[i] == 'C' || str[i] == 'G' || str[i]== 'T' || str[i]=='R' || str[i]=='Y' || str[i]=='N' )) {
			ereport(
            ERROR,
            (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
            errmsg("Error: Invalid nucleotide '%c' in sequence.\n", str[i])));
    }
  }
  
  qkmer = (Qkmer*) palloc(VARHDRSZ + len  + 1 );
  SET_VARSIZE(qkmer, VARHDRSZ +  len  + 1 );
  memcpy(qkmer->sequence, str, len + 1);

  return qkmer;
}



/*Qkmer to str (internal)*/
static char *
qkmer_to_str(const Qkmer* qkmer)
{
  return pstrdup(qkmer->sequence);
}

/********************************************************/

/*Internal function for Postgre to create the Kmer datatype*/

/*In function (str -> Kmer)*/
PG_FUNCTION_INFO_V1(qkmer_in);
Datum
qkmer_in(PG_FUNCTION_ARGS)
{
  const char * str = PG_GETARG_CSTRING(0);
  PG_RETURN_POINTER(qkmer_parse(str));
  
}

/*Out function (qKmer -> str)*/
PG_FUNCTION_INFO_V1(qkmer_out);
Datum
qkmer_out(PG_FUNCTION_ARGS)
{
  const Qkmer *qkmer = (Qkmer *) PG_GETARG_POINTER(0);
  PG_RETURN_CSTRING(qkmer_to_str(qkmer));
}

/*Binary in (binary -> Kmer)*/

PG_FUNCTION_INFO_V1(qkmer_recv);
Datum
qkmer_recv(PG_FUNCTION_ARGS)
{
    StringInfo buf = (StringInfo) PG_GETARG_POINTER(0);
    int32 len = pq_getmsgint(buf, sizeof(int32));
    Qkmer *qkmer = (Qkmer *) palloc(VARHDRSZ + len + 1); 
    SET_VARSIZE(qkmer, VARHDRSZ + len + 1);  
    pq_copymsgbytes(buf, qkmer->sequence, len);
    qkmer->sequence[len] = '\0';
    PG_RETURN_POINTER(qkmer);
}


/*Binary out (qKmer --> out)*/

PG_FUNCTION_INFO_V1(qkmer_send);
Datum
qkmer_send(PG_FUNCTION_ARGS)
{
    Qkmer *qkmer = (Qkmer *) PG_GETARG_POINTER(0);
    int32 len = VARSIZE(qkmer) - VARHDRSZ; 
    StringInfoData buf;
    pq_begintypsend(&buf);
    pq_sendint32(&buf, len);
    pq_sendbytes(&buf, qkmer->sequence, len);
    PG_FREE_IF_COPY(qkmer, 0);
    PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

/*************************************************/

/*text -> qKmer (external)*/
PG_FUNCTION_INFO_V1(qkmer_cast_from_text);
Datum
qkmer_cast_from_text(PG_FUNCTION_ARGS)
{
  text *txt = PG_GETARG_TEXT_P(0);
  char *str = DatumGetCString(DirectFunctionCall1(textout,
               PointerGetDatum(txt))); /*Postgre text to C string*/
  PG_RETURN_POINTER(qkmer_parse(str));
}

/* qKmer -> text (external)*/
PG_FUNCTION_INFO_V1(qkmer_cast_to_text);
Datum
qkmer_cast_to_text(PG_FUNCTION_ARGS)
{
  const Qkmer *qkmer  = (Qkmer *) PG_GETARG_POINTER(0); 
  text *out = (text *)DirectFunctionCall1(textin,
            PointerGetDatum(qkmer_to_str(qkmer)));
  PG_RETURN_TEXT_P(out);
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

    /* Check if pattern and kmer have the same length */
    if (strlen(qkmer) != strlen(kmer)) {
        PG_RETURN_BOOL(false);
    }

    /* Convert 'N' in pattern to a regex pattern [ATGC] */
    char regex_pattern[1024] = {0};
    char *p = qkmer;
    int i = 0;

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
        else {
            /* Append the character as-is */
            regex_pattern[i++] = *p;
        }
        p++;
    }

    /* Compile the regular expression */
    regex_t regex;
    int ret = regcomp(&regex, regex_pattern, REG_EXTENDED);
    if (ret) {
        ereport(ERROR, (errmsg("Could not compile regex")));
    }

    /* Execute the regex match */
    ret = regexec(&regex, kmer, 0, NULL, 0);
    regfree(&regex);

    /* Return true if it matches, false otherwise */
    PG_RETURN_BOOL(ret == 0);
}

