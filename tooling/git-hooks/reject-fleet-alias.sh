#!/usr/bin/env sh
# Rejects any commit or push that carries the fleet host's alias (operator rule, bead brepgpu-8k0).
# The alias is assembled from parts so this file never contains it.
# Usage: reject-fleet-alias.sh staged | msg <file> | push
w="neur""oses"
fail() { echo "rejected: $1 contains the fleet host alias; remove it (bead brepgpu-8k0)." >&2; exit 1; }
case "$1" in
  staged)
    if git diff --cached --name-only | grep -qi -e "$w"; then fail "a staged path"; fi
    if git grep --cached -q -i -e "$w"; then
      git grep --cached -l -i -e "$w" >&2
      fail "the staged tree"
    fi ;;
  msg)
    if grep -qi -e "$w" "$2"; then fail "the commit message"; fi ;;
  push)
    # Every local commit no remote has yet, branches and tags alike; stdin is left for the beads hook.
    if git log -p --no-color --format='%H%n%B' --branches --tags --not --remotes | grep -qi -e "$w"; then
      fail "an unpushed commit"
    fi ;;
  *) echo "usage: $0 staged | msg <file> | push" >&2; exit 2 ;;
esac
exit 0
