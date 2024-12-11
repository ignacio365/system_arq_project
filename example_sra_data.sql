----------------------------------------------------------------------------------------------------------------
-- SQL SCRIPT TO TEST THE dna_seq EXTENSION with data from the SRA DATABASE (ncbi)
-----------------------------------------------------------------------------------------------------------

-- Step 1: 
-- Follow the instructions in the READ ME to restore the database "ncbi" from the "sra_backup.tar" file in the zip-folder.
-- The database comes with 3 tables with qkmer-, kmer- and dna sequences and the extension already connected.

-- THE QUERIES

SELECT typname, typlen, typinput, typoutput, typreceive, typsend
FROM pg_type
WHERE typname = 'dna';

--  typname | typlen | typinput | typoutput | typreceive | typsend
-- ---------+--------+----------+-----------+------------+----------
--  dna     |     -1 | dna_in   | dna_out   | dna_recv   | dna_send
-- (1 Zeile) -> Excuse my computer, this is german for row


SELECT typname, typlen, typinput, typoutput, typreceive, typsend
FROM pg_type
WHERE typname = 'kmer';

--  typname | typlen | typinput | typoutput | typreceive |  typsend
-- ---------+--------+----------+-----------+------------+-----------
--  kmer    |     -1 | kmer_in  | kmer_out  | kmer_recv  | kmer_send
-- (1 Zeile)

SELECT typname, typlen, typinput, typoutput, typreceive, typsend
FROM pg_type
WHERE typname = 'qkmer';

--  typname | typlen | typinput | typoutput | typreceive |  typsend
-- ---------+--------+----------+-----------+------------+------------
--  qkmer   |     -1 | qkmer_in | qkmer_out | qkmer_recv | qkmer_send
-- (1 Zeile)

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
        INSERT INTO dna_sequences VALUES (5, 'BTGE', 'BTGE');
    EXCEPTION WHEN OTHERS THEN
        RAISE NOTICE 'Error 1: Invalid nucleotide ''B'' in sequence.';
    END;
END $$;

-- Error 2. "Input array cannot be NULL or empty"
DO $$
BEGIN
    BEGIN
        INSERT INTO dna_sequences VALUES (5, '', '');
    EXCEPTION WHEN OTHERS THEN
        RAISE NOTICE 'Error 2: Input array cannot be NULL or empty.';
    END;
END $$;

-- Error 3. "Input array cannot be longer than 32 nucleotides."
DO $$
BEGIN
    BEGIN
        INSERT INTO kmer_sequences VALUES (5, 'AGTTTTGAAAAAGTTTTGAAAAAGTTTTGAAAAAGTTTTGAAAA', 'AGTTTTGAAAAAGTTTTGAAAAAGTTTTGAAAAAGTTTTGAAAA');
    EXCEPTION WHEN OTHERS THEN
        RAISE NOTICE 'Error 3: Input array cannot be longer than 32 nucleotides.';
    END;
END $$;

--------------------------------------------------------------------------------------
-- Step 4.a. Printing the first 10 rows of the dna and kmer table and testing text, size, length and text functions for kmer and dna
DO $$ 
BEGIN
    RAISE NOTICE 'Step 4.a. Printing the dna and kmer table and testing text, size, length and text functions for kmer and dna';
END $$;

SELECT * FROM dna_sequences LIMIT 10;
/* Output:
                                   dna
--------------------------------------------------------------------------
 AAGTAGGTCTCGTCTGTGTTTTCTACGAGCTTGTGTTCCAGCTGACCCACTCCCTGGGTGGGGGGACTGGGT
 CCAGCCTGGCCAACAGAGTGTTACCCCGTTTTTACTTATTTATTATTATTATTTTGAGACAGAGCATTGGTC
 ATAAAATCAGGGGTGTTGGAGATGGGATGCCTATTTCTGCACACCTTGGCCTCCCAAATTGCTGGGATTACA
 TTAAGAAATTTTTGCTCAAACCATGCCCTAAAGGGTTCTGTAATAAATAGGGCTGGGAAAACTGGCAAGCCA
 TCTTATAACAATTTTCCACTCATTTGTGCCAATTTTGTGATGTGCAAGATTCTGCTTCAACTTTTGCCATGA
 GCTTTTCTACTTTTCAATAAAACCTTCTTTATTTTTGTGTTTCACCATGTTGGTCAGGCTTATCTCTAATTC
 AGAACCTAGAAATAAGGCCAAATACTTACAACCAACTTTCTTCTAGAATTTTTATGGTTTTTTTTTTTATAT
 TTCCTTTGCTCTGGGAAGAAGTCTTAACTTCCTTTGGGACGGTAGGGGTTGGAGCCACAGTGAGTCTTACAC
 TAAAAACCTTGAAAAAAGATTAGACGGATGGCTAACAAGACTTTGCTCATTTCTTTTTACTCTCTTTTCTCT
 GAATGAAATAAATCCACAAGAGGAGTTTAGAGAAAATCTGTATTTCCTGAATCTGAATGTTGGCCTGCCTTG
(10 Zeilen)
*/

SELECT dna, text(dna), dna(text(dna)), size(dna), length(dna) FROM dna_sequences LIMIT 10;
/*Output
                                  dna                                    |                                   text                                   |                                   dna                                    | size | length 
 ATAAAATCAGGGGTGTTGGAGATGGGATGCCTATTTCTGCACACCTTGGCCTCCCAAATTGCTGGGATTACA | ATAAAATCAGGGGTGTTGGAGATGGGATGCCTATTTCTGCACACCTTGGCCTCCCAAATTGCTGGGATTACA | ATAAAATCAGGGGTGTTGGAGATGGGATGCCTATTTCTGCACACCTTGGCCTCCCAAATTGCTGGGATTACA |  308 |     72
 TTAAGAAATTTTTGCTCAAACCATGCCCTAAAGGGTTCTGTAATAAATAGGGCTGGGAAAACTGGCAAGCCA | TTAAGAAATTTTTGCTCAAACCATGCCCTAAAGGGTTCTGTAATAAATAGGGCTGGGAAAACTGGCAAGCCA | TTAAGAAATTTTTGCTCAAACCATGCCCTAAAGGGTTCTGTAATAAATAGGGCTGGGAAAACTGGCAAGCCA |  308 |     72
 TCTTATAACAATTTTCCACTCATTTGTGCCAATTTTGTGATGTGCAAGATTCTGCTTCAACTTTTGCCATGA | TCTTATAACAATTTTCCACTCATTTGTGCCAATTTTGTGATGTGCAAGATTCTGCTTCAACTTTTGCCATGA | TCTTATAACAATTTTCCACTCATTTGTGCCAATTTTGTGATGTGCAAGATTCTGCTTCAACTTTTGCCATGA |  308 |     72
 GCTTTTCTACTTTTCAATAAAACCTTCTTTATTTTTGTGTTTCACCATGTTGGTCAGGCTTATCTCTAATTC | GCTTTTCTACTTTTCAATAAAACCTTCTTTATTTTTGTGTTTCACCATGTTGGTCAGGCTTATCTCTAATTC | GCTTTTCTACTTTTCAATAAAACCTTCTTTATTTTTGTGTTTCACCATGTTGGTCAGGCTTATCTCTAATTC |  308 |     72
 AGAACCTAGAAATAAGGCCAAATACTTACAACCAACTTTCTTCTAGAATTTTTATGGTTTTTTTTTTTATAT | AGAACCTAGAAATAAGGCCAAATACTTACAACCAACTTTCTTCTAGAATTTTTATGGTTTTTTTTTTTATAT | AGAACCTAGAAATAAGGCCAAATACTTACAACCAACTTTCTTCTAGAATTTTTATGGTTTTTTTTTTTATAT |  308 |     72
 TTCCTTTGCTCTGGGAAGAAGTCTTAACTTCCTTTGGGACGGTAGGGGTTGGAGCCACAGTGAGTCTTACAC | TTCCTTTGCTCTGGGAAGAAGTCTTAACTTCCTTTGGGACGGTAGGGGTTGGAGCCACAGTGAGTCTTACAC | TTCCTTTGCTCTGGGAAGAAGTCTTAACTTCCTTTGGGACGGTAGGGGTTGGAGCCACAGTGAGTCTTACAC |  308 |     72
 TAAAAACCTTGAAAAAAGATTAGACGGATGGCTAACAAGACTTTGCTCATTTCTTTTTACTCTCTTTTCTCT | TAAAAACCTTGAAAAAAGATTAGACGGATGGCTAACAAGACTTTGCTCATTTCTTTTTACTCTCTTTTCTCT | TAAAAACCTTGAAAAAAGATTAGACGGATGGCTAACAAGACTTTGCTCATTTCTTTTTACTCTCTTTTCTCT |  308 |     72
 GAATGAAATAAATCCACAAGAGGAGTTTAGAGAAAATCTGTATTTCCTGAATCTGAATGTTGGCCTGCCTTG | GAATGAAATAAATCCACAAGAGGAGTTTAGAGAAAATCTGTATTTCCTGAATCTGAATGTTGGCCTGCCTTG | GAATGAAATAAATCCACAAGAGGAGTTTAGAGAAAATCTGTATTTCCTGAATCTGAATGTTGGCCTGCCTTG |  308 |     72
(10 Zeilen)
*/

SELECT * FROM kmer_sequences LIMIT 10;
-- output
--                kmer
-- ----------------------------------
--  CATTCTTCACGTAGTTCTCGAGCCTTGGTTTT
--  CAGCGATGGAGAATGACTTTGACAAGCTGAGA
--  GGAGAATTGCTTGAACCTGGGAGCCAGAGGTT
--  GGGAGAAT
--  TTTTTGAGCAGCAGCAAGATTTATTGTGAAGA
--  TGGATATAACACATTTTGTTTATCCATTCATC
--  AGTTATTGTTAGTCATCAGTCACTGTAGAAAC
--  CTACATTTGATACCTAAACATTCTAATTTCCT
--  ATACATTTATTAAACAATGTTAAGACATTAAA
--  AGCCCAACCCCGTAGGCCATGAGGGAGGGGCA
-- (10 Zeilen)

SELECT kmer, text(kmer), kmer(text(kmer)), dna(kmer), kmer(dna(kmer)), size(kmer), length(kmer) FROM kmer_sequences LIMIT 10;

/* Output
               kmer               |               text               |               kmer               |               dna                |               kmer               | size | length
----------------------------------+----------------------------------+----------------------------------+----------------------------------+----------------------------------+------+--------
 CATTCTTCACGTAGTTCTCGAGCCTTGGTTTT | CATTCTTCACGTAGTTCTCGAGCCTTGGTTTT | CATTCTTCACGTAGTTCTCGAGCCTTGGTTTT | CATTCTTCACGTAGTTCTCGAGCCTTGGTTTT | CATTCTTCACGTAGTTCTCGAGCCTTGGTTTT |  148 |     32
 CAGCGATGGAGAATGACTTTGACAAGCTGAGA | CAGCGATGGAGAATGACTTTGACAAGCTGAGA | CAGCGATGGAGAATGACTTTGACAAGCTGAGA | CAGCGATGGAGAATGACTTTGACAAGCTGAGA | CAGCGATGGAGAATGACTTTGACAAGCTGAGA |  148 |     32
 GGAGAATTGCTTGAACCTGGGAGCCAGAGGTT | GGAGAATTGCTTGAACCTGGGAGCCAGAGGTT | GGAGAATTGCTTGAACCTGGGAGCCAGAGGTT | GGAGAATTGCTTGAACCTGGGAGCCAGAGGTT | GGAGAATTGCTTGAACCTGGGAGCCAGAGGTT |  148 |     32
 GGGAGAAT                         | GGGAGAAT                         | GGGAGAAT                         | GGGAGAAT                         | GGGAGAAT                         |   52 |      8
 TTTTTGAGCAGCAGCAAGATTTATTGTGAAGA | TTTTTGAGCAGCAGCAAGATTTATTGTGAAGA | TTTTTGAGCAGCAGCAAGATTTATTGTGAAGA | TTTTTGAGCAGCAGCAAGATTTATTGTGAAGA | TTTTTGAGCAGCAGCAAGATTTATTGTGAAGA |  148 |     32
 TGGATATAACACATTTTGTTTATCCATTCATC | TGGATATAACACATTTTGTTTATCCATTCATC | TGGATATAACACATTTTGTTTATCCATTCATC | TGGATATAACACATTTTGTTTATCCATTCATC | TGGATATAACACATTTTGTTTATCCATTCATC |  148 |     32
 AGTTATTGTTAGTCATCAGTCACTGTAGAAAC | AGTTATTGTTAGTCATCAGTCACTGTAGAAAC | AGTTATTGTTAGTCATCAGTCACTGTAGAAAC | AGTTATTGTTAGTCATCAGTCACTGTAGAAAC | AGTTATTGTTAGTCATCAGTCACTGTAGAAAC |  148 |     32
 CTACATTTGATACCTAAACATTCTAATTTCCT | CTACATTTGATACCTAAACATTCTAATTTCCT | CTACATTTGATACCTAAACATTCTAATTTCCT | CTACATTTGATACCTAAACATTCTAATTTCCT | CTACATTTGATACCTAAACATTCTAATTTCCT |  148 |     32
 ATACATTTATTAAACAATGTTAAGACATTAAA | ATACATTTATTAAACAATGTTAAGACATTAAA | ATACATTTATTAAACAATGTTAAGACATTAAA | ATACATTTATTAAACAATGTTAAGACATTAAA | ATACATTTATTAAACAATGTTAAGACATTAAA |  148 |     32
 AGCCCAACCCCGTAGGCCATGAGGGAGGGGCA | AGCCCAACCCCGTAGGCCATGAGGGAGGGGCA | AGCCCAACCCCGTAGGCCATGAGGGAGGGGCA | AGCCCAACCCCGTAGGCCATGAGGGAGGGGCA | AGCCCAACCCCGTAGGCCATGAGGGAGGGGCA |  148 |     32
(10 Zeilen)
*/

--------------------------------------------------------------------------------------
-- Step 4.b. Print the qkmer table and test length, text for qkmer

SELECT * FROM qkmer_sequences LIMIT 10;
/* Output
              qkmer
----------------------------------
 GAAGNTNC
 GAAGGCTCGGGGAAGNAGTAGTGGGGGGAGGA
 GCGAATTGAATGGCTGTTAGTTANTNNNNNNN
 NNNNNNNN
 CTGTNCNT
 TCCTAAAGTCTGAAAGATGAGGAAANNANGAA
 TCNNCNNN
 TGAAAAAATTACAATGGCTTGTCNTNNNNNNN
 NNNNNNNN
 TCANNNNT
(10 Zeilen)
*/

SELECT qkmer, text(qkmer), qkmer(text(qkmer)), length(qkmer) FROM qkmer_sequences LIMIT 10;
/*
              qkmer               |               text               |              qkmer               | length
----------------------------------+----------------------------------+----------------------------------+--------
 GAAGNTNC                         | GAAGNTNC                         | GAAGNTNC                         |      8
 GAAGGCTCGGGGAAGNAGTAGTGGGGGGAGGA | GAAGGCTCGGGGAAGNAGTAGTGGGGGGAGGA | GAAGGCTCGGGGAAGNAGTAGTGGGGGGAGGA |     32
 GCGAATTGAATGGCTGTTAGTTANTNNNNNNN | GCGAATTGAATGGCTGTTAGTTANTNNNNNNN | GCGAATTGAATGGCTGTTAGTTANTNNNNNNN |     32
 NNNNNNNN                         | NNNNNNNN                         | NNNNNNNN                         |      8
 CTGTNCNT                         | CTGTNCNT                         | CTGTNCNT                         |      8
 TCCTAAAGTCTGAAAGATGAGGAAANNANGAA | TCCTAAAGTCTGAAAGATGAGGAAANNANGAA | TCCTAAAGTCTGAAAGATGAGGAAANNANGAA |     32
 TCNNCNNN                         | TCNNCNNN                         | TCNNCNNN                         |      8
 TGAAAAAATTACAATGGCTTGTCNTNNNNNNN | TGAAAAAATTACAATGGCTTGTCNTNNNNNNN | TGAAAAAATTACAATGGCTTGTCNTNNNNNNN |     32
 NNNNNNNN                         | NNNNNNNN                         | NNNNNNNN                         |      8
 TCANNNNT                         | TCANNNNT                         | TCANNNNT                         |      8
(10 Zeilen)
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
         SELECT * FROM generate_kmers((SELECT dna FROM dna_sequences LIMIT 1), 9) AS k(kmer);
    EXCEPTION WHEN OTHERS THEN
        RAISE NOTICE 'k must be between 1 and the length of the DNA sequence';
    END;
END $$;

-- Correct use cases of the function
SELECT *
 FROM generate_kmers((SELECT dna FROM dna_sequences LIMIT 1), 6) AS k(kmer);

 /* Output
              kmer
--------------------------------
 AAGTAGGTCTCGTCTGTGTTTTCTACGAGC
 AGTAGGTCTCGTCTGTGTTTTCTACGAGCT
 GTAGGTCTCGTCTGTGTTTTCTACGAGCTT
 TAGGTCTCGTCTGTGTTTTCTACGAGCTTG
 AGGTCTCGTCTGTGTTTTCTACGAGCTTGT
 GGTCTCGTCTGTGTTTTCTACGAGCTTGTG
 GTCTCGTCTGTGTTTTCTACGAGCTTGTGT
 TCTCGTCTGTGTTTTCTACGAGCTTGTGTT
 CTCGTCTGTGTTTTCTACGAGCTTGTGTTC
 TCGTCTGTGTTTTCTACGAGCTTGTGTTCC
 CGTCTGTGTTTTCTACGAGCTTGTGTTCCA
 GTCTGTGTTTTCTACGAGCTTGTGTTCCAG
 TCTGTGTTTTCTACGAGCTTGTGTTCCAGC
 CTGTGTTTTCTACGAGCTTGTGTTCCAGCT
 TGTGTTTTCTACGAGCTTGTGTTCCAGCTG
 GTGTTTTCTACGAGCTTGTGTTCCAGCTGA
 TGTTTTCTACGAGCTTGTGTTCCAGCTGAC
 GTTTTCTACGAGCTTGTGTTCCAGCTGACC
 TTTTCTACGAGCTTGTGTTCCAGCTGACCC
 TTTCTACGAGCTTGTGTTCCAGCTGACCCA
 TTCTACGAGCTTGTGTTCCAGCTGACCCAC
 TCTACGAGCTTGTGTTCCAGCTGACCCACT
 CTACGAGCTTGTGTTCCAGCTGACCCACTC
 TACGAGCTTGTGTTCCAGCTGACCCACTCC
 ACGAGCTTGTGTTCCAGCTGACCCACTCCC
 CGAGCTTGTGTTCCAGCTGACCCACTCCCT
 GAGCTTGTGTTCCAGCTGACCCACTCCCTG
 AGCTTGTGTTCCAGCTGACCCACTCCCTGG
 GCTTGTGTTCCAGCTGACCCACTCCCTGGG
 CTTGTGTTCCAGCTGACCCACTCCCTGGGT
 TTGTGTTCCAGCTGACCCACTCCCTGGGTG
 TGTGTTCCAGCTGACCCACTCCCTGGGTGG
 GTGTTCCAGCTGACCCACTCCCTGGGTGGG
 TGTTCCAGCTGACCCACTCCCTGGGTGGGG
 GTTCCAGCTGACCCACTCCCTGGGTGGGGG
 TTCCAGCTGACCCACTCCCTGGGTGGGGGG
 TCCAGCTGACCCACTCCCTGGGTGGGGGGA
 CCAGCTGACCCACTCCCTGGGTGGGGGGAC
 CAGCTGACCCACTCCCTGGGTGGGGGGACT
 AGCTGACCCACTCCCTGGGTGGGGGGACTG
 GCTGACCCACTCCCTGGGTGGGGGGACTGG
 CTGACCCACTCCCTGGGTGGGGGGACTGGG
 TGACCCACTCCCTGGGTGGGGGGACTGGGT
(43 Zeilen)
 */
 
---------------------------------------------------------------------------------------
-- Step 6. Testing the kmer_equals function
DO $$ 
BEGIN
    RAISE NOTICE 'Step 6. Testing the kmer_equals function';
END $$;

SELECT *  FROM kmer_sequences LIMIT 10;

-- output:
--                kmer
-- ----------------------------------
--  CATTCTTCACGTAGTTCTCGAGCCTTGGTTTT
--  CAGCGATGGAGAATGACTTTGACAAGCTGAGA
--  GGAGAATTGCTTGAACCTGGGAGCCAGAGGTT
--  GGGAGAAT
--  TTTTTGAGCAGCAGCAAGATTTATTGTGAAGA
--  TGGATATAACACATTTTGTTTATCCATTCATC
--  AGTTATTGTTAGTCATCAGTCACTGTAGAAAC
--  CTACATTTGATACCTAAACATTCTAATTTCCT
--  ATACATTTATTAAACAATGTTAAGACATTAAA
--  AGCCCAACCCCGTAGGCCATGAGGGAGGGGCA
-- (10 Zeilen)

-- Returns 2 tables with one row (using table t) - these 2 queries are equivalent
SELECT * FROM kmer_sequences WHERE kmer_equals('GGGAGAAT', kmer);
SELECT * FROM kmer_sequences WHERE kmer = 'GGGAGAAT';

/* Output
   kmer
----------
 GGGAGAAT
(1 Zeile)

   kmer
----------
 GGGAGAAT
(1 Zeile)
*/

-- Returns 2 tables with 0 row (using table t) - these 2 queries are equivalent
SELECT * FROM kmer_sequences WHERE kmer_equals('ACGTA', kmer);
SELECT * FROM kmer_sequences WHERE kmer = 'ACGTA';

/* Output
 kmer
------
(0 Zeilen)

 kmer
------
(0 Zeilen)
*/

 --------------------------------------------------------------------------------------
-- Step 7. Testing the starts_with function
DO $$ 
BEGIN
    RAISE NOTICE 'Step 7. Testing the starts_with function';
END $$;

-- Returns 2 tables with 2 rows (ACGT and ACGTC) - these 2 queries are equivalent
SELECT * FROM kmer_sequences WHERE starts_with('ACG', kmer) LIMIT 10;
SELECT * FROM kmer_sequences WHERE 'ACG' ^@ kmer LIMIT 10;

/* Output
                kmer
----------------------------------
 ACGTAGCAGCATCCACACATCAAGCTCAATCA
 ACGGCCTCCTTATCGATCTGGCCGTTCCTTGC
 ACGTTAGGAGTCTGGCTCAGGATGTCGCCGTG
 ACGGACTC
 ACGAGGGACGCTCCTGGAGCAAACATTCTGGC
 ACGGAGCTGCACAGCGTGGGGAACTCATCCAT
 ACGTAAAGCATTCTTGTGCCTTAGCCTCCCGA
 ACGCTGGTGAAAACTTGTCTCTTCTTACCAAA
 ACGCTGGTGAAAACTTGTCTCTTCTTACCAAA
 ACGCCATCACCACAGGAAGGCTCGCACGAGTC
(10 Zeilen)

               kmer
----------------------------------
 ACGTAGCAGCATCCACACATCAAGCTCAATCA
 ACGGCCTCCTTATCGATCTGGCCGTTCCTTGC
 ACGTTAGGAGTCTGGCTCAGGATGTCGCCGTG
 ACGGACTC
 ACGAGGGACGCTCCTGGAGCAAACATTCTGGC
 ACGGAGCTGCACAGCGTGGGGAACTCATCCAT
 ACGTAAAGCATTCTTGTGCCTTAGCCTCCCGA
 ACGCTGGTGAAAACTTGTCTCTTCTTACCAAA
 ACGCTGGTGAAAACTTGTCTCTTCTTACCAAA
 ACGCCATCACCACAGGAAGGCTCGCACGAGTC
(10 Zeilen)

*/
 -----------------------------------------------------------------------------------------------------------------
-- Step 8. Testing the contains function (works with qkmers too)
DO $$ 
BEGIN
    RAISE NOTICE 'Step 8. Testing the contains function';
END $$;

-- Returns 2 tables with 0 row (no matching kmer in length and in pattern) these 2 queries are equivalent
SELECT * FROM kmer_sequences WHERE contains('ANGTA', kmer) LIMIT 10;
SELECT * FROM kmer_sequences where 'ANGTA' @>kmer LIMIT 10;

/* Output
 kmer
------
(0 Zeilen)

 kmer
------
(0 Zeilen)
*/

-- Returns 2 tables with 4 rows - these 2 queries are equivalent
SELECT * FROM kmer_sequences WHERE contains('ACGNACTC', kmer);
SELECT * FROM kmer_sequences where 'ACGNACTC' @>kmer;

/* Output
   kmer
----------
 ACGGACTC
 ACGAACTC
 ACGCACTC
 ACGCACTC
(4 Zeilen)

   kmer
----------
 ACGGACTC
 ACGAACTC
 ACGCACTC
 ACGCACTC
(4 Zeilen)
*/

 --------------------------------------------------------------------------------------
-- Step 9. Testing the kmer counting support
DO $$ 
BEGIN
    RAISE NOTICE 'Step 9. Testing the kmer counting support';
END $$;

-- Count all 4-mers in the first dna sequence from the dna_sequence table
-- "AAGTAGGTCTCGTCTGTGTTTTCTACGAGCTTGTGTTCCAGCTGACCCACTCCCTGGGTGGGGGGACTGGGT"
-- Returns a table with 4 rows 
SELECT k.kmer, count(*)
FROM generate_kmers((SELECT dna FROM dna_sequences LIMIT 1), 4) AS k(kmer)
GROUP BY k.kmer
ORDER BY count(*) DESC;

/* Output
  kmer | count
------+-------
 GGGG |     3
 TGGG |     3
 GGGT |     2
 AGCT |     2
 GTCT |     2
 CTGG |     2
 TGTG |     2
 TGTT |     2
 GTGT |     2
 GTTT |     1
 GCTG |     1
 TCCA |     1
 CCAG |     1
 CACT |     1
 TTGT |     1
 TACG |     1
 GAGC |     1
 ACGA |     1
 -- More --
*/

-- Return the total, distinct and unique count of 4-mers in the first dna sequence from the dna_sequences table
-- "AAGTAGGTCTCGTCTGTGTTTTCTACGAGCTTGTGTTCCAGCTGACCCACTCCCTGGGTGGGGGGACTGGGT"
-- Returns a table with one row
WITH kmers AS (
  SELECT k.kmer, count(*) AS kmer_count
  FROM generate_kmers((SELECT dna FROM dna_sequences LIMIT 1), 4) AS k(kmer)
  GROUP BY k.kmer
)
SELECT 
  sum(kmer_count) AS total_count,
  count(*) AS distinct_count,
  count(*) FILTER (WHERE kmer_count = 1) AS unique_count
FROM kmers;

/* Output
 total_count | distinct_count | unique_count
-------------+----------------+--------------
          69 |             58 |           49
(1 Zeile)
*/

-- Return the total, distinct and unique count of 4-mers in 'ACGTACGT' and each kmer  
WITH kmers AS (
SELECT k.kmer, count(*) 
FROM generate_kmers((SELECT dna FROM dna_sequences LIMIT 1), 4) AS k(kmer)
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
 GGTG |           1 |              1 |            1
 GGGT |           2 |              1 |            0
 TCCC |           1 |              1 |            1
 CTGT |           1 |              1 |            1
 CGTC |           1 |              1 |            1
 CAGC |           1 |              1 |            1
 TAGG |           1 |              1 |            1
 ACCC |           1 |              1 |            1
 AGGT |           1 |              1 |            1
 GGAC |           1 |              1 |            1
 GTTT |           1 |              1 |            1
 GCTG |           1 |              1 |            1
 TCCA |           1 |              1 |            1
 CCAG |           1 |              1 |            1
 CACT |           1 |              1 |            1
 TGTG |           2 |              1 |            0
                    -- More --
*/

 --------------------------------------------------------------------------------------
-- Step 10. Testing the Indexes 
DO $$ 
BEGIN
    RAISE NOTICE 'Step 10. Testing the Indexes';
END $$;

-- Create the index
DROP INDEX IF EXISTS spgist_index;
CREATE INDEX spgist_index ON kmer_sequences USING spgist (kmer kmer_index_support);

SET enable_seqscan = OFF;

EXPLAIN (SELECT * FROM kmer_sequences WHERE 'ACGCACTC'= kmer);

--                                             QUERY PLAN
-- --------------------------------------------------------------------------------------------------
--  Index Only Scan using spgist_index on kmer_sequences  (cost=0.29..25521.51 rows=230206 width=35)
--    Filter: ('ACGT'::kmer = kmer)
-- (2 Zeilen)

-- In Comparison to the runtime without the Index:

-- ncbi=# DROP INDEX IF EXISTS spgist_index;
-- DROP INDEX
-- ncbi=# EXPLAIN (SELECT * FROM kmer_sequences WHERE 'ACGCACTC'= kmer);
--                                        QUERY PLAN
-- ----------------------------------------------------------------------------------------
--  Seq Scan on kmer_sequences  (cost=10000000000.00..10000009539.16 rows=230206 width=35)
--    Filter: ('ACGCACTC'::kmer = kmer)
--  JIT:
--    Functions: 2
--    Options: Inlining true, Optimization true, Expressions true, Deforming true
-- (5 Zeilen)


EXPLAIN (SELECT * FROM kmer_sequences WHERE  kmer ^@ 'ACG');

--                                             QUERY PLAN
-- --------------------------------------------------------------------------------------------------
--  Index Only Scan using spgist_index on kmer_sequences  (cost=0.29..12844.89 rows=230206 width=35)
--    Index Cond: (kmer ^@ 'ACG'::kmer)
-- (2 Zeilen)

EXPLAIN (SELECT * FROM kmer_sequences WHERE  'ANGTA' @> kmer);

--                                             QUERY PLAN
-- --------------------------------------------------------------------------------------------------
--  Index Only Scan using spgist_index on kmer_sequences  (cost=0.29..25521.51 rows=230206 width=35)
--    Filter: ('ANGTA'::qkmer @> kmer)
-- (2 Zeilen)







