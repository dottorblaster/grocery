FROM alpine:3.6

RUN apk update
RUN apk add --no-cache build-base imagemagick

RUN mkdir /grocery
RUN mkdir /grocery/www
RUN mkdir /grocery/cache

ADD . /grocery

WORKDIR /grocery
RUN make clean release

RUN apk del build-base

VOLUME /grocery/www

CMD ["./grocery", "8080"]