# grocery [![Build Status](https://travis-ci.org/dottorblaster/grocery.svg?branch=master)](https://travis-ci.org/dottorblaster/grocery)
Tiny C webserver with resource adaption features.

## Dependencies
To execute grocery you need:

- A Linux kernel
- GLibC or similar (muslc is fine, for Void/Alpine users)
- The [ImageMagick](https://www.imagemagick.org/script/index.php) suite

If you want compile grocery from its source code, you only need `make` and `gcc`. Of course if you want to build a grocery container from scratch you need Docker.

## Running grocery on Docker
It's dead simple. You just need to run the container mounting a directory as the `www` volume. Let's say you have a `files` folder in your current working directory:

```shell
$ sudo docker run -d -P -p 8080:8080 -v $(pwd)/files:/grocery/www dottorblaster/grocery
```

## Development testing on Docker
All you need is in `Makefile`. After building the test image with `make build_docker_test`, you can run that with `make docker_test`. Then, you can restart the container to reflect code changes as the image rebuilds the whole application as the entry point.

## Running locally (on Linux)
If you got all your dependencies ready, just build the webserver and run it:

```shell
$ make

$ ./grocery 8080 
```

Grocery will be listening on `localhost:8080`.

## Directories and permissions
You need:

- A `www` directory in your cwd as grocery will attempt to read files from that
- An empty `cache` directory in the cwd, as grocery will use it as a persistency layer for the caching logic. 
