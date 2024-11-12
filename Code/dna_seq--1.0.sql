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


CREATE OR REPLACE FUNCTION length(dna)
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


CREATE OR REPLACE FUNCTION length(kmer)
  RETURNS integer
  AS 'MODULE_PATHNAME', 'kmer_len'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

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

