#! /bin/sh
#
# SPDX-FileCopyrightText: 2024 Bùi Gia Viện (BlissLabs) <shadichy@blisslabs.org>
# SPDX-License-Identifier: BSD-2-Clause
#
# Test preparation for unpackfs; since there's a bunch
# of fiddly bits than need to be present for the tests,
# do that in a script rather than entirely in CTest.
#
SRCDIR=$( dirname "$0" )
