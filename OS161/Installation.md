## Prerequisite

Make sure you have [Docker](https://docs.docker.com/get-started/get-docker/) installed.

## Pull the OS161 Image

```bash
docker pull marcopalena/polito-os161:latest
```

## Run the Container

```bash
docker run -it -v $HOME/os161:/home/os161 marcopalena/polito-os161
```

**Note:** Work inside the container persists in `~/os161`.
