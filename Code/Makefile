MODULE_big	= dna_seq

OBJS = 	$(WIN32RES) \
		src/dna.o \
		src/kmer.o\
		src/functions.o
		

EXTENSION = dna_seq

DATA = 	dna_seq--1.0.sql \
		dna_seq.control \
		src/dna.control \
		src/kmer.control \
		src/functions.control

HEADERS_dna_seq = src/dna.h \
				  src/kmer.h

PG_CONFIG ?= pg_config
PGXS = $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
