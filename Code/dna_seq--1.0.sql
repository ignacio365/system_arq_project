-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION dna_seq" to load this file. \quit


/*DNA TYPE*/
/******************************************************************************
 * Input/Output
 ******************************************************************************/

CREATE OR REPLACE FUNCTION dna_in(cstring)
  RETURNS dna
  AS 'MODULE_PATHNAME'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;


CREATE OR REPLACE FUNCTION dna_out(dna)
  RETURNS cstring
  AS 'MODULE_PATHNAME'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION dna_recv(internal)
  RETURNS dna
  AS 'MODULE_PATHNAME'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION dna_send(dna)
  RETURNS bytea
  AS 'MODULE_PATHNAME'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE TYPE dna (
    INPUT = dna_in,
    OUTPUT = dna_out,
    RECEIVE = dna_recv,
    SEND = dna_send,
    INTERNALLENGTH = VARIABLE
);

COMMENT ON TYPE dna IS 'dna';

/******************************************************************************
 * Text <-> Dna functions
 ******************************************************************************/


/*This works as a constructor too*/

CREATE OR REPLACE FUNCTION dna(text)
  RETURNS dna
  AS 'MODULE_PATHNAME', 'dna_cast_from_text'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION text(dna)
  RETURNS text
  AS 'MODULE_PATHNAME', 'dna_cast_to_text'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE CAST (text as dna) WITH FUNCTION dna(text) AS IMPLICIT;
CREATE CAST (dna as text) WITH FUNCTION text(dna); 


/******************************************************************************
 Dna functions
 ******************************************************************************/


CREATE OR REPLACE FUNCTION size(dna)
  RETURNS integer
  AS 'MODULE_PATHNAME', 'dna_size'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;


CREATE OR REPLACE FUNCTION dna_len(dna)
  RETURNS integer
  AS 'MODULE_PATHNAME', 'dna_len'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

  /***************************************************************************************/
  /***************************************************************************************/
  /***************************************************************************************/

/*KMER TYPE*/
/******************************************************************************
 * Input/Output
 ******************************************************************************/

CREATE OR REPLACE FUNCTION kmer_in(cstring)
  RETURNS kmer
  AS 'MODULE_PATHNAME'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;


CREATE OR REPLACE FUNCTION kmer_out(kmer)
  RETURNS cstring
  AS 'MODULE_PATHNAME'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION kmer_recv(internal)
  RETURNS kmer
  AS 'MODULE_PATHNAME'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION kmer_send(kmer)
  RETURNS bytea
  AS 'MODULE_PATHNAME'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE TYPE kmer (
    INPUT = kmer_in,
    OUTPUT = kmer_out,
    RECEIVE = kmer_recv,
    SEND = kmer_send,
    INTERNALLENGTH = VARIABLE
);

COMMENT ON TYPE kmer IS 'kmer';

/******************************************************************************
 * Text <-> Kmer functions
 ******************************************************************************/


/*This works as a constructor too*/

CREATE OR REPLACE FUNCTION kmer(text)
  RETURNS kmer
  AS 'MODULE_PATHNAME', 'kmer_cast_from_text'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION text(kmer)
  RETURNS text
  AS 'MODULE_PATHNAME', 'kmer_cast_to_text'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE CAST (text as kmer) WITH FUNCTION kmer(text) AS IMPLICIT;
CREATE CAST (kmer as text) WITH FUNCTION text(kmer);


/******************************************************************************
 Kmer functions
 ******************************************************************************/


CREATE OR REPLACE FUNCTION size(kmer)
  RETURNS integer
  AS 'MODULE_PATHNAME', 'kmer_size'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;


CREATE OR REPLACE FUNCTION kmer_len(kmer)
  RETURNS integer
  AS 'MODULE_PATHNAME', 'kmer_len'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;


/******************************************************************************
 * Operators
 ******************************************************************************/

/*Equals function operator*/
CREATE OR REPLACE FUNCTION kmer_equals(kmer,kmer)
    RETURNS boolean
    AS 'MODULE_PATHNAME', 'kmer_equals'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;


 /*Equals operator*/
CREATE OPERATOR = (
    LEFTARG = kmer, RIGHTARG = kmer,
    PROCEDURE = kmer_equals
);

/*Starts with function operator*/
CREATE OR REPLACE FUNCTION starts_with(kmer, kmer)
    RETURNS boolean
    AS 'MODULE_PATHNAME', 'starts_with'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

/*Starts with operator*/
CREATE OPERATOR ^@ (
    LEFTARG = kmer, RIGHTARG = kmer,
    PROCEDURE = starts_with
);


  /***************************************************************************************/
  /***************************************************************************************/
  /***************************************************************************************/

  
/******************************************************************************
 FUNCTIONS
 ******************************************************************************/

CREATE OR REPLACE FUNCTION generate_kmers(IN dna, IN integer, OUT f kmer)
    RETURNS SETOF kmer
    AS 'MODULE_PATHNAME', 'generate_kmers'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;


 /***************************************************************************************/
  /***************************************************************************************/
  /***************************************************************************************/

/*QKMER TYPE*/
/******************************************************************************
 * Input/Output
 ******************************************************************************/

CREATE OR REPLACE FUNCTION qkmer_in(cstring)
  RETURNS qkmer
  AS 'MODULE_PATHNAME'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;


CREATE OR REPLACE FUNCTION qkmer_out(qkmer)
  RETURNS cstring
  AS 'MODULE_PATHNAME'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION qkmer_recv(internal)
  RETURNS qkmer
  AS 'MODULE_PATHNAME'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION qkmer_send(qkmer)
  RETURNS bytea
  AS 'MODULE_PATHNAME'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE TYPE qkmer (
    INPUT = qkmer_in,
    OUTPUT = qkmer_out,
    RECEIVE = qkmer_recv,
    SEND = qkmer_send,
    INTERNALLENGTH = VARIABLE
);

COMMENT ON TYPE qkmer IS 'qkmer';

/******************************************************************************
 * Text <-> qKmer functions
 ******************************************************************************/


/*This works as a constructor too*/

CREATE OR REPLACE FUNCTION qkmer(text)
  RETURNS qkmer
  AS 'MODULE_PATHNAME', 'qkmer_cast_from_text'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION text(qkmer)
  RETURNS text
  AS 'MODULE_PATHNAME', 'qkmer_cast_to_text'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE CAST (text as qkmer) WITH FUNCTION qkmer(text) AS IMPLICIT;
CREATE CAST (qkmer as text) WITH FUNCTION text(qkmer);

/******************************************************************************
 * Qkmer operators
 ******************************************************************************/
/*Qkmer lenght */ 
CREATE OR REPLACE FUNCTION qkmer_len(qkmer)
  RETURNS integer
  AS 'MODULE_PATHNAME', 'qkmer_len'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

/*contains function */
CREATE OR REPLACE FUNCTION contains(qkmer, kmer) 
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'contains'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

/*contains with operator*/
CREATE OPERATOR @> (
    LEFTARG = qkmer, RIGHTARG = kmer,
    PROCEDURE = contains
);

/******************************************************************************
 * Lenght functions for all the types
 ******************************************************************************/
CREATE FUNCTION length(dna) 
  RETURNS INT 
  AS 'MODULE_PATHNAME', 'dna_len'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION length(kmer) 
  RETURNS INT 
  AS 'MODULE_PATHNAME', 'kmer_len'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION length(qkmer) 
  RETURNS INT 
  AS 'MODULE_PATHNAME', 'qkmer_len'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;


/******************************************************************************
  AGREGATION
 ******************************************************************************/
CREATE FUNCTION kmer_hash(kmer) 
  RETURNS INT 
  AS 'MODULE_PATHNAME', 'kmer_hash'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR CLASS kmer_hash_ops
DEFAULT FOR TYPE Kmer USING hash AS
    OPERATOR 1 =,
    FUNCTION 1 kmer_hash(kmer);

/******************************************************************************
 INDEX
 ******************************************************************************/

CREATE OR REPLACE FUNCTION my_config(internal, internal) 
    RETURNS void
    AS 'MODULE_PATHNAME', 'spg_kmer_config'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION my_choose(internal, internal) 
    RETURNS void
    AS 'MODULE_PATHNAME', 'spg_kmer_choose'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION my_picksplit(internal, internal) 
    RETURNS void
    AS 'MODULE_PATHNAME', 'spg_kmer_picksplit'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION my_inner_consistent(internal, internal) 
    RETURNS void
    AS 'MODULE_PATHNAME', 'spg_kmer_inner_consistent'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION my_leaf_consistent(internal, internal) 
    RETURNS bool
    AS 'MODULE_PATHNAME', 'spg_kmer_leaf_consistent'
    LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
  

CREATE OPERATOR CLASS kmer_index_support
FOR TYPE kmer USING spgist
AS
        STORAGE kmer, 
        OPERATOR        1       =  (kmer, kmer) , 
        FUNCTION        1 my_config(internal, internal),
        FUNCTION        2 my_choose(internal, internal),
        FUNCTION        3 my_picksplit(internal, internal),
        FUNCTION        4 my_inner_consistent(internal, internal),
        FUNCTION        5 my_leaf_consistent(internal, internal);




