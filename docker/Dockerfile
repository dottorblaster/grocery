FROM alpine:3.6

RUN apk update
RUN apk add alpine-sdk imagemagick

RUN mkdir /grocery
ADD . /grocery
WORKDIR /grocery
RUN make clean release
