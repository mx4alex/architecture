#!/bin/sh

set -eu

old_uid=$1
old_gid=$2
shift
shift

groupadd --gid "$old_gid" --non-unique user
useradd --uid "$old_uid" --gid "$old_gid" --non-unique user

sudo -E -u user "$@"
