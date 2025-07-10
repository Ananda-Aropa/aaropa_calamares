/* === This file is part of Calamares - <https://calamares.io> ===
 *
 *   SPDX-FileCopyrightText: 2016 Luca Giambonini <almack@chakraos.org>
 *   SPDX-FileCopyrightText: 2016 Lisa Vitolo <shainer@chakraos.org>
 *   SPDX-FileCopyrightText: 2017 Kyle Robbertze  <krobbertze@gmail.com>
 *   SPDX-FileCopyrightText: 2017-2018 2020, Adriaan de Groot <groot@kde.org>
 *   SPDX-FileCopyrightText: 2023 Vladislav Nepogodin <nepogodin.vlad@gmail.com>
 *   SPDX-FileCopyrightText: 2025 Bùi Gia Viện (BlissLabs) <shadichy@blisslabs.org>
 *   SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   Calamares is Free Software: see the License-Identifier above.
 *
 */

#include "OptionsViewStep.h"

#include "OptionsPage.h"

CALAMARES_PLUGIN_FACTORY_DEFINITION( OptionsViewStepFactory, registerPlugin< OptionsViewStep >(); )

OptionsViewStep::OptionsViewStep( QObject* parent )
    : Calamares::ViewStep( parent )
    , m_widget( new OptionsPage( &m_config ) )
    , m_nextEnabled( false )
{
    connect( &m_config, &Config::statusReady, this, &OptionsViewStep::nextIsReady );

    m_config.model()->setUpdateNextCall( [this]( bool enabled ) {
        this->updateNextEnabled( enabled );
    });
}


OptionsViewStep::~OptionsViewStep()
{
    if ( m_widget && m_widget->parent() == nullptr )
    {
        m_widget->deleteLater();
    }
}


QString
OptionsViewStep::prettyName() const
{
    return m_config.sidebarLabel();

#if defined( TABLE_OF_TRANSLATIONS )
    __builtin_unreachable();
    // This is a table of "standard" labels for this module. If you use them
    // in the label: sidebar: section of the config file, the existing
    // translations can be used.
    //
    // These translations still live here, even though the lookup
    // code is in the Config class.
    tr( "Option selection" );
    tr( "Office software" );
    tr( "Office option" );
    tr( "Browser software" );
    tr( "Browser option" );
    tr( "Web browser" );
    tr( "Kernel", "label for options module, Linux kernel" );
    tr( "Services", "label for options module, system services" );
    tr( "Login", "label for options module, choose login manager" );
    tr( "Desktop", "label for options module, choose desktop environment" );
    tr( "Applications" );
    tr( "Communication", "label for options module" );
    tr( "Development", "label for options module" );
    tr( "Office", "label for options module" );
    tr( "Multimedia", "label for options module" );
    tr( "Internet", "label for options module" );
    tr( "Theming", "label for options module" );
    tr( "Gaming", "label for options module" );
    tr( "Utilities", "label for options module" );
#endif
}


QWidget*
OptionsViewStep::widget()
{
    return m_widget;
}


bool
OptionsViewStep::isNextEnabled() const
{
    return !m_config.required() || m_nextEnabled;
}


bool
OptionsViewStep::isBackEnabled() const
{
    return true;
}


bool
OptionsViewStep::isAtBeginning() const
{
    return true;
}


bool
OptionsViewStep::isAtEnd() const
{
    return true;
}


Calamares::JobList
OptionsViewStep::jobs() const
{
    return Calamares::JobList();
}


void
OptionsViewStep::onActivate()
{
    m_widget->onActivate();
}

void
OptionsViewStep::onLeave()
{
    m_config.finalizeGlobalStorage();
}

void
OptionsViewStep::nextIsReady()
{
    m_nextEnabled = true;
    emit nextStatusChanged( true );
}

void
OptionsViewStep::setConfigurationMap( const QVariantMap& configurationMap )
{
    m_config.setConfigurationMap( configurationMap );
}

void
OptionsViewStep::updateNextEnabled( bool enabled )
{
    m_nextEnabled = enabled;
    emit nextStatusChanged( enabled );
}
