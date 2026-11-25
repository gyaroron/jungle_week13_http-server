#!/bin/sh
printf 'Content-Type: text/plain; charset=utf-8\r\n\r\nhello query=%s\n' "$QUERY_STRING"
