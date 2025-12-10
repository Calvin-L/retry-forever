# -----------------------------------------------------------------------------
# Project Setup

cmake_minimum_required(VERSION 3.30)
project(RetryForever
    VERSION 1.0)

# -----------------------------------------------------------------------------
# Executable

add_executable(retry-forever
    src/main.c)

# -----------------------------------------------------------------------------
# Install  (I feel that this should be automatic given just the list of
# targets, but CMake requires us to explicitly list the things we want
# installed.)

install(
    TARGETS retry-forever
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
