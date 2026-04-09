## Prerequisite

Make sure you have [Docker](https://docs.docker.com/get-started/get-docker/) installed.

## Pull the OS161 Image

```bash
docker pull marcopalena/polito-os161:latest
```

## Make a Copy to Save Edits

```
docker volume create polito-os161-vol
```

```
mkdir </path/to/custom/volume/location>
docker volume create --driver local \
                     --opt o=bind \
                     --opt type=none \
                     --opt device=</path/to/custom/volume/location> polito-os161-vol
```

```
docker volume inspect polito-os161-vol
```

## Run the Container

```bash
docker run --volume polito-os161-vol:/home/os161user --name polito-os161 -itd marcopalena/polito-os161 /bin/bash
```

```
docker exec -it polito-os161 /bin/bash
```
