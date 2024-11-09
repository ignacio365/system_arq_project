#pragma once

/* fmgr macros complex type */

#define DatumToDnaP(X)  ((Dna *) DatumGetPointer(X)) /*Converts Datum to a pointer to DNA*/
#define DnaPGetDatum(X)  PointerGetDatum(X) /*Converts pointer to DNA to Datum*/
#define PG_GETARG_DNA_P(n) DatumToDnaP(PG_GETARG_DATUM(n)) /*Gets the nth input to a Postgresql function and transforms it into a pointer to DNA*/
#define PG_RETURN_DNA_P(x) return DnaPGetDatum(x) /*Returns a pointer to a Dna type as a Datum from a PostgreSQL function.*/

/******************************************************************************************************/

/* Structure to represent DNA */

typedef struct Dna {
    int32 size;
    char sequence[FLEXIBLE_ARRAY_MEMBER];
} Dna;

