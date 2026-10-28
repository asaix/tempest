#!/bin/bash
# installs the tempest binary next to this script to ~/.local/bin
set -e

cd "$(dirname "$0")"
mkdir -p "$HOME/.local/bin"
install -m 755 tempest "$HOME/.local/bin/tempest"
echo "installed tempest to $HOME/.local/bin/tempest"
