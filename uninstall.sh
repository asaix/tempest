#!/bin/bash
# removes the tempest binary installed by install.sh
set -e

rm -f "$HOME/.local/bin/tempest"
echo "removed $HOME/.local/bin/tempest"
