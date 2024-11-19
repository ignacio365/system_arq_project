TO DO LIST
 1. ~~DNA Sequence Type (DNA)~~
 2. ~~K-mer Type (kmer)~~
 3. Query K-mer Type (qkmer) 
 4. ~~length(dna)~~
 5. ~~length(kmer)~~
 6. length(qkmer)
 7. ~~generate_kmers(sequence dna, k integer)~~
 8. ~~equals(k kmer, k kmer)~~
 9. ~~starts_with(k kmer, k kmer)~~
 10. contains(pattern qkmer, k kmer) << IGNACIO
 11. K-mer Counting Support
 12. Index Structure
 13. Test your implementation with both synthetic and real-world data. For development and initial

 ## Building and Running the Custom PostgreSQL Docker Image for Extension

### Step 1: Build the Docker Image

Navigate to the directory containing the Dockerfile, then build the Docker image by running:

```bash
docker build -t custom_postgres .
```
### Step 2: Run the Docker Container
After building the image, start a container using the following command (Do not forget to open the docker app in your PC): 

```bash
docker run --name postgres_ext_project_container -d -p 25432:5432 -e POSTGRES_PASSWORD=mysecretpassword custom_postgres
```

### Important

Each time that you build the docker container and run it, beforehand you have to delete the old docker container on the docker app. Other option is, instead of deleting the old container, just change the name of the container in the second step. 


