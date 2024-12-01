
/*AUXILIAR FUNCTIONS */

#define SPGIST_MAX_PREFIX_LENGTH    Max((int) (BLCKSZ - 258 * 16 - 100), 32)
#define SPG_STRATEGY_ADDITION   (10)
#define SPG_IS_COLLATION_AWARE_STRATEGY(s) ((s) > SPG_STRATEGY_ADDITION \
                                          && (s) != RTPrefixStrategyNumber)

 typedef struct spgNodePtr
 {
     Datum       d;
     int         i;
     int16       c;
 } spgNodePtr;

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

/*Returns number of equal characters between two strings*/
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
  
 /* qsort comparator to sort spgNodePtr structs by "c" */
 static int
 cmpNodePtr(const void *a, const void *b)
 {
     const spgNodePtr *aa = (const spgNodePtr *) a;
     const spgNodePtr *bb = (const spgNodePtr *) b;
  
     /*return pg_cmp_s16(aa->c, bb->c);*/
         // Inline comparison for int16
    int16 diff = (aa->c > bb->c) - (aa->c < bb->c);
    return diff;

 }


/*ACTUAL FUNCTIONS*/

/*CONFIG FUNCTION*/
/*Att can be ignored, therefore all the in, because kmer is fixed*/
/*Leaf type being uninitialized (zero), index storage type derived from opckeytype. Same as operator input class.*/
PG_FUNCTION_INFO_V1(spgKmerConfig); /*Define static info*/
Datum 
spgKmerConfig(PG_FUNCTION_ARGS) {
    spgConfigOut *out = (spgConfigOut *) PG_GETARG_POINTER(1);
    out->prefixType = TEXTOID;      // Use text to represent prefixes
    out->labelType = INT2OID;       // Use char to represent node labels. It will be binary. (INT)
    out->canReturnData = true;      // Can reconstruct the full Kmer
    out->longValuesOK = true;       // Support long sequences  /* suffixing will shorten long values */
    PG_RETURN_VOID();
}


/*CHOOSE FUNCTION*/
/*Method for inserting new values into inner tuple */
/*We have a new value and we have to put it somewhere, there is a tuple. What do we do?*/
/*1. IF TUPLE HAS A PREFIX HANDLE IT*/
/*1.1 Prefix works with our word*/
/*1.2 Prefix does not match our word*/
/*2. IF NOT PREFIX ASSING VALUE OF NEXT NODE*/
/*3. ONCE WE HAVE OUR NEXT NODE LOOK HOW TO GO DOWN*/



PG_FUNCTION_INFO_V1(spgKmerChoose);  
Datum 
spgKmerChoose(PG_FUNCTION_ARGS) {
    spgChooseIn *in = (spgChooseIn *) PG_GETARG_POINTER(0);
    spgChooseOut *out = (spgChooseOut *) PG_GETARG_POINTER(1);
    const  text *kmer_text =  PG_GETARG_TEXT_PP(in->datum); /* in->datum is the original datum to be indexed */
    
    char *inStr= text_to_cstring(kmer_text); /*Retrieving the string and size without metadata (maybe wrong) THIS IS A POINTER TO THE INPUT STRING*/
    int inSize = VARSIZE_ANY_EXHDR(kmer_text);

    char *prefixStr = NULL;
    int prefixSize = 0; 
    int commonLen = 0; 
    int16 nodeChar = 0;
    int i = 0;


    /*STEP 1*/
    /* Check for prefix match, set nodeChar to first byte after prefix */
     if (in->hasPrefix)/*IF WE HAVE A PREFIX WE GO IN HERE IN THE INNER TUPLE THAT WE ARE LOOKING AT*/
     {
         text      *prefixText = DatumGetTextPP(in->prefixDatum);

         prefixStr = VARDATA_ANY(prefixText); /*THIS IS TUPLE PREFIX, NOT INPUT*/
         prefixSize = VARSIZE_ANY_EXHDR(prefixText);
  
         commonLen = commonPrefix(inStr + in->level,
                                  prefixStr,
                                  inSize - in->level,
                                  prefixSize);
        
        
        /*Know that we know how many character are the same we handle it, different cases*/
         /*STEP 1.1*/
         if (commonLen == prefixSize)
         {
             if (inSize - in->level > commonLen) /*Are there more input characters after the prefix?*/
                 nodeChar = *(unsigned char *) (inStr + in->level + commonLen);
             else
                 nodeChar = -1; /*If not we say -1*/
         }
         /*STEP 1.2*/
         else /*The prefix is longer than the common Len so we need to cut the prefix*/
         {
             /* Must split tuple because incoming value doesn't match prefix */
             out->resultType = spgSplitTuple; /* This action moves all the existing nodes into a new lower-level inner tuple*/
  
             if (commonLen == 0) /*Completely different*/
             {
                 out->result.splitTuple.prefixHasPrefix = false; /*The new tuple should be null prefixed*/
             }
             else /*Some equality. */
             {
                 out->result.splitTuple.prefixHasPrefix = true; /*We set the prefix to the longest common prefix possible*/
                 out->result.splitTuple.prefixPrefixDatum =
                     formTextDatum(prefixStr, commonLen);
             }

             out->result.splitTuple.prefixNNodes = 1; /*We will create one child node*/
             out->result.splitTuple.prefixNodeLabels =
                 (Datum *) palloc(sizeof(Datum));
             out->result.splitTuple.prefixNodeLabels[0] =
                 Int16GetDatum(*(unsigned char *) (prefixStr + commonLen)); /*Assigns first unmatched character in the prefix*/
  
             out->result.splitTuple.childNodeN = 0; /*Where will move old inner tuple*/
  
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
     /*STEP 2*/
     /*If not prefix and size bigger than level we just assign it to the next node*/
     else if (inSize > in->level)
     {
         nodeChar = *(unsigned char *) (inStr + in->level);
     }
     else
     {
         nodeChar = -1;
     }
  
     /* Look up nodeChar in the node label array */
     if (searchChar(in->nodeLabels, in->nNodes, nodeChar, &i)) /*If searchChar returns true, this means the character is already represented as a node, and the algorithm will proceed to descend into the matching child node.*/
     {
         /*
          * Descend to existing node.  (If in->allTheSame, the core code will
          * ignore our nodeN specification here, but that's OK.  We still have
          * to provide the correct levelAdd and restDatum values, and those are
          * the same regardless of which node gets chosen by core.)
          */
         int         levelAdd;
  
         out->resultType = spgMatchNode;
         out->result.matchNode.nodeN = i; /*Descend to this node*/
         levelAdd = commonLen;
         if (nodeChar >= 0)
             levelAdd++;
         out->result.matchNode.levelAdd = levelAdd; /*Update level*/
         if (inSize - in->level - levelAdd > 0) /*Here we are checking if there is more parts of the word left and if so adding them to a new leaf*/
             out->result.matchNode.restDatum =
                 formTextDatum(inStr + in->level + levelAdd,
                               inSize - in->level - levelAdd);
         else
             out->result.matchNode.restDatum =
                 formTextDatum(NULL, 0);
     }
     else if (in->allTheSame)
     {
         /*
          * Can't use AddNode action, so split the tuple.  The upper tuple has
          * the same prefix as before and uses a dummy node label -2 for the
          * lower tuple.  The lower tuple has no prefix and the same node
          * labels as the original tuple.
          *
          * Note: it might seem tempting to shorten the upper tuple's prefix,
          * if it has one, then use its last byte as label for the lower tuple.
          * But that doesn't win since we know the incoming value matches the
          * whole prefix: we'd just end up splitting the lower tuple again.
          */
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
         /* Add a node for the not-previously-seen nodeChar value and calls the function again */
         out->resultType = spgAddNode;
         out->result.addNode.nodeLabel = Int16GetDatum(nodeChar);
         out->result.addNode.nodeN = i;
     }
  
     PG_RETURN_VOID();
 }




/*Picksplit*/
/*1. Calculate longest prefix of set of leaf tuples*/
/*2. Set inner tuple that value*/
/*3. Extract the node label (first non-common byte) from each value */
/*4. Sort nodes*/
/*5. Send results*/

/*Decides how to create a new inner tuple over a set of leaf tuples.*/
PG_FUNCTION_INFO_V1(spgKmerPicksplit);
Datum
spgKmerPicksplit(PG_FUNCTION_ARGS)
 {
     spgPickSplitIn *in = (spgPickSplitIn *) PG_GETARG_POINTER(0);
     spgPickSplitOut *out = (spgPickSplitOut *) PG_GETARG_POINTER(1);
     text       *text0 = DatumGetTextPP(in->datums[0]); /*First leaf tuple*/
     int         i,
                 commonLen;
     spgNodePtr *nodes;


    /*1 STEP*/
     /* Identify longest common prefix, if any */
     commonLen = VARSIZE_ANY_EXHDR(text0);
     for (i = 1; i < in->nTuples && commonLen > 0; i++)
     {
         text       *texti = DatumGetTextPP(in->datums[i]);
         int         tmp = commonPrefix(VARDATA_ANY(text0),
                                        VARDATA_ANY(texti),
                                        VARSIZE_ANY_EXHDR(text0),
                                        VARSIZE_ANY_EXHDR(texti));
  
         if (tmp < commonLen) /*The shortest because common length is the max common length that all leaves share*/
             commonLen = tmp;
     }

     /*
      * Limit the prefix length, if necessary, to ensure that the resulting
      * inner tuple will fit on a page.
      */
     commonLen = Min(commonLen, SPGIST_MAX_PREFIX_LENGTH);
  

     /*STEP 2*/
     /* Set INNNER TUPLE prefix to be that string, if it's not empty */
     if (commonLen == 0)
     {
         out->hasPrefix = false;
     }
     else
     {
         out->hasPrefix = true;
         out->prefixDatum = formTextDatum(VARDATA_ANY(text0), commonLen);
     }
  
    /*STEP 3*/
     /* Extract the node label (first non-common byte) from each value */
     nodes = (spgNodePtr *) palloc(sizeof(spgNodePtr) * in->nTuples);
  
     for (i = 0; i < in->nTuples; i++)
     {
         text       *texti = DatumGetTextPP(in->datums[i]);
  
         if (commonLen < VARSIZE_ANY_EXHDR(texti))
             nodes[i].c = *(unsigned char *) (VARDATA_ANY(texti) + commonLen); /*Stores the first noncommon bit*/
         else
             nodes[i].c = -1;    /* use -1 if string is all common */
         nodes[i].i = i; /*Stores the node position*/
         nodes[i].d = in->datums[i]; /*Stores the actual data (WHY DO WE NEED THIS??)*/
     }
  

    /*STEP 4*/
     /*
      * Sort by label values so that we can group the values into nodes.  This
      * also ensures that the nodes are ordered by label value, allowing the
      * use of binary search in searchChar.
      */
     qsort(nodes, in->nTuples, sizeof(*nodes), cmpNodePtr);

     /*STEP 5*/
     /* And emit results */
     out->nNodes = 0;
     out->nodeLabels = (Datum *) palloc(sizeof(Datum) * in->nTuples);
     out->mapTuplesToNodes = (int *) palloc(sizeof(int) * in->nTuples);
     out->leafTupleDatums = (Datum *) palloc(sizeof(Datum) * in->nTuples);
  
     for (i = 0; i < in->nTuples; i++)
     {
         text       *texti = DatumGetTextPP(nodes[i].d);
         Datum       leafD;
  
         if (i == 0 || nodes[i].c != nodes[i - 1].c) /*Try to join leaves with same node*/
         {
             out->nodeLabels[out->nNodes] = Int16GetDatum(nodes[i].c);
             out->nNodes++;
         }
  
         if (commonLen < VARSIZE_ANY_EXHDR(texti)) /*Set leaf*/
             leafD = formTextDatum(VARDATA_ANY(texti) + commonLen + 1,
                                   VARSIZE_ANY_EXHDR(texti) - commonLen - 1);
         else
             leafD = formTextDatum(NULL, 0);
  
         out->leafTupleDatums[nodes[i].i] = leafD;
         out->mapTuplesToNodes[nodes[i].i] = out->nNodes - 1;
     }
  
     PG_RETURN_VOID();
 }
  










/*Returns set of nodes (branches) to follow during tree search*/
PG_FUNCTION_INFO_V1(spgKmerInnerConsistent);
Datum
spgKmerInnerConsistent(PG_FUNCTION_ARGS)
 {
     spgInnerConsistentIn *in = (spgInnerConsistentIn *) PG_GETARG_POINTER(0);
     spgInnerConsistentOut *out = (spgInnerConsistentOut *) PG_GETARG_POINTER(1);
     bool        collate_is_c = lc_collate_is_c(PG_GET_COLLATION());
     text       *reconstructedValue;
     text       *reconstrText;
     int         maxReconstrLen;
     text       *prefixText = NULL;
     int         prefixSize = 0;
     int         i;
  
     /*
      * Reconstruct values represented at this tuple, including parent data,
      * prefix of this tuple if any, and the node label if it's non-dummy.
      * in->level should be the length of the previously reconstructed value,
      * and the number of bytes added here is prefixSize or prefixSize + 1.
      *
      * Note: we assume that in->reconstructedValue isn't toasted and doesn't
      * have a short varlena header.  This is okay because it must have been
      * created by a previous invocation of this routine, and we always emit
      * long-format reconstructed values.
      */
     reconstructedValue = (text*) DatumGetPointer(in->reconstructedValue);
     Assert(reconstructedValue == NULL ? in->level == 0 :
            VARSIZE_ANY_EXHDR(reconstructedValue) == in->level);
  
     maxReconstrLen = in->level + 1;
     if (in->hasPrefix)
     {
         prefixText = DatumGetTextPP(in->prefixDatum);
         prefixSize = VARSIZE_ANY_EXHDR(prefixText);
         maxReconstrLen += prefixSize;
     }
  
     reconstrText = palloc(VARHDRSZ + maxReconstrLen);
     SET_VARSIZE(reconstrText, VARHDRSZ + maxReconstrLen);
  
     if (in->level)
         memcpy(VARDATA(reconstrText),
                VARDATA(reconstructedValue),
                in->level);
     if (prefixSize)
         memcpy(((char *) VARDATA(reconstrText)) + in->level,
                VARDATA_ANY(prefixText),
                prefixSize);
     /* last byte of reconstrText will be filled in below */
  
     /*
      * Scan the child nodes.  For each one, complete the reconstructed value
      * and see if it's consistent with the query.  If so, emit an entry into
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
             ((unsigned char *) VARDATA(reconstrText))[maxReconstrLen - 1] = nodeChar;
             thisLen = maxReconstrLen;
         }
  
         for (j = 0; j < in->nkeys; j++)
         {
             StrategyNumber strategy = in->scankeys[j].sk_strategy;
             text       *inText;
             int         inSize;
             int         r;
  
             /*
              * If it's a collation-aware operator, but the collation is C, we
              * can treat it as non-collation-aware.  With non-C collation we
              * need to traverse whole tree :-( so there's no point in making
              * any check here.  (Note also that our reconstructed value may
              * well end with a partial multibyte character, so that applying
              * any encoding-sensitive test to it would be risky anyhow.)
              */
             if (SPG_IS_COLLATION_AWARE_STRATEGY(strategy))
             {
                 if (collate_is_c)
                     strategy -= SPG_STRATEGY_ADDITION;
                 else
                     continue;
             }
  
             inText = DatumGetTextPP(in->scankeys[j].sk_argument);
             inSize = VARSIZE_ANY_EXHDR(inText);
  
             r = memcmp(VARDATA(reconstrText), VARDATA_ANY(inText),
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
             SET_VARSIZE(reconstrText, VARHDRSZ + thisLen);
             out->reconstructedValues[out->nNodes] =
                 datumCopy(PointerGetDatum(reconstrText), false, -1);
             out->nNodes++;
         }
     }
  
     PG_RETURN_VOID();
 }











/*Returns true if a leaf tuple satisfies a query.*/
PG_FUNCTION_INFO_V1(spgKmerLeafConsistent);
Datum
spgKmerLeafConsistent(PG_FUNCTION_ARGS)
 {
     spgLeafConsistentIn *in = (spgLeafConsistentIn *) PG_GETARG_POINTER(0); /*INFO needed to evaluate whether a leaf tuple satisfies the query conditions*/
     spgLeafConsistentOut *out = (spgLeafConsistentOut *) PG_GETARG_POINTER(1); /*Tools to retun the value*/
     
     int         level = in->level; /* current level (counting from zero) HERE IS 0 */
     text       *leafValue,
                *reconstrValue = NULL;
     char       *fullValue;
     int         fullLen;
     bool        res;
     int         j;
  
     out->recheck = false; /*WE PUT TRUE IF OUT INDEX DOES NOT STORE FULL INFO. IDK YET.*/
  
     leafValue = DatumGetTextPP(in->leafDatum); /*Data stored in the leaf (This is being evaluated)*/
  
     if (DatumGetPointer(in->reconstructedValue)) /* The value reconstructed from the parent node's prefix.*/
         reconstrValue = (text *) DatumGetPointer(in->reconstructedValue); /*If exists we get it*/
  
     Assert(reconstrValue == NULL ? level == 0 :
            VARSIZE_ANY_EXHDR(reconstrValue) == level); /*This is an if that raises an error if false, checks that if recVal==Null then level==0 and if it exists the size of the value should be the same as the level*/
  
     /* Reconstruct the full string represented by this leaf tuple */
     fullLen = level + VARSIZE_ANY_EXHDR(leafValue); /*Handle the size*/
     if (VARSIZE_ANY_EXHDR(leafValue) == 0 && level > 0) /*If leave value not size just return the previous value*/
     {
         fullValue = VARDATA(reconstrValue);
         out->leafValue = PointerGetDatum(reconstrValue);
     }
     else
     {
         text       *fullText = palloc(VARHDRSZ + fullLen); /*Allocate memory*/
         SET_VARSIZE(fullText, VARHDRSZ + fullLen); /*Set the memory to the header*/
         fullValue = VARDATA(fullText);
         if (level)
             memcpy(fullValue, VARDATA(reconstrValue), level); /*Copy reconsValue*/
         if (VARSIZE_ANY_EXHDR(leafValue) > 0)
             memcpy(fullValue + level, VARDATA_ANY(leafValue), /*Copy and append leaf value to reconsValue*/
                    VARSIZE_ANY_EXHDR(leafValue));
         out->leafValue = PointerGetDatum(fullText); 
     }
  
     /* Perform the required comparison(s) */
     res = true;
     for (j = 0; j < in->nkeys; j++) /*We are gonna iterate through the conditions of the query*/
     {
         StrategyNumber strategy = in->scankeys[j].sk_strategy; /* Identifies the operation type for this search condition*/
         text       *query = DatumGetTextPP(in->scankeys[j].sk_argument);
         int         queryLen = VARSIZE_ANY_EXHDR(query);
         int         r;
  
        /*STARTS WITH FUNCTION*/
         if (strategy == RTPrefixStrategyNumber) /*Indicates that the current search condition is a prefix match*/
         {
             /*
              * if level >= length of query then reconstrValue must begin with
              * query (prefix) string, so we don't need to check it again.
              */
             res = (level >= queryLen) ||
                 DatumGetBool(DirectFunctionCall2Coll(starts_with,
                                                      PG_GET_COLLATION(),
                                                      out->leafValue,
                                                      PointerGetDatum(query)));
  
             if (!res)           /* no need to consider remaining conditions */
                 break;
  
             continue;
         }

         /*THIS IS COLLATION = COMPARING TEXT = EQUALS = CONTAINS*/
        /*WE HAVE TO WORK HERE TO HAVE 2 OPTIONS??????*/
         if (SPG_IS_COLLATION_AWARE_STRATEGY(strategy))
         {
             /* Collation-aware comparison */
             strategy -= SPG_STRATEGY_ADDITION;
  
             /* If asserts enabled, verify encoding of reconstructed string */
             Assert(pg_verifymbstr(fullValue, fullLen, false));
  
             r = varstr_cmp(fullValue, fullLen,
                            VARDATA_ANY(query), queryLen,
                            PG_GET_COLLATION());
         }

         /*MAYBE EQUAL CAN BE HERE IDK*/
         else
         {
             /* Non-collation-aware comparison */
             r = memcmp(fullValue, VARDATA_ANY(query), Min(queryLen, fullLen));
  
             if (r == 0)
             {
                 if (queryLen > fullLen)
                     r = -1;
                 else if (queryLen < fullLen)
                     r = 1;
             }
         }
        
        /*THIS IS USED FOR DECIDING THE RESPONSE TRUE OR FALSE DEPENDING OF THE RESULTS BEFORE r TODO*/
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



















#define SPGIST_MAX_PREFIX_LENGTH    Max((int) (BLCKSZ - 258 * 16 - 100), 32)
#define SPG_STRATEGY_ADDITION   (10)
#define SPG_IS_COLLATION_AWARE_STRATEGY(s) ((s) > SPG_STRATEGY_ADDITION \
                                          && (s) != RTPrefixStrategyNumber)
  
#define DatumGetKmerPP(X)  ((Kmer *) DatumGetPointer(X))


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
     spgConfigOut *cfg = (spgConfigOut *) PG_GETARG_POINTER(1);
     cfg->prefixType = TEXTOID;
     cfg->labelType = INT2OID;
     cfg->canReturnData = true;
     cfg->longValuesOK = true;   
     PG_RETURN_VOID();
 }
  

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


static Datum
formKmerDatum(const char *data, int datalen)
{
    Kmer       *p;

    /* Allocate memory for the Kmer object, including additional headers if needed */
    p = (Kmer *) palloc(datalen + VARHDRSZ);

    /* Initialize the Kmer structure */
    if (datalen + VARHDRSZ_SHORT <= VARATT_SHORT_MAX) 
    {
        /* Set size for a short Kmer representation */
        SET_VARSIZE_SHORT(p, datalen + VARHDRSZ_SHORT);
        if (datalen)
        {
           p=kmer_parse(data);
        }
    }
    else
    {
        /* Set size for a standard Kmer representation */
         SET_VARSIZE(p, datalen + VARHDRSZ);
         p=kmer_parse(data);
    }

    /* Return the pointer as a Datum */
    return PointerGetDatum(p);
}

  
 /*
  * Find the length of the common prefix of a and b
  */
 static int
 commonPrefix(const char *a, const char *b, int lena, int lenb)
 {
     int         i = 0;
  
     while (i < lena && i < lenb && *a == *b)
     {
         a++;
         b++;
         i++;
     }
  
     return i;
 }
  
 /*
  * Binary search an array of int16 datums for a match to c
  *
  * On success, *i gets the match location; on failure, it gets where to insert
  */
 static bool
 searchChar(Datum *nodeLabels, int nNodes, int16 c, int *i)
 {
     int         StopLow = 0,
                 StopHigh = nNodes;
  
     while (StopLow < StopHigh)
     {
         int         StopMiddle = (StopLow + StopHigh) >> 1;
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
     Kmer       *inKmer = DatumGetKmerPP(in->datum);
     char       *inStr = inKmer->sequence;
     int         inSize = inKmer->size;
     char       *prefixStr = NULL;
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
  
         commonLen = commonPrefix(inStr + in->level,
                                  prefixStr,
                                  inSize - in->level,
                                  prefixSize);
  
         if (commonLen == prefixSize)
         {
             if (inSize - in->level > commonLen)
                 nodeChar = *(unsigned char *) (inStr + in->level + commonLen);
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
         nodeChar = *(unsigned char *) (inStr + in->level);
     }
     else
     {
         nodeChar = -1;
     }
  
     /* Look up nodeChar in the node label array */
     if (searchChar(in->nodeLabels, in->nNodes, nodeChar, &i))
     {
         /*
          * Descend to existing node.  (If in->allTheSame, the core code will
          * ignore our nodeN specification here, but that's OK.  We still have
          * to provide the correct levelAdd and restDatum values, and those are
          * the same regardless of which node gets chosen by core.)
          */
         int         levelAdd;
  
         out->resultType = spgMatchNode;
         out->result.matchNode.nodeN = i;
         levelAdd = commonLen;
         if (nodeChar >= 0)
             levelAdd++;
         out->result.matchNode.levelAdd = levelAdd;
         if (inSize - in->level - levelAdd > 0)
             out->result.matchNode.restDatum =
                 formKmerDatum(inStr + in->level + levelAdd,
                               inSize - in->level - levelAdd);
         else
             out->result.matchNode.restDatum =
                 formKmerDatum(NULL, 0);
     }
     else if (in->allTheSame)
     {
         /*
          * Can't use AddNode action, so split the tuple.  The upper tuple has
          * the same prefix as before and uses a dummy node label -2 for the
          * lower tuple.  The lower tuple has no prefix and the same node
          * labels as the original tuple.
          *
          * Note: it might seem tempting to shorten the upper tuple's prefix,
          * if it has one, then use its last byte as label for the lower tuple.
          * But that doesn't win since we know the incoming value matches the
          * whole prefix: we'd just end up splitting the lower tuple again.
          */
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
  
     return  (aa->c > bb->c) - (aa->c < bb->c);
 }
  
 PG_FUNCTION_INFO_V1(spg_kmer_picksplit);
 Datum
 spg_kmer_picksplit(PG_FUNCTION_ARGS)
 {
     spgPickSplitIn *in = (spgPickSplitIn *) PG_GETARG_POINTER(0);
     spgPickSplitOut *out = (spgPickSplitOut *) PG_GETARG_POINTER(1);
     Kmer      *text0 = DatumGetKmerPP(in->datums[0]);
     int         i,
                 commonLen;
     spgNodePtr *nodes;
  
     /* Identify longest common prefix, if any */
     commonLen = text0->size;
     for (i = 1; i < in->nTuples && commonLen > 0; i++)
     {
         Kmer       *texti = DatumGetKmerPP(in->datums[i]);
         int         tmp = commonPrefix(text0->sequence,
                                        texti->sequence,
                                        text0->size,
                                        texti->size);
  
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
         out->prefixDatum = formTextDatum(text0->sequence, commonLen);
     }
  
     /* Extract the node label (first non-common byte) from each value */
     nodes = (spgNodePtr *) palloc(sizeof(spgNodePtr) * in->nTuples);
  
     for (i = 0; i < in->nTuples; i++)
     {
         Kmer       *texti = DatumGetKmerPP(in->datums[i]);
  
         if (commonLen < texti->size)
             nodes[i].c = *(unsigned char *) (texti->sequence + commonLen);
         else
             nodes[i].c = -1;    /* use -1 if string is all common */
         nodes[i].i = i;
         nodes[i].d = in->datums[i];
     }
  
     /*
      * Sort by label values so that we can group the values into nodes.  This
      * also ensures that the nodes are ordered by label value, allowing the
      * use of binary search in searchChar.
      */
     qsort(nodes, in->nTuples, sizeof(*nodes), cmpNodePtr);
  
     /* And emit results */
     out->nNodes = 0;
     out->nodeLabels = (Datum *) palloc(sizeof(Datum) * in->nTuples);
     out->mapTuplesToNodes = (int *) palloc(sizeof(int) * in->nTuples);
     out->leafTupleDatums = (Datum *) palloc(sizeof(Datum) * in->nTuples);
  
     for (i = 0; i < in->nTuples; i++)
     {
         Kmer       *texti = DatumGetKmerPP(nodes[i].d);
         Datum       leafD;
  
         if (i == 0 || nodes[i].c != nodes[i - 1].c)
         {
             out->nodeLabels[out->nNodes] = Int16GetDatum(nodes[i].c);
             out->nNodes++;
         }
  
         if (commonLen < texti->size)
             leafD = formKmerDatum( texti->sequence + commonLen + 1,
                                   texti->size - commonLen - 1);
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
     Kmer       *reconstructedValue;
     text       *reconstrText;
     int         maxReconstrLen;
     text       *prefixText = NULL;
     int         prefixSize = 0;
     int         i;
  
     /*
      * Reconstruct values represented at this tuple, including parent data,
      * prefix of this tuple if any, and the node label if it's non-dummy.
      * in->level should be the length of the previously reconstructed value,
      * and the number of bytes added here is prefixSize or prefixSize + 1.
      *
      * Note: we assume that in->reconstructedValue isn't toasted and doesn't
      * have a short varlena header.  This is okay because it must have been
      * created by a previous invocation of this routine, and we always emit
      * long-format reconstructed values.
      */
     reconstructedValue = (Kmer *) DatumGetPointer(in->reconstructedValue);
     Assert(reconstructedValue == NULL ? in->level == 0 :
            reconstructedValue->size == in->level);
  
     maxReconstrLen = in->level + 1;
     if (in->hasPrefix)
     {
         prefixText = DatumGetTextPP(in->prefixDatum);
         prefixSize = VARSIZE_ANY_EXHDR(prefixText);
         maxReconstrLen += prefixSize;
     }
  
     reconstrText = palloc(VARHDRSZ + maxReconstrLen);
     SET_VARSIZE(reconstrText, VARHDRSZ + maxReconstrLen);
  
     if (in->level)
         memcpy(VARDATA(reconstrText),
                reconstructedValue->sequence,
                in->level);
     if (prefixSize)
         memcpy(((char *) VARDATA(reconstrText)) + in->level,
                VARDATA_ANY(prefixText),
                prefixSize);
     /* last byte of reconstrText will be filled in below */
  
     /*
      * Scan the child nodes.  For each one, complete the reconstructed value
      * and see if it's consistent with the query.  If so, emit an entry into
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
             ((unsigned char *) VARDATA(reconstrText))[maxReconstrLen - 1] = nodeChar;
             thisLen = maxReconstrLen;
         }
  
         for (j = 0; j < in->nkeys; j++)
         {
             StrategyNumber strategy = in->scankeys[j].sk_strategy;
             text       *inText;
             int         inSize;
             int         r;
  
             /*
              * If it's a collation-aware operator, but the collation is C, we
              * can treat it as non-collation-aware.  With non-C collation we
              * need to traverse whole tree :-( so there's no point in making
              * any check here.  (Note also that our reconstructed value may
              * well end with a partial multibyte character, so that applying
              * any encoding-sensitive test to it would be risky anyhow.)
              */
             if (SPG_IS_COLLATION_AWARE_STRATEGY(strategy))
             {
                 if (collate_is_c)
                     strategy -= SPG_STRATEGY_ADDITION;
                 else
                     continue;
             }
  
             inText = DatumGetTextPP(in->scankeys[j].sk_argument);
             inSize = VARSIZE_ANY_EXHDR(inText);
  
             r = memcmp(VARDATA(reconstrText), VARDATA_ANY(inText),
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
             SET_VARSIZE(reconstrText, VARHDRSZ + thisLen);
             out->reconstructedValues[out->nNodes] =
                 datumCopy(PointerGetDatum(reconstrText), false, -1);
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
     Kmer       *leafValue,
                *reconstrValue = NULL;
     char      *fullValue;
     int         fullLen;
     bool        res;
     int         j;
     char      *reconstrStr = NULL;
  
     /* all tests are exact */
     out->recheck = false;
  
     leafValue = DatumGetKmerPP(in->leafDatum);

     reconstrValue = (Kmer *) DatumGetPointer(in->reconstructedValue);
     reconstrStr = reconstrValue->sequence;
  
     Assert(reconstrValue == NULL ? level == 0 :
            reconstrValue->size == level);
  
     /* Reconstruct the full string represented by this leaf tuple */
     fullLen = level + leafValue->size;
     if (leafValue->size == 0 && level > 0)
     {
         fullValue = reconstrStr;
         out->leafValue = PointerGetDatum(reconstrValue);
     }
     else
     {
         Kmer       *fullText = palloc(VARHDRSZ + fullLen);
  
         SET_VARSIZE(fullText, VARHDRSZ + fullLen);
         fullValue = fullText->sequence;
         if (level)
             memcpy(fullValue, reconstrStr, level);
         if (leafValue->size > 0)
             memcpy(fullValue + level, leafValue->sequence,
                    leafValue->size);
         out->leafValue = PointerGetDatum(fullText);
     }
  
     /* Perform the required comparison(s) */
     res = true;
     for (j = 0; j < in->nkeys; j++)
     {
         StrategyNumber strategy = in->scankeys[j].sk_strategy;
         Kmer       *query = DatumGetKmerPP(in->scankeys[j].sk_argument);
         int         queryLen = VARSIZE_ANY_EXHDR(query);
         /*int         r;*/
  
         /*PREFIX MATCH*/
         if (strategy == RTPrefixStrategyNumber)
         {
             /*
              * if level >= length of query then reconstrValue must begin with
              * query (prefix) string, so we don't need to check it again.
              */
             res = (level >= queryLen) ||
                 DatumGetBool(DirectFunctionCall2Coll(starts_with,
                                                      PG_GET_COLLATION(),
                                                      out->leafValue,
                                                      PointerGetDatum(query)));
  
             if (!res)           /* no need to consider remaining conditions */
                 break;
  
             continue;
         }
  
        /* if (SPG_IS_COLLATION_AWARE_STRATEGY(strategy))
         {
            
             strategy -= SPG_STRATEGY_ADDITION;
  
            
             Assert(pg_verifymbstr(fullValue, fullLen, false));
  
             r = varstr_cmp(fullValue, fullLen,
                            VARDATA_ANY(query), queryLen,
                            PG_GET_COLLATION());
         }
         else
         {
             
             r = memcmp(fullValue, VARDATA_ANY(query), Min(queryLen, fullLen));
  
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
             break; */        
     }
  
     PG_RETURN_BOOL(res);
 }



#define DatumGetKmerPP(X) ((Kmer *) PG_DETOAST_DATUM_PACKED(X))
#define KmerPGetDatum(X) PointerGetDatum(X)


/* spg_kmer_config */
PG_FUNCTION_INFO_V1(spg_kmer_config);
Datum
spg_kmer_config(PG_FUNCTION_ARGS)
{
    spgConfigOut *out = (spgConfigOut *) PG_GETARG_POINTER(1);

    out->prefixType = TEXTOID;    /* Prefix is text */
    out->labelType = INT2OID;     /* Node labels are int2 */
    out->canReturnData = true;
    out->longValuesOK = false;

    PG_RETURN_VOID();
}

/* spg_kmer_choose */
PG_FUNCTION_INFO_V1(spg_kmer_choose);
Datum
spg_kmer_choose(PG_FUNCTION_ARGS)
{
    spgChooseIn *in = (spgChooseIn *) PG_GETARG_POINTER(0);
    spgChooseOut *out = (spgChooseOut *) PG_GETARG_POINTER(1);

    Kmer *kmer = DatumGetKmerPP(in->datum);

    if (in->hasPrefix)
    {
         text       *prefixText = DatumGetTextPP(in->prefixDatum);
         prefixStr = VARDATA_ANY(prefixText);
         prefixSize = VARSIZE_ANY_EXHDR(prefixText);
  
         commonLen = commonPrefix(kmer->sequence + in->level,
                                  prefixStr,
                                  kmer->size - in->level,
                                  prefixSize);
  
         if (commonLen == prefixSize)
         {
             if (kmer->size - in->level > commonLen)
                 nodeChar = *(unsigned char *) (kmer->sequence + in->level + commonLen);
             else
                 nodeChar = -1;
         }
         else
         {
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

    if (in->level < kmer->size)
    {
        char next_char = kmer->sequence[in->level];
        out->resultType = spgMatchNode;
        out->result.matchNode.nodeN = (int) next_char;
        out->result.matchNode.restDatum = KmerPGetDatum(kmer);
    }

    PG_RETURN_VOID();
}

/* spg_kmer_picksplit */
PG_FUNCTION_INFO_V1(spg_kmer_picksplit);
Datum
spg_kmer_picksplit(PG_FUNCTION_ARGS)
{
    spgPickSplitIn *in = (spgPickSplitIn *) PG_GETARG_POINTER(0);
    spgPickSplitOut *out = (spgPickSplitOut *) PG_GETARG_POINTER(1);
    int i;

    if (in->nTuples == 0)
    {
        out->nNodes = 0;
        PG_RETURN_VOID();
    }

    // Collect all distinct next characters
    bool labels_present[256] = {false};
    for (i = 0; i < in->nTuples; i++)
    {
        Kmer *kmer = DatumGetKmerPP(in->datums[i]);
        if (kmer->size > in->level)
        {
            unsigned char label = (unsigned char) kmer->sequence[in->level];
            labels_present[label] = true;
        }
        else
        {
            // Assign to a special node, e.g., label '\0'
            labels_present[0] = true;
        }
    }

    // Count the number of distinct labels
    int nLabels = 0;
    for (i = 0; i < 256; i++)
    {
        if (labels_present[i])
            nLabels++;
    }

    out->nNodes = nLabels;
    out->nodeLabels = palloc(sizeof(Datum) * nLabels);
    out->mapTuplesToNodes = palloc(sizeof(int) * in->nTuples);
    out->leafTupleDatums = palloc(sizeof(Datum) * in->nTuples);

    // Assign node labels
    int node_idx = 0;
    int label_to_node[256] = {0};
    for (i = 0; i < 256; i++)
    {
        if (labels_present[i])
        {
            out->nodeLabels[node_idx] = Int2GetDatum(i);
            label_to_node[i] = node_idx;
            node_idx++;
        }
    }

    // Assign tuples to nodes
    for (i = 0; i < in->nTuples; i++)
    {
        Kmer *kmer = DatumGetKmerPP(in->datums[i]);
        if (kmer->size > in->level)
        {
            unsigned char label = (unsigned char) kmer->sequence[in->level];
            out->mapTuplesToNodes[i] = label_to_node[label];
        }
        else
        {
            // Assign to a special node, e.g., label '\0'
            out->mapTuplesToNodes[i] = label_to_node[0];
        }
        out->leafTupleDatums[i] = in->datums[i];
    }

    PG_RETURN_VOID();
}

/* spg_kmer_inner_consistent */
PG_FUNCTION_INFO_V1(spg_kmer_inner_consistent);
Datum
spg_kmer_inner_consistent(PG_FUNCTION_ARGS)
{
    spgInnerConsistentIn *in = (spgInnerConsistentIn *) PG_GETARG_POINTER(0);
    spgInnerConsistentOut *out = (spgInnerConsistentOut *) PG_GETARG_POINTER(1);
    int level = in->level;
    Kmer *query = DatumGetKmerPP(in->scankeys[0].sk_argument);
    int query_len = query->size;

    StrategyNumber strategy = in->scankeys[0].sk_strategy;

    bool is_prefix = (strategy == RTPrefixStrategyNumber);
    bool is_equal = (strategy == BTEqualStrategyNumber);

    if (is_prefix || is_equal)
    {
        if (level < query_len)
        {
            unsigned char label = (unsigned char) query->sequence[level];
            // Find node with label
            for (int i = 0; i < in->nNodes; i++)
            {
                int node_label = DatumGetInt16(in->nodeLabels[i]);
                if (node_label == label)
                {
                    out->nodeNumbers[out->nNodes++] = i;
                    break;
                }
            }
        }
        else
        {
            // If the query prefix has been fully traversed, descend into all nodes
            for (int i = 0; i < in->nNodes; i++)
            {
                out->nodeNumbers[out->nNodes++] = i;
            }
        }
    }
    else
    {
        elog(ERROR, "spg_kmer_inner_consistent: unsupported strategy %d", strategy);
    }

    PG_RETURN_VOID();
}

/* spg_kmer_leaf_consistent */
PG_FUNCTION_INFO_V1(spg_kmer_leaf_consistent);
Datum
spg_kmer_leaf_consistent(PG_FUNCTION_ARGS)
{
    spgLeafConsistentIn *in = (spgLeafConsistentIn *) PG_GETARG_POINTER(0);
    spgLeafConsistentOut *out = (spgLeafConsistentOut *) PG_GETARG_POINTER(1);
    Kmer *leafValue = DatumGetKmerPP(in->leafDatum);
    Kmer *query = DatumGetKmerPP(in->scankeys[0].sk_argument);
    int query_len = query->size;

    StrategyNumber strategy = in->scankeys[0].sk_strategy;

    bool res = false;

    if (strategy == BTEqualStrategyNumber)
    {
        if (leafValue->size == query_len)
        {
            res = (strncmp(leafValue->sequence, query->sequence, leafValue->size) == 0);
        }
    }
    else if (strategy == RTPrefixStrategyNumber)
    {
        if (leafValue->size >= query_len)
        {
            res = (strncmp(leafValue->sequence, query->sequence, query_len) == 0);
        }
    }
    else
    {
        elog(ERROR, "spg_kmer_leaf_consistent: unsupported strategy %d", strategy);
    }

    out->recheck = false;
    PG_RETURN_BOOL(res);
}