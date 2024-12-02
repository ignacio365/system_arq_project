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
-- Step 4. Printing t table and testing text, size, length and text functions for kmer and dna
DO $$ 
BEGIN
    RAISE NOTICE 'Step 4. Printing t table and testing text, size, length and text functions for kmer and dna';
END $$;

SELECT * FROM t;
SELECT dna, text(dna), dna(text(dna)), kmer(dna), dna(kmer(dna)), size(dna), length(dna) FROM t;
SELECT kmer, text(kmer), kmer(text(kmer)), dna(kmer), kmer(dna(kmer)), size(kmer), length(kmer) FROM t;

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

 SELECT *
 FROM generate_kmers((SELECT dna FROM t WHERE text(dna)='AGTTTTGAAAA'),2) AS k(kmer);
 
SELECT *
 FROM generate_kmers((SELECT dna(kmer) FROM t LIMIT 1), 2) AS k(kmer);

---------------------------------------------------------------------------------------
-- Step 6. Testing the kmer_equals function
DO $$ 
BEGIN
    RAISE NOTICE 'Step 6. Testing the kmer_equals function';
END $$;

-- Returns 2 tables with one row (using table t) - these 2 queries are equivalent
SELECT * FROM t WHERE kmer_equals('ACGT', kmer);
SELECT * FROM t WHERE kmer = 'ACGT';

-- Returns 2 tables with 0 row (using table t) - these 2 queries are equivalent
SELECT * FROM t WHERE kmer_equals('ACGTA', kmer);
SELECT * FROM t WHERE kmer = 'ACGTA';

 --------------------------------------------------------------------------------------
-- Step 7. Testing the starts_with function
DO $$ 
BEGIN
    RAISE NOTICE 'Step 7. Testing the starts_with function';
END $$;

-- Returns 2 tables with 2 rows (ACGT and ACGTC) - these 2 queries are equivalent
SELECT * FROM t WHERE starts_with('ACG', kmer);
SELECT * FROM t WHERE 'ACG' ^@ kmer;

 --------------------------------------------------------------------------------------
-- Step 8. Testing the contains function (works with qkmers too)
DO $$ 
BEGIN
    RAISE NOTICE 'Step 8. Testing the contains function';
END $$;

-- Returns a table with 0 row (no dna sequence contains "ANGTA") - these 2 queries are equivalent
SELECT * FROM t WHERE contains('ANGTA', kmer);
SELECT * FROM t where 'ANGTA' @>kmer;

-- Returns a table with 4 rows - these 2 queries are equivalent
SELECT * FROM t WHERE contains('ANGTC', kmer);
SELECT * FROM t where 'ANGTC' @>kmer;

-- Returns a table with 2 rows - these 2 queries are equivalent
SELECT * FROM t WHERE contains('ANGT', kmer);
SELECT * FROM t where 'ANGT' @>kmer;

-- Returns a table with 0 rows (no dna sequences with 2 nucleotides and A followed by any nucleotide) - these 2 queries are equivalent
SELECT * FROM t WHERE contains('AN', kmer);
SELECT * FROM t where 'AN' @>kmer;

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


-- Create the index
DROP INDEX IF EXISTS spgist_index;
CREATE INDEX spgist_index ON q USING spgist (kmer kmer_index_support);

SET enable_seqscan = OFF;

EXPLAIN (SELECT * FROM q WHERE 'ACGT'= kmer);
EXPLAIN (SELECT * FROM q WHERE  kmer ^@ 'ACG');
EXPLAIN (SELECT * FROM q WHERE  'ANGTA' @> kmer);







