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

### Step 3: Create extension and execute queries

Run the commented example.sql file which creates the extension and tests it with several queries (can also be done in PgAdmin4 by connecting to the psql server, creating the dna database and copy pasting queries from the example.sql script).

To do so, first connect to psql: 

```bash
psql -h localhost -p 25432 -U postgres -W 
```

Then in psql, create a new database: 
```bash
postgres=#CREATE DATABASE dna;
postgres=#QUIT;
```

Then connect to the dna database and run the example.sql script, which outputs query results and comments.
```bash
psql -h localhost -p 25432 -U postgres -W -d dna -f example.sql
```
### Step 4: Test the extension with data from the SRA-Databse 

To test the extension with real data from the SRA-Database, use the "sra_backup.tar" databasefile from the zip-folder.
It contains data from the "DNA of bacteria in beagle feces" database with the Accession ID "SRX26747536" found on the SRA-Database Website. (https://www.ncbi.nlm.nih.gov/sra/?term=SRA%20database)
To simplify the testing process, the database was pre-transformed using a python script to separate the provided sequences into kmers, qkmers and dna strings. 
The resulting database with one table per sequence type and the "dna_seq" extension are backed up in a .tar file.

Unfortunately that file is too big to upload, therefore you will have to access the zipped .tar file from the ULB OneDrive and unzip it into the project folder. 

Access the OneDrive via this Link: 
https://universitelibrebruxelles-my.sharepoint.com/:u:/g/personal/jule_grigat_ulb_be/EZEDAnGs_jJMpmMN6V6nPmQB41hQMFlwszpRBlgjVNrw6g?e=1jRh9Y

Please use your ULB-Account to do so, since you're only granted access with your ULB-Account. 

To eventually restore the database, please navigate to the project folder that should now contain the sra_backup.tar file. Run the following command:

```bash
pg_restore -h localhost -p 25432 -U postgres -W -C -d postgres sra_backup.tar
 ```
After entering the password for the server, the database including the extension will be restored.  
Then run the following line to connect to the dna database and run the sql script, which outputs query results and comments.

```bash
psql -h localhost -p 25432 -U postgres -W -d ncbi -f example_sra_data.sql
```







