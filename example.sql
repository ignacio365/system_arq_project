-- Drop the table if it exists
DROP TABLE IF EXISTS t;
-- Drop the extension if it exists
DROP EXTENSION IF EXISTS dna_seq;

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

CREATE TABLE t (id integer, dna dna, kmer kmer);

DROP TABLE q;
CREATE TABLE q (
    random_string KMER
);
INSERT INTO q (random_string)
SELECT 
    array_to_string(
        ARRAY(SELECT (array['A', 'T', 'C', 'G'])[floor(random() * 4 + 1)] FROM generate_series(1, 10)), 
        ''
    )
FROM generate_series(1, 1000000);

DROP INDEX spgist_index;
CREATE INDEX spgist_index ON q USING spgist (random_string kmer_index_support);
SET enable_seqscan = OFF;
SET enable_seqscan = ON;
EXPLAIN (SELECT * FROM q WHERE 'ACGT'= random_string);






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
(10, 'ATG','ATG');
(11, 'ATG','ATG');
(12, 'ATG','ATG');

/*Checking that it works*/
--INSERT INTO t VALUES (5, 'BTGE', 'BTGE');

--INSERT INTO t VALUES (5, '', '');

--INSERT INTO t VALUES (5, 'AGTTTTGAAAAAGTTTTGAAAAAGTTTTGAAAAAGTTTTGAAAA', 'AGTTTTGAAAAAGTTTTGAAAAAGTTTTGAAAAAGTTTTGAAAA');

SELECT * FROM t;
SELECT dna, text(dna), dna(text(dna)), size(dna), length(dna) FROM t;
SELECT kmer, text(kmer), kmer(text(kmer)), size(kmer), length(kmer) FROM t;

SELECT *
 FROM generate_kmers('ACGTACGT', 6) AS k(kmer);


 SELECT *
 FROM generate_kmers((SELECT dna FROM t WHERE text(dna)='AGTTTTGAAAA'),2);


-- testing the equals
-- this should get one row (using the table created using this script)
SELECT * FROM t WHERE kmer_equals('ACGT', kmer);
SELECT * FROM t WHERE kmer = 'ACGT';

-- this should get empty (using the table created using this script)
SELECT * FROM t WHERE kmer_equals('ACGTA', kmer);
SELECT * FROM t WHERE kmer = 'ACGTA';


-- These two queries are equivalent:
SELECT * FROM t WHERE starts_with('ACG', kmer);
-- SELECT * FROM t WHERE  kmer^@ 'ACG'; this is what is in the report
SELECT * FROM t WHERE 'ACG' ^@ kmer;

SELECT * FROM t;

CREATE INDEX spgist_index ON t USING spgist (kmer kmer_index_support);
SET enable_seqscan = OFF;
SET enable_seqscan = ON;
EXPLAIN (SELECT * FROM t WHERE 'ACGT'= kmer);


-- CONTAINS FUNCTION TEST
SELECT * FROM t WHERE contains('ANGTC', kmer);
SELECT * FROM t where 'ANGTC' @>kmer;

SELECT * FROM t WHERE contains('ANGT', kmer);
SELECT * FROM t where 'ANGT' @>kmer;

SELECT * FROM t WHERE contains('ANG', kmer);
SELECT * FROM t where 'ANG' @>kmer;

SELECT * FROM t WHERE contains('AN', kmer);
SELECT * FROM t where 'AN' @>kmer;

----------------

SELECT k.kmer, count(*)
FROM generate_kmers('ACGTACGT', 4) AS k(kmer)
GROUP BY k.kmer
ORDER BY count(*) DESC;

WITH kmers AS (
SELECT k.kmer, count(*)
FROM generate_kmers('ACGTACGT', 4) AS k(kmer)
GROUP BY k.kmer
)
SELECT sum(count) AS total_count,
count(*) AS distinct_count,
count(*) FILTER (WHERE count = 1) AS unique_count
FROM kmers;


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

 



