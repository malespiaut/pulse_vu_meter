#!/bin/sh

glib-compile-resources --target=resource.c --generate-source resource.xml
make
