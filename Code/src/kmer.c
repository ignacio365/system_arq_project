
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


#include "kmer.h"


/**********************************************************/

/*KMER CREATION*/


/*Kmer creation from str (internal) (with checks)*/
Kmer*
kmer_parse(const char* str)
{

  validate_dna_sequence(str);
  
  int32 len = strlen(str);
  Kmer *kmer;

  if(strlen(str) > 32){
        ereport(ERROR, (errmsg("Input array cannot be longer than 32 nucleotides.")));
    }
  
  kmer = (Kmer*) palloc(VARHDRSZ + len  + 1 );
  SET_VARSIZE(kmer, VARHDRSZ +  len  + 1 );
  memcpy(kmer->sequence, str, len + 1);

  return kmer;
}



/*Kmer to str (internal)*/
char *
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

/*Equals function*/

PG_FUNCTION_INFO_V1(kmer_equals);
Datum
kmer_equals(PG_FUNCTION_ARGS)
{
    // Retrieve arguments as C-strings (text type in PostgreSQL)
    text *a_text = PG_GETARG_TEXT_PP(0);
    text *b_text = PG_GETARG_TEXT_PP(1);

    // Convert `text` to C-strings for comparison
    const char *a = text_to_cstring(a_text);
    const char *b = text_to_cstring(b_text);

    // Assume `FPeq` is your custom function that compares two kmers.
    bool result = (strcmp(a, b) == 0);

    PG_RETURN_BOOL(result);
}

/*Starts with function*/
PG_FUNCTION_INFO_V1(starts_with);
Datum
starts_with(PG_FUNCTION_ARGS)
{
    // Retrieve the prefix and kmer arguments as text
    text *prefix_text = PG_GETARG_TEXT_PP(0);
    text *kmer_text = PG_GETARG_TEXT_PP(1);
    bool result;

    // Convert the `text` values to C-strings for easier manipulation
    const char *prefix = text_to_cstring(prefix_text);
    const char *kmer = text_to_cstring(kmer_text);

    // Check if prefix lenght is greater than the kmer length 
    if (strlen(prefix) > strlen(kmer)) {
        PG_RETURN_BOOL(false);
    }

    // Check if `kmer` starts with `prefix`
    result = (strncmp(kmer, prefix, strlen(prefix)) == 0);

    PG_RETURN_BOOL(result);
}



/***********************COUNTING SUPPORT***********************/

PG_FUNCTION_INFO_V1(kmer_hash);
Datum
kmer_hash(PG_FUNCTION_ARGS)
{
    Kmer *kmer = (Kmer *) PG_GETARG_POINTER(0);
    uint32 hash = 5381;  // Seed value
    int c;
    const char *str = kmer->sequence;

    // Compute hash using djb2 algorithm
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;

    PG_RETURN_UINT32(hash);
}


/*************************************************************************************************************************/
/* Index Structure (Using a SpGist index https://habr.com/en/companies/postgrespro/articles/446624/)*/
/* Source: https://doxygen.postgresql.org/spgtextproc_8c_source.html */
/****************************************************************************************************************/


 #define SPGIST_MAX_PREFIX_LENGTH    Max((int) (BLCKSZ - 258 * sizeof(Kmer) - 100), 32)
 #define SPG_STRATEGY_ADDITION   (10)
 #define SPG_IS_COLLATION_AWARE_STRATEGY(s) ((s) > SPG_STRATEGY_ADDITION \
                                          && (s) != RTPrefixStrategyNumber)
  
 /* Struct for sorting values in picksplit */
 typedef struct spgNodePtr
 {
     Datum       d;
     int         i;
     int16       c;
 } spgNodePtr;
 

 PG_FUNCTION_INFO_V1(spg_kmer_config);
 Datum
 spg_kmer_config(PG_FUNCTION_ARGS)
 {
     /* spgConfigIn *cfgin = (spgConfigIn *) PG_GETARG_POINTER(0); */
     spgConfigOut *cfg = (spgConfigOut *) PG_GETARG_POINTER(1);
  
     cfg->prefixType = TEXTOID;
     cfg->labelType = INT2OID;
     cfg->canReturnData = true;
     cfg->longValuesOK = true;   /* suffixing will shorten long values */
     PG_RETURN_VOID();
 }
  

static Datum
formKmerDatum(const char *data, int datalen)
{
    Kmer *kmer = (Kmer *) palloc(VARHDRSZ + sizeof(int32) + datalen + 1);

    SET_VARSIZE(kmer, VARHDRSZ + sizeof(int32) + datalen + 1);
    memcpy(kmer->sequence, data, datalen);
    kmer->sequence[datalen] = '\0';  // Null-terminate the sequence

    return PointerGetDatum(kmer);
}

 /*
  * Form a text datum from the given not-necessarily-null-terminated string,
  * using short varlena header format if possible
  */
 static Datum
 formTextDatum(const char *data, int datalen)
 {
     char       *p;
  
     p = (char *) palloc(datalen + VARHDRSZ);
  
     if (datalen + VARHDRSZ_SHORT <= VARATT_SHORT_MAX)
     {
         SET_VARSIZE_SHORT(p, datalen + VARHDRSZ_SHORT);
         if (datalen)
             memcpy(p + VARHDRSZ_SHORT, data, datalen);
     }
     else
     {
         SET_VARSIZE(p, datalen + VARHDRSZ);
         memcpy(p + VARHDRSZ, data, datalen);
     }
  
     return PointerGetDatum(p);
 }
  
 /*
  * Find the length of the common prefix of a and b
  */
 static int
 commonPrefix(const char *a, const char *b, int lena, int lenb) /*a and b are the first characters of the string*/
 {
     int         i = 0;
  
     while (i < lena && i < lenb && *a == *b)
     {
         a++;
         b++;
         i++;
     }
  
     return i; /*Returns the length of common characters*/
 }

  
 /*
  * Binary search an array of int16 datums for a match to c
  *
  * On success, *i gets the match location; on failure, it gets where to insert
  */
 static bool
 searchChar(Datum *nodeLabels, int nNodes, int16 c, int *i) /*Seachs in nodeLabels for c*/
 {
     int         StopLow = 0,
                 StopHigh = nNodes;
  
     while (StopLow < StopHigh)
     {
         int         StopMiddle = (StopLow + StopHigh) >> 1; /*This means dividing by two the window of search*/
         int16       middle = DatumGetInt16(nodeLabels[StopMiddle]);
  
         if (c < middle)
             StopHigh = StopMiddle;
         else if (c > middle)
             StopLow = StopMiddle + 1;
         else
         {
             *i = StopMiddle;
             return true;
         }
     }
  
     *i = StopHigh;
     return false;
 }

  
PG_FUNCTION_INFO_V1(spg_kmer_choose);
Datum
spg_kmer_choose(PG_FUNCTION_ARGS)
{
    spgChooseIn *in = (spgChooseIn *) PG_GETARG_POINTER(0);
    spgChooseOut *out = (spgChooseOut *) PG_GETARG_POINTER(1);
    Kmer       *inKmer = (Kmer *) DatumGetPointer(in->datum);
    const char *inSeq = inKmer->sequence;
    int         inSize = strlen(inSeq);
    const char *prefixStr = NULL;
    int         prefixSize = 0;
    int         commonLen = 0;
    int16       nodeChar = 0;
    int         i = 0;

    /* Check for prefix match, set nodeChar to first byte after prefix */
    if (in->hasPrefix)
    {
        text       *prefixText = DatumGetTextPP(in->prefixDatum);
  
        prefixStr = VARDATA_ANY(prefixText);
        prefixSize = VARSIZE_ANY_EXHDR(prefixText);
  
        commonLen = commonPrefix(inSeq + in->level,
                                 prefixStr,
                                 inSize - in->level,
                                 prefixSize);

        if (commonLen == prefixSize)
        {
            if (inSize - in->level > commonLen)
                nodeChar = *(unsigned char *) (inSeq + in->level + commonLen);
            else
                nodeChar = -1;
        }
        else
        {
            /* Must split tuple because incoming value doesn't match prefix */
            out->resultType = spgSplitTuple;

            if (commonLen == 0)
            {
                out->result.splitTuple.prefixHasPrefix = false;
            }
            else
            {
                out->result.splitTuple.prefixHasPrefix = true;
                out->result.splitTuple.prefixPrefixDatum =
                    formTextDatum(prefixStr, commonLen);
            }
            out->result.splitTuple.prefixNNodes = 1;
            out->result.splitTuple.prefixNodeLabels =
                (Datum *) palloc(sizeof(Datum));
            out->result.splitTuple.prefixNodeLabels[0] =
                Int16GetDatum(*(unsigned char *) (prefixStr + commonLen));

            out->result.splitTuple.childNodeN = 0;

            if (prefixSize - commonLen == 1)
            {
                out->result.splitTuple.postfixHasPrefix = false;
            }
            else
            {
                out->result.splitTuple.postfixHasPrefix = true;
                out->result.splitTuple.postfixPrefixDatum =
                    formTextDatum(prefixStr + commonLen + 1,
                                  prefixSize - commonLen - 1);
            }

            PG_RETURN_VOID();
        }
    }
    else if (inSize > in->level)
    {
        nodeChar = *(unsigned char *) (inSeq + in->level);
    }
    else
    {
        nodeChar = -1;
    }

    /* Look up nodeChar in the node label array */
    if (searchChar(in->nodeLabels, in->nNodes, nodeChar, &i))
    {
        /* Descend to existing node */
        int levelAdd;

        out->resultType = spgMatchNode;
        out->result.matchNode.nodeN = i;
        levelAdd = commonLen;
        if (nodeChar >= 0)
            levelAdd++;
        out->result.matchNode.levelAdd = levelAdd;
        if (inSize - in->level - levelAdd > 0)
            out->result.matchNode.restDatum =
                formKmerDatum(inSeq + in->level + levelAdd,
                              inSize - in->level - levelAdd);
        else
            out->result.matchNode.restDatum =
                formKmerDatum(NULL, 0);
    }
    else if (in->allTheSame)
    {
        /* Split the tuple */
        out->resultType = spgSplitTuple;
        out->result.splitTuple.prefixHasPrefix = in->hasPrefix;
        out->result.splitTuple.prefixPrefixDatum = in->prefixDatum;
        out->result.splitTuple.prefixNNodes = 1;
        out->result.splitTuple.prefixNodeLabels = (Datum *) palloc(sizeof(Datum));
        out->result.splitTuple.prefixNodeLabels[0] = Int16GetDatum(-2);
        out->result.splitTuple.childNodeN = 0;
        out->result.splitTuple.postfixHasPrefix = false;
    }
    else
    {
        /* Add a node for the not-previously-seen nodeChar value */
        out->resultType = spgAddNode;
        out->result.addNode.nodeLabel = Int16GetDatum(nodeChar);
        out->result.addNode.nodeN = i;
    }

    PG_RETURN_VOID();
}

  
 /* qsort comparator to sort spgNodePtr structs by "c" */
 static int
 cmpNodePtr(const void *a, const void *b)
 {
     const spgNodePtr *aa = (const spgNodePtr *) a;
     const spgNodePtr *bb = (const spgNodePtr *) b;
  
     /*return pg_cmp_s16(aa->c, bb->c);*/
    int16 diff = (aa->c > bb->c) - (aa->c < bb->c);
    return diff;

 }
 
PG_FUNCTION_INFO_V1(spg_kmer_picksplit);
Datum
spg_kmer_picksplit(PG_FUNCTION_ARGS)
{
    spgPickSplitIn *in = (spgPickSplitIn *) PG_GETARG_POINTER(0);
    spgPickSplitOut *out = (spgPickSplitOut *) PG_GETARG_POINTER(1);
    Kmer       *kmer0 = (Kmer *) DatumGetPointer(in->datums[0]);
    int         i,
                commonLen;
    spgNodePtr *nodes;

    /* Identify the longest common prefix, if any */
    commonLen = strlen(kmer0->sequence);
    for (i = 1; i < in->nTuples && commonLen > 0; i++)
    {
        Kmer *kmeri = (Kmer *) DatumGetPointer(in->datums[i]);
        int   tmp = commonPrefix(kmer0->sequence,
                                 kmeri->sequence,
                                 strlen(kmer0->sequence),
                                 strlen(kmeri->sequence));

        if (tmp < commonLen)
            commonLen = tmp;
    }

    /*
     * Limit the prefix length, if necessary, to ensure that the resulting
     * inner tuple will fit on a page.
     */
    commonLen = Min(commonLen, SPGIST_MAX_PREFIX_LENGTH);

    /* Set node prefix to be that string, if it's not empty */
    if (commonLen == 0)
    {
        out->hasPrefix = false;
    }
    else
    {
        out->hasPrefix = true;
        out->prefixDatum = formTextDatum(kmer0->sequence, commonLen);
    }

    /* Extract the node label (first non-common byte) from each value */
    nodes = (spgNodePtr *) palloc(sizeof(spgNodePtr) * in->nTuples);

    for (i = 0; i < in->nTuples; i++)
    {
        Kmer *kmeri = (Kmer *) DatumGetPointer(in->datums[i]);

        if (commonLen < strlen(kmeri->sequence))
            nodes[i].c = (int16) *(unsigned char *) (kmeri->sequence + commonLen);
        else
            nodes[i].c = -1; /* use -1 if the sequence is entirely common */
        nodes[i].i = i;
        nodes[i].d = in->datums[i];
    }

    /*
     * Sort by label values so that we can group the values into nodes. This
     * also ensures that the nodes are ordered by label value, allowing the
     * use of binary search in `searchChar`.
     */
    qsort(nodes, in->nTuples, sizeof(*nodes), cmpNodePtr);

    /* And emit results */
    out->nNodes = 0;
    out->nodeLabels = (Datum *) palloc(sizeof(Datum) * in->nTuples);
    out->mapTuplesToNodes = (int *) palloc(sizeof(int) * in->nTuples);
    out->leafTupleDatums = (Datum *) palloc(sizeof(Datum) * in->nTuples);

    for (i = 0; i < in->nTuples; i++)
    {
        Kmer *kmeri = (Kmer *) DatumGetPointer(nodes[i].d);
        Datum leafD;

        if (i == 0 || nodes[i].c != nodes[i - 1].c)
        {
            out->nodeLabels[out->nNodes] = Int16GetDatum(nodes[i].c);
            out->nNodes++;
        }

        if (commonLen < strlen(kmeri->sequence))
            leafD = formKmerDatum(kmeri->sequence + commonLen + 1,
                                  strlen(kmeri->sequence) - commonLen - 1);
        else
            leafD = formKmerDatum(NULL, 0);

        out->leafTupleDatums[nodes[i].i] = leafD;
        out->mapTuplesToNodes[nodes[i].i] = out->nNodes - 1;
    }

    PG_RETURN_VOID();
}

PG_FUNCTION_INFO_V1(spg_kmer_inner_consistent);
Datum
spg_kmer_inner_consistent(PG_FUNCTION_ARGS)
{
    spgInnerConsistentIn *in = (spgInnerConsistentIn *) PG_GETARG_POINTER(0);
    spgInnerConsistentOut *out = (spgInnerConsistentOut *) PG_GETARG_POINTER(1);
    bool        collate_is_c = lc_collate_is_c(PG_GET_COLLATION());
    Kmer        *reconstructedValue;
    Kmer        *reconstrKmer;
    int         maxReconstrLen;
    text        *prefixText = NULL;
    int         prefixSize = 0;
    int         i;

    /*
     * Reconstruct values represented at this tuple, including parent data,
     * prefix of this tuple if any, and the node label if it's non-dummy.
     * in->level should be the length of the previously reconstructed value,
     * and the number of bytes added here is prefixSize or prefixSize + 1.
     */
    reconstructedValue = (Kmer *) DatumGetPointer(in->reconstructedValue);
    Assert(reconstructedValue == NULL ? in->level == 0 :
           strlen(reconstructedValue) == in->level);

    maxReconstrLen = in->level + 1;
    if (in->hasPrefix)
    {
         prefixText = DatumGetTextPP(in->prefixDatum);
         prefixSize = VARSIZE_ANY_EXHDR(prefixText);
         maxReconstrLen += prefixSize;
    }

    reconstrKmer = palloc(VARHDRSZ + maxReconstrLen);
    SET_VARSIZE(reconstrKmer, VARHDRSZ + maxReconstrLen);

    if (in->level)
        memcpy(reconstrKmer->sequence, reconstructedValue->sequence, in->level);
    if (prefixSize)
        memcpy(reconstrKmer->sequence + in->level, VARDATA_ANY(prefixText), prefixSize);
    // last byte of reconstrKmer will be filled in below

    /*
     * Scan the child nodes. For each one, complete the reconstructed value
     * and see if it's consistent with the query. If so, emit an entry into
     * the output arrays.
     */
    out->nodeNumbers = (int *) palloc(sizeof(int) * in->nNodes);
    out->levelAdds = (int *) palloc(sizeof(int) * in->nNodes);
    out->reconstructedValues = (Datum *) palloc(sizeof(Datum) * in->nNodes);
    out->nNodes = 0;

    for (i = 0; i < in->nNodes; i++)
    {
        int16       nodeChar = DatumGetInt16(in->nodeLabels[i]);
        int         thisLen;
        bool        res = true;
        int         j;

        /* If nodeChar is a dummy value, don't include it in data */
        if (nodeChar <= 0)
            thisLen = maxReconstrLen - 1;
        else
        {
            reconstrKmer->sequence[maxReconstrLen - 1] = nodeChar;
            thisLen = maxReconstrLen;
        }

        for (j = 0; j < in->nkeys; j++)
        {
            StrategyNumber strategy = in->scankeys[j].sk_strategy;
            Kmer *inKmer;
            int   inSize;
            int   r;

            /*
             * If it's a collation-aware operator, but the collation is C, we
             * can treat it as non-collation-aware. With non-C collation we
             * need to traverse the whole tree :-( so there's no point in making
             * any check here.
             */
            if (SPG_IS_COLLATION_AWARE_STRATEGY(strategy))
            {
                if (collate_is_c)
                    strategy -= SPG_STRATEGY_ADDITION;
                else
                    continue;
            }

            inKmer = (Kmer *) DatumGetPointer(in->scankeys[j].sk_argument);
            inSize = strlen(inKmer->sequence);

            r = memcmp(reconstrKmer->sequence, inKmer->sequence,
                       Min(inSize, thisLen));

            switch (strategy)
            {
                case BTLessStrategyNumber:
                case BTLessEqualStrategyNumber:
                    if (r > 0)
                        res = false;
                    break;
                case BTEqualStrategyNumber:
                    if (r != 0 || inSize < thisLen)
                        res = false;
                    break;
                case BTGreaterEqualStrategyNumber:
                case BTGreaterStrategyNumber:
                    if (r < 0)
                        res = false;
                    break;
                case RTPrefixStrategyNumber:
                    if (r != 0)
                        res = false;
                    break;
                default:
                    elog(ERROR, "unrecognized strategy number: %d",
                         in->scankeys[j].sk_strategy);
                    break;
            }

            if (!res)
                break;          /* no need to consider remaining conditions */
        }

        if (res)
        {
            out->nodeNumbers[out->nNodes] = i;
            out->levelAdds[out->nNodes] = thisLen - in->level;
            out->reconstructedValues[out->nNodes] =
                datumCopy(PointerGetDatum(reconstrKmer), false, -1);
            out->nNodes++;
        }
    }

    PG_RETURN_VOID();
}

PG_FUNCTION_INFO_V1(spg_kmer_leaf_consistent);
Datum
spg_kmer_leaf_consistent(PG_FUNCTION_ARGS)
{
    spgLeafConsistentIn *in = (spgLeafConsistentIn *) PG_GETARG_POINTER(0);
    spgLeafConsistentOut *out = (spgLeafConsistentOut *) PG_GETARG_POINTER(1);
    int         level = in->level;
    Kmer        *leafValue,
                *reconstrValue = NULL;
    char       *fullValue;
    int         fullLen;
    bool        res;
    int         j;

    /* all tests are exact */
    out->recheck = false;

    leafValue = (Kmer *) DatumGetPointer(in->leafDatum);

    /* As above, in->reconstructedValue isn't toasted or short. */
    if (DatumGetPointer(in->reconstructedValue))
        reconstrValue = (Kmer *) DatumGetPointer(in->reconstructedValue);

    Assert(reconstrValue == NULL ? level == 0 :
           strlen(reconstrValue->sequence) == level);

    /* Reconstruct the full Kmer represented by this leaf tuple */
    fullLen = level + strlen(leafValue->sequence);
    if (strlen(leafValue->sequence) == 0 && level > 0)
    {
        fullValue = reconstrValue->sequence;
        out->leafValue = PointerGetDatum(reconstrValue);
    }
    else
    {
        Kmer *fullKmer = palloc(VARHDRSZ + fullLen);

        SET_VARSIZE(fullKmer, VARHDRSZ + fullLen);
        fullValue = fullKmer->sequence;
        if (level)
            memcpy(fullValue, reconstrValue->sequence, level);
        if (strlen(leafValue->sequence) > 0)
            memcpy(fullValue + level, leafValue->sequence, strlen(leafValue->sequence));
        out->leafValue = PointerGetDatum(fullKmer);
    }

    /* Perform the required comparison(s) */
    res = true;
    for (j = 0; j < in->nkeys; j++)
    {
        StrategyNumber strategy = in->scankeys[j].sk_strategy;
        Kmer *query = (Kmer *) DatumGetPointer(in->scankeys[j].sk_argument);
        int queryLen = strlen(query->sequence);
        int r;

        if (strategy == RTPrefixStrategyNumber)
        {
            /*
             * If level >= length of query then reconstrValue must begin with
             * query (prefix) sequence, so we don't need to check it again.
             */
            res = (level >= queryLen) ||
                (memcmp(reconstrValue->sequence, query->sequence, queryLen) == 0);

            if (!res)           /* no need to consider remaining conditions */
                break;

            continue;
        }

        if (SPG_IS_COLLATION_AWARE_STRATEGY(strategy))
        {
            /* Collation-aware comparison */
            strategy -= SPG_STRATEGY_ADDITION;

            /* If asserts enabled, verify encoding of reconstructed sequence */
            Assert(pg_verifymbstr(fullValue, fullLen, false));

            r = memcmp(fullValue, query->sequence, Min(queryLen, fullLen));
        }
        else
        {
            /* Non-collation-aware comparison */
            r = memcmp(fullValue, query->sequence, Min(queryLen, fullLen));

            if (r == 0)
            {
                if (queryLen > fullLen)
                    r = -1;
                else if (queryLen < fullLen)
                    r = 1;
            }
        }

        switch (strategy)
        {
            case BTLessStrategyNumber:
                res = (r < 0);
                break;
            case BTLessEqualStrategyNumber:
                res = (r <= 0);
                break;
            case BTEqualStrategyNumber:
                res = (r == 0);
                break;
            case BTGreaterEqualStrategyNumber:
                res = (r >= 0);
                break;
            case BTGreaterStrategyNumber:
                res = (r > 0);
                break;
            default:
                elog(ERROR, "unrecognized strategy number: %d",
                     in->scankeys[j].sk_strategy);
                res = false;
                break;
        }

        if (!res)
            break;              /* no need to consider remaining conditions */
    }

    PG_RETURN_BOOL(res);
}
