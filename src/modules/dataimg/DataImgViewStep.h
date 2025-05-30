/* === This file is part of Calamares - <https://calamares.io> ===
 *
 *   SPDX-FileCopyrightText: 2025 Bùi Gia Viện <shadichy@blisslabs.org>
 *   SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   Calamares is Free Software: see the License-Identifier above.
 *
 */

#ifndef DATAIMGVIEWSTEP_H
#define DATAIMGVIEWSTEP_H

#include "Config.h"

#include "DllMacro.h"
#include "utils/PluginFactory.h"
#include "viewpages/ViewStep.h"

#include <QVariant>

class DataImgPage;

class PLUGINDLLEXPORT DataImgViewStep : public Calamares::ViewStep
{
    Q_OBJECT

public:
    explicit DataImgViewStep( QObject* parent = nullptr );
    ~DataImgViewStep() override;

    QString prettyName() const override;

    QWidget* widget() override;

    bool isNextEnabled() const override;
    bool isBackEnabled() const override;

    bool isAtBeginning() const override;
    bool isAtEnd() const override;

    Calamares::JobList jobs() const override;

    void onActivate() override;

    void onLeave() override;

    void setConfigurationMap( const QVariantMap& configurationMap ) override;

    void updateNextEnabled( bool enabled );

public slots:
    void nextIsReady();

private:
    Config m_config;

    DataImgPage* m_widget;
    bool m_nextEnabled = true;
};

CALAMARES_PLUGIN_FACTORY_DECLARATION( DataImgViewStepFactory )

#endif  // DATAIMGVIEWSTEP_H
