#!/usr/bin/env bash

cd ../..
gzip -c9 webui.html > webui_html && xxd -i webui_html > generated/webui_html.h && rm webui_html && sed -i -e 's/unsigned char webui_html\[]/const uint8_t webui_html[] PROGMEM/' generated/webui_html.h && sed -i -e 's/unsigned int webui_html_len/const unsigned int webui_html_len/' generated/webui_html.h
rm -fv generated/webui_html.h-e

echo "generated/webui_html.h GENERATED";