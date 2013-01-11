#!/usr/bin/env sh
rm *.c *.h Makefile.am.sample
asn1c -fnative-types *.asn1
rm converter-sample.c
