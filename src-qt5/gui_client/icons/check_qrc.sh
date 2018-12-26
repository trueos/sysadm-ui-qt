#!/bin/sh

tmp=""
for file in $(grep '<file' icons.qrc | cut -d '>' -f 2 | cut -d '<' -f 1)
do
  echo "${file}" | grep -q -E "(\.svg|\.png)"
  if [ $? -ne 0 ] ; then
      if [ -n "${tmp}" ] ; then
        tmp="${tmp} ${file}"
      else
        tmp="${file}"
      fi
      continue
  elif [ -n "${tmp}" ] ; then
    file="${tmp} ${file}"
  fi
  tmp=""
  if [ ! -e "${file}" ] ; then
    echo "File Missing: ${file}"
  fi
done
