#!/bin/bash
# Author: Bùi Gia Viện (BlissLabs) <shadichy@blisslabs.org>

# Patches branding name in the Cmake file
sed -i -r 's|calamares_add_branding_subdirectory|# calamares_add_branding_subdirectory|g' src/branding/CMakeLists.txt
