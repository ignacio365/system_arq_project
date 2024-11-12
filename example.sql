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

CREATE TABLE t (id integer, dna dna, kmer kmer);

INSERT INTO t VALUES
(1, 'ACGT','ACGT'),
(2, 'CT', 'CT'),
(3, 'AAA', 'AAA'),
(4, 'AGTTTTGAAAA','AGTTTTGAAAA');

/*Checking that it works*/
INSERT INTO t VALUES (5, 'BTGE', 'BTGE');

INSERT INTO t VALUES (5, '', '');

INSERT INTO t VALUES (5, 'AGTTTTGAAAAAGTTTTGAAAAAGTTTTGAAAAAGTTTTGAAAA', 'AGTTTTGAAAAAGTTTTGAAAAAGTTTTGAAAAAGTTTTGAAAA');

SELECT * FROM t;
SELECT dna, text(dna), dna(text(dna)), size(dna), length(dna) FROM t;
SELECT kmer, text(kmer), kmer(text(kmer)), size(kmer), length(kmer) FROM t;

SELECT *
 FROM generate_kmers('ACGTACGT', 6) AS k(kmer);


 SELECT *
 FROM generate_kmers((SELECT dna FROM t WHERE text(dna)='AGTTTTGAAAA'),2);



