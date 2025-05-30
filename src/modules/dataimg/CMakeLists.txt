# === This file is part of Calamares - <https://calamares.io> ===
#
#   SPDX-FileCopyrightText: 2025 Bùi Gia Viện <shadichy@blisslabs.org>
#   SPDX-License-Identifier: GPL-3.0-or-later
#
calamares_add_plugin(dataimg
    TYPE viewmodule
    EXPORT_MACRO PLUGINDLLEXPORT_PRO
    SOURCES
        Config.cpp
        DataImgViewStep.cpp
        DataImgPage.cpp
    UI
        dataimg.ui
    LINK_PRIVATE_LIBRARIES
        ${qtname}::Widgets
    SHARED_LIB
)

if(TARGET ${kfname}::CoreAddons)
    #calamares_add_test(
    #    dataimgtest
    #    SOURCES Tests.cpp Config.cpp
    #    LIBRARIES ${qtname}::Widgets ${qtname}::Gui ${kfname}::CoreAddons
    #)
endif()
