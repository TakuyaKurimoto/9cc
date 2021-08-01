FROM ubuntu:latest
RUN apt update
RUN apt install -y gcc make binutils libc6-dev gdb sudo