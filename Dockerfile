FROM postgres:latest

# Update package lists and install the necessary packages
RUN apt-get update && apt-get install -y \
    postgresql-server-dev-all \
    gcc

# Copy the complex extension files into the container
COPY ./Code /tmp/Code

# Navigate to the directory and build/install the extension
RUN cd /tmp/Code && make clean && make && make install

