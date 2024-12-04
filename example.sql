----------------------------------------------------------------------------------------------------------------
-- SQL SCRIPT TO CREATE THE dna_seq EXTENSION AND NECESSARY ATTRIBUTES, AND RUN SOME TESTS
----------------------------------------------------------------------------------------------------------------

-- Step 1. Set up environment and create extension
DO $$ 
BEGIN
    RAISE NOTICE 'Step 1. Set up environment and create extension';
END $$;

-- Drop the table if it exists (if script previously ran)
DROP TABLE IF EXISTS t;

-- Drop the extension if it exists (if script previously ran)
DROP EXTENSION IF EXISTS dna_seq CASCADE;

SELECT * FROM pg_available_extensions WHERE name = 'dna_seq';

-- Create the extension
CREATE EXTENSION dna_seq;

SELECT typname, typlen, typinput, typoutput, typreceive, typsend
FROM pg_type
WHERE typname = 'dna';

SELECT typname, typlen, typinput, typoutput, typreceive, typsend
FROM pg_type
WHERE typname = 'kmer';

SELECT typname, typlen, typinput, typoutput, typreceive, typsend
FROM pg_type
WHERE typname = 'qkmer';

-------------------------------------------------------------------------------------

-- Step 2. Create a table and insert 12 valid dna sequences 
DO $$ 
BEGIN
    RAISE NOTICE 'Step 2. Create a table and insert 12 valid dna sequences ';
END $$;


CREATE TABLE t (id integer, dna dna, kmer kmer);

INSERT INTO t VALUES
(1, 'ACGT','ACGT'),
(2, 'CT', 'CT'),
(3, 'AAA', 'AAA'),
(4, 'AGTTTTGAAAA','AGTTTTGAAAA'),
(5, 'ACGTC','ACGTC'),
(6, 'AAGTC','AAGTC'),
(7, 'AGGTC','AGGTC'),
(8, 'ATGTC','ATGTC'),
(9, 'ATGT','ATGT'),
(10, 'ATG','ATG'),
(11, 'ATG','ATG'),
(12, 'ATG','ATG');

--------------------------------------------------------------------------------------
-- Step 3. Checking that we respects the input rules
DO $$ 
BEGIN
    RAISE NOTICE 'Step 3. Checking that we respect the input rules';
END $$;

-- Error 1. "Invalid nucleotide 'B' in sequence.""
DO $$
BEGIN
    BEGIN
        INSERT INTO t VALUES (5, 'BTGE', 'BTGE');
    EXCEPTION WHEN OTHERS THEN
        RAISE NOTICE 'Error 1: Invalid nucleotide ''B'' in sequence.';
    END;
END $$;

-- Error 2. "Input array cannot be NULL or empty"
DO $$
BEGIN
    BEGIN
        INSERT INTO t VALUES (5, '', '');
    EXCEPTION WHEN OTHERS THEN
        RAISE NOTICE 'Error 2: Input array cannot be NULL or empty.';
    END;
END $$;

-- Error 3. "Input array cannot be longer than 32 nucleotides."
DO $$
BEGIN
    BEGIN
        INSERT INTO t VALUES (5, 'AGTTTTGAAAAAGTTTTGAAAAAGTTTTGAAAAAGTTTTGAAAA', 'AGTTTTGAAAAAGTTTTGAAAAAGTTTTGAAAAAGTTTTGAAAA');
    EXCEPTION WHEN OTHERS THEN
        RAISE NOTICE 'Error 3: Input array cannot be longer than 32 nucleotides.';
    END;
END $$;

--------------------------------------------------------------------------------------
-- Step 4.a. Printing t table and testing text, size, length and text functions for kmer and dna
DO $$ 
BEGIN
    RAISE NOTICE 'Step 4.a. Printing t table and testing text, size, length and text functions for kmer and dna';
END $$;

SELECT * FROM t;
/* Output:
 id |     dna     |    kmer     
----+-------------+-------------
  1 | ACGT        | ACGT
  2 | CT          | CT
  3 | AAA         | AAA
  4 | AGTTTTGAAAA | AGTTTTGAAAA
  5 | ACGTC       | ACGTC
  6 | AAGTC       | AAGTC
  7 | AGGTC       | AGGTC
  8 | ATGTC       | ATGTC
  9 | ATGT        | ATGT
 10 | ATG         | ATG
 11 | ATG         | ATG
 12 | ATG         | ATG
(12 rows)
*/
SELECT dna, text(dna), dna(text(dna)), kmer(dna), dna(kmer(dna)), size(dna), length(dna) FROM t;
/*Output
     dna     |    text     |     dna     |    kmer     |     dna     | size | length 
-------------+-------------+-------------+-------------+-------------+------+--------
 ACGT        | ACGT        | ACGT        | ACGT        | ACGT        |   36 |      4
 CT          | CT          | CT          | CT          | CT          |   28 |      2
 AAA         | AAA         | AAA         | AAA         | AAA         |   32 |      3
 AGTTTTGAAAA | AGTTTTGAAAA | AGTTTTGAAAA | AGTTTTGAAAA | AGTTTTGAAAA |   64 |     11
 ACGTC       | ACGTC       | ACGTC       | ACGTC       | ACGTC       |   40 |      5
 AAGTC       | AAGTC       | AAGTC       | AAGTC       | AAGTC       |   40 |      5
 AGGTC       | AGGTC       | AGGTC       | AGGTC       | AGGTC       |   40 |      5
 ATGTC       | ATGTC       | ATGTC       | ATGTC       | ATGTC       |   40 |      5
 ATGT        | ATGT        | ATGT        | ATGT        | ATGT        |   36 |      4
 ATG         | ATG         | ATG         | ATG         | ATG         |   32 |      3
 ATG         | ATG         | ATG         | ATG         | ATG         |   32 |      3
 ATG         | ATG         | ATG         | ATG         | ATG         |   32 |      3
(12 rows)
*/

SELECT kmer, text(kmer), kmer(text(kmer)), dna(kmer), kmer(dna(kmer)), size(kmer), length(kmer) FROM t;

/* Output
    kmer     |    text     |    kmer     |     dna     |    kmer     | size | length 
-------------+-------------+-------------+-------------+-------------+------+--------
 ACGT        | ACGT        | ACGT        | ACGT        | ACGT        |   36 |      4
 CT          | CT          | CT          | CT          | CT          |   28 |      2
 AAA         | AAA         | AAA         | AAA         | AAA         |   32 |      3
 AGTTTTGAAAA | AGTTTTGAAAA | AGTTTTGAAAA | AGTTTTGAAAA | AGTTTTGAAAA |   64 |     11
 ACGTC       | ACGTC       | ACGTC       | ACGTC       | ACGTC       |   40 |      5
 AAGTC       | AAGTC       | AAGTC       | AAGTC       | AAGTC       |   40 |      5
 AGGTC       | AGGTC       | AGGTC       | AGGTC       | AGGTC       |   40 |      5
 ATGTC       | ATGTC       | ATGTC       | ATGTC       | ATGTC       |   40 |      5
 ATGT        | ATGT        | ATGT        | ATGT        | ATGT        |   36 |      4
 ATG         | ATG         | ATG         | ATG         | ATG         |   32 |      3
 ATG         | ATG         | ATG         | ATG         | ATG         |   32 |      3
 ATG         | ATG         | ATG         | ATG         | ATG         |   32 |      3
(12 rows)
*/

--------------------------------------------------------------------------------------
-- Step 4.b. Create table, insert qkemrs, print it and test length, text for qkmer
DO $$ 
BEGIN
    RAISE NOTICE 'Step 4.b. Create table, insert qkemrs, print it and test length, text for qkmer';
END $$;

DROP TABLE IF EXISTS s;

CREATE TABLE s (id integer, dna dna, qkmer qkmer);

INSERT INTO s VALUES
(1, 'ACGT','ANGT'),
(2, 'CT', 'NT'),
(3, 'AAA', 'ANA'),
(4, 'AGTTTTGAAAA','AGTTTNNAAAA'),
(5, 'ACGTC','ANGTC'),
(6, 'AAGTC','ANNNC'),
(7, 'AGGTC','AGGNN'),
(8, 'ATGTC','ATGTN');

SELECT * FROM s;
/* Output
 id |     dna     |    qkmer    
----+-------------+-------------
  1 | ACGT        | ANGT
  2 | CT          | NT
  3 | AAA         | ANA
  4 | AGTTTTGAAAA | AGTTTNNAAAA
  5 | ACGTC       | ANGTC
  6 | AAGTC       | ANNNC
  7 | AGGTC       | AGGNN
  8 | ATGTC       | ATGTN
(8 rows)
*/

SELECT qkmer, text(qkmer), qkmer(text(qkmer)), length(qkmer) FROM s;
/*
    qkmer    |    text     |    qkmer    | length 
-------------+-------------+-------------+--------
 ANGT        | ANGT        | ANGT        |      4
 NT          | NT          | NT          |      2
 ANA         | ANA         | ANA         |      3
 AGTTTNNAAAA | AGTTTNNAAAA | AGTTTNNAAAA |     11
 ANGTC       | ANGTC       | ANGTC       |      5
 ANNNC       | ANNNC       | ANNNC       |      5
 AGGNN       | AGGNN       | AGGNN       |      5
 ATGTN       | ATGTN       | ATGTN       |      5
(8 rows)
*/

--------------------------------------------------------------------------------------
-- Step 5. Testing the generate_kmers function
DO $$ 
BEGIN
    RAISE NOTICE 'Step 5. Testing the generate_kmers function';
END $$;

 -- Testing error handling: k larger than the dna sequence
 DO $$
BEGIN
    BEGIN
         SELECT * FROM generate_kmers('ACGTACGT', 9) AS k(kmer);
    EXCEPTION WHEN OTHERS THEN
        RAISE NOTICE 'k must be between 1 and the length of the DNA sequence';
    END;
END $$;

-- Corrects use cases of the function
SELECT *
 FROM generate_kmers('ACGTACGT', 6) AS k(kmer);

 /* Output
  kmer  
--------
 ACGTAC
 CGTACG
 GTACGT
(3 rows)
 */

 SELECT *
 FROM generate_kmers((SELECT dna FROM t WHERE text(dna)='AGTTTTGAAAA'),2) AS k(kmer);
 
/* Output
 kmer 
------
 AG
 GT
 TT
 TT
 TT
 TG
 GA
 AA
 AA
 AA
(10 rows)
 */

SELECT *
 FROM generate_kmers((SELECT dna(kmer) FROM t LIMIT 1), 2) AS k(kmer);

 /* Output
 kmer 
------
 AC
 CG
 GT
(3 rows)
 */

---------------------------------------------------------------------------------------
-- Step 6. Testing the kmer_equals function
DO $$ 
BEGIN
    RAISE NOTICE 'Step 6. Testing the kmer_equals function';
END $$;

-- Returns 2 tables with one row (using table t) - these 2 queries are equivalent
SELECT * FROM t WHERE kmer_equals('ACGT', kmer);
SELECT * FROM t WHERE kmer = 'ACGT';

/* Output
 id | dna  | kmer 
----+------+------
  1 | ACGT | ACGT
(1 row)

 id | dna  | kmer 
----+------+------
  1 | ACGT | ACGT
(1 row)
*/

-- Returns 2 tables with 0 row (using table t) - these 2 queries are equivalent
SELECT * FROM t WHERE kmer_equals('ACGTA', kmer);
SELECT * FROM t WHERE kmer = 'ACGTA';

/* Output
 id | dna | kmer 
----+-----+------
(0 rows)

 id | dna | kmer 
----+-----+------
(0 rows)
*/

 --------------------------------------------------------------------------------------
-- Step 7. Testing the starts_with function
DO $$ 
BEGIN
    RAISE NOTICE 'Step 7. Testing the starts_with function';
END $$;

-- Returns 2 tables with 2 rows (ACGT and ACGTC) - these 2 queries are equivalent
SELECT * FROM t WHERE starts_with('ACG', kmer);
SELECT * FROM t WHERE 'ACG' ^@ kmer;

/* Output
 id |  dna  | kmer  
----+-------+-------
  1 | ACGT  | ACGT
  5 | ACGTC | ACGTC
(2 rows)

 id |  dna  | kmer  
----+-------+-------
  1 | ACGT  | ACGT
  5 | ACGTC | ACGTC
(2 rows)

*/
 --------------------------------------------------------------------------------------
-- Step 8. Testing the contains function (works with qkmers too)
DO $$ 
BEGIN
    RAISE NOTICE 'Step 8. Testing the contains function';
END $$;

-- Returns 2 tables with 0 row (no dna sequence contains "ANGTA") - these 2 queries are equivalent
SELECT * FROM t WHERE contains('ANGTA', kmer);
SELECT * FROM t where 'ANGTA' @>kmer;

/* Output
 id | dna | kmer 
----+-----+------
(0 rows)

 id | dna | kmer 
----+-----+------
(0 rows)
*/

-- Returns 2 tables with 4 rows - these 2 queries are equivalent
SELECT * FROM t WHERE contains('ANGTC', kmer);
SELECT * FROM t where 'ANGTC' @>kmer;

/* Output
 id |  dna  | kmer  
----+-------+-------
  5 | ACGTC | ACGTC
  6 | AAGTC | AAGTC
  7 | AGGTC | AGGTC
  8 | ATGTC | ATGTC
(4 rows)

 id |  dna  | kmer  
----+-------+-------
  5 | ACGTC | ACGTC
  6 | AAGTC | AAGTC
  7 | AGGTC | AGGTC
  8 | ATGTC | ATGTC
(4 rows)
*/

-- Returns 2 tables with 2 rows - these 2 queries are equivalent
SELECT * FROM t WHERE contains('ANGT', kmer);
SELECT * FROM t where 'ANGT' @>kmer;

/* Output
 id | dna  | kmer 
----+------+------
  1 | ACGT | ACGT
  9 | ATGT | ATGT
(2 rows)

 id | dna  | kmer 
----+------+------
  1 | ACGT | ACGT
  9 | ATGT | ATGT
(2 rows)
*/

-- Returns a table with 0 rows (no dna sequences with 2 nucleotides and A followed by any nucleotide) - these 2 queries are equivalent
SELECT * FROM t WHERE contains('AN', kmer);
SELECT * FROM t where 'AN' @>kmer;

/* Output
 id | dna | kmer 
----+-----+------
(0 rows)

 id | dna | kmer 
----+-----+------
(0 rows)
*/

 --------------------------------------------------------------------------------------
-- Step 9. Testing the kmer counting support
DO $$ 
BEGIN
    RAISE NOTICE 'Step 9. Testing the kmer counting support';
END $$;

-- Count all 4-mers in 'ACGTACGT' 
-- Returns a table with 4 rows 
SELECT k.kmer, count(*)
FROM generate_kmers('ACGTACGT', 4) AS k(kmer)
GROUP BY k.kmer
ORDER BY count(*) DESC;

/* Output
 kmer | count 
------+-------
 ACGT |     2
 CGTA |     1
 GTAC |     1
 TACG |     1
(4 rows)
*/

-- Return the total, distinct and unique count of 4-mers in 'ACGTACGT'
-- Returns a table with one row
WITH kmers AS (
SELECT k.kmer, count(*)
FROM generate_kmers('ACGTACGT', 4) AS k(kmer)
GROUP BY k.kmer
)
SELECT sum(count) AS total_count,
count(*) AS distinct_count,
count(*) FILTER (WHERE count = 1) AS unique_count
FROM kmers;

/* Output
 total_count | distinct_count | unique_count 
-------------+----------------+--------------
           5 |              4 |            3
(1 row)
*/

-- Return the total, distinct and unique count of 4-mers in 'ACGTACGT' and each kmer 
-- Returns a table with 4 rows 
WITH kmers AS (
SELECT k.kmer, count(*)
FROM generate_kmers('ACGTACGT', 4) AS k(kmer)
GROUP BY k.kmer
)
SELECT kmer, count AS total_count,
CASE 
   WHEN count IS NOT NULL THEN 1
   ELSE 0
END AS distinct_count,
CASE 
   WHEN count=1 THEN 1
   ELSE 0
END AS unique_count
FROM kmers;

/* Output
 kmer | total_count | distinct_count | unique_count 
------+-------------+----------------+--------------
 ACGT |           2 |              1 |            0
 CGTA |           1 |              1 |            1
 GTAC |           1 |              1 |            1
 TACG |           1 |              1 |            1
(4 rows)
*/

 --------------------------------------------------------------------------------------
-- Step 10. Testing the Indexes 
DO $$ 
BEGIN
    RAISE NOTICE 'Step 10. Testing the Indexes';
END $$;

-- Create new table with 1000000 rows of qkmers
DROP TABLE q;
CREATE TABLE q (
    kmer KMER
);

INSERT INTO q (kmer) 
SELECT 
    array_to_string(
        ARRAY(SELECT (array['A', 'T', 'C', 'G'])[floor(random() * 4 + 1)] FROM generate_series(1, 10)), 
        ''
    )
FROM generate_series(1, 1000000);

-- Prints the first 20 rows of the table
SELECT * FROM q LIMIT 20;

/* Output
    kmer    
------------
 TTCTCTTATC
 TTCTCTTATC
 TTCTCTTATC
 TTCTCTTATC
 TTCTCTTATC
 TTCTCTTATC
 TTCTCTTATC
 TTCTCTTATC
 TTCTCTTATC
 TTCTCTTATC
 TTCTCTTATC
 TTCTCTTATC
 TTCTCTTATC
 TTCTCTTATC
 TTCTCTTATC
 TTCTCTTATC
 TTCTCTTATC
 TTCTCTTATC
 TTCTCTTATC
 TTCTCTTATC
(20 rows)
*/

-- Create the index
DROP INDEX IF EXISTS spgist_index;
CREATE INDEX spgist_index ON q USING spgist (kmer kmer_index_support);

SET enable_seqscan = OFF;

EXPLAIN (SELECT * FROM q WHERE 'ACGT'= kmer);
EXPLAIN (SELECT * FROM q WHERE  kmer ^@ 'ACG');
EXPLAIN (SELECT * FROM q WHERE  'ANGTA' @> kmer);

/* Output
psql:example.sql:255: NOTICE:  index "spgist_index" does not exist, skipping
DROP INDEX
CREATE INDEX
SET
                                     QUERY PLAN                                      
-------------------------------------------------------------------------------------
 Bitmap Heap Scan on q  (cost=25305.28..43211.29 rows=500000 width=32)
   Filter: ('ACGT'::kmer = kmer)
   ->  Bitmap Index Scan on spgist_index  (cost=0.00..25180.28 rows=1000000 width=0)
(3 rows)

                                     QUERY PLAN                                     
------------------------------------------------------------------------------------
 Bitmap Heap Scan on q  (cost=13967.28..25623.28 rows=500000 width=32)
   Recheck Cond: (kmer ^@ 'ACG'::kmer)
   ->  Bitmap Index Scan on spgist_index  (cost=0.00..13842.28 rows=500000 width=0)
         Index Cond: (kmer ^@ 'ACG'::kmer)
(4 rows)

                                     QUERY PLAN                                      
-------------------------------------------------------------------------------------
 Bitmap Heap Scan on q  (cost=25305.28..43211.29 rows=500000 width=32)
   Filter: ('ANGTA'::qkmer @> kmer)
   ->  Bitmap Index Scan on spgist_index  (cost=0.00..25180.28 rows=1000000 width=0)
(3 rows)
*/







