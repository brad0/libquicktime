#!/bin/sh
OLD_YEARS="2002-2007"
NEW_YEARS="2002-2010"
find . -name '*.[ch]' -exec sed -i "s/$OLD_YEARS/$NEW_YEARS/g" {} \;
