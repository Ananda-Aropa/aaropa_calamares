/*
 *   SPDX-FileCopyrightText: 2016 Luca Giambonini <almack@chakraos.org>
 *   SPDX-FileCopyrightText: 2016 Lisa Vitolo     <shainer@chakraos.org>
 *   SPDX-FileCopyrightText: 2017 Kyle Robbertze  <krobbertze@gmail.com>
 *   SPDX-FileCopyrightText: 2017-2018 2020, Adriaan de Groot <groot@kde.org>
 *   SPDX-FileCopyrightText: 2017 Gabriel Craciunescu <crazy@frugalware.org>
 *   SPDX-FileCopyrightText: 2025 Bùi Gia Viện (BlissLabs) <shadichy@blisslabs.org>
 *   SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   Calamares is Free Software: see the License-Identifier above.
 *
 */

#include "Config.h"

#include "LoaderQueue.h"

#include "GlobalStorage.h"
#include "JobQueue.h"
#include "compat/Variant.h"
#include "network/Manager.h"
#include "utils/Logger.h"
#include "utils/Retranslator.h"
#include "utils/Variant.h"

#include <QNetworkReply>

Config::Config( QObject* parent )
    : QObject( parent )
    , m_model( new OptionModel( this ) )
{
    CALAMARES_RETRANSLATE_SLOT( &Config::retranslate );
}

Config::~Config() {}

void
Config::retranslate()
{
    emit statusChanged( status() );
    emit sidebarLabelChanged( sidebarLabel() );
    emit titleLabelChanged( titleLabel() );
    emit subtitleLabelChanged( subtitleLabel() );
}

QString
Config::status() const
{
    switch ( m_status )
    {
    case Status::Ok:
        return QString();
    case Status::FailedBadConfiguration:
        return tr( "Network Installation. (Disabled: Incorrect configuration)" );
    case Status::FailedBadData:
        return tr( "Network Installation. (Disabled: Received invalid groups data)" );
    case Status::FailedInternalError:
        return tr( "Network Installation. (Disabled: Internal error)" );
    case Status::FailedNetworkError:
        return tr( "Network Installation. (Disabled: Unable to fetch option lists, check your network connection)" );
    case Status::FailedNoData:
        return tr( "Network Installation. (Disabled: No option list)" );
    }
    __builtin_unreachable();
}

void
Config::setStatus( Status s )
{
    m_status = s;
    emit statusChanged( status() );
}

QString
Config::sidebarLabel() const
{
    return m_sidebarLabel ? m_sidebarLabel->get() : tr( "Option selection" );
}

QString
Config::titleLabel() const
{
    return m_titleLabel ? m_titleLabel->get() : QString();
}

QString
Config::subtitleLabel() const
{
    return m_subtitleLabel ? m_subtitleLabel->get() : QString();
}

void
Config::loadGroupList( const QVariantList& groupData )
{
    m_model->setupModelData( groupData );
    if ( m_model->rowCount() < 1 )
    {
        cWarning() << "Options groups data was empty.";
        setStatus( Status::FailedNoData );
    }
    else
    {
        setStatus( Status::Ok );
    }
}

void
Config::loadingDone()
{
    if ( m_queue )
    {
        m_queue->deleteLater();
        m_queue = nullptr;
    }
    emit statusReady();
}

void
Config::setConfigurationMap( const QVariantMap& configurationMap )
{
    setRequired( Calamares::getBool( configurationMap, "required", false ) );

    // Get the translations, if any
    bool bogus = false;
    auto label = Calamares::getSubMap( configurationMap, "label", bogus );
    // Use a different class name for translation lookup because the
    // .. table of strings lives in OptionsViewStep.cpp and moving them
    // .. around is annoying for translators.
    static const char className[] = "OptionsViewStep";

    if ( label.contains( "sidebar" ) )
    {
        m_sidebarLabel = new Calamares::Locale::TranslatedString( label, "sidebar", className );
    }
    if ( label.contains( "title" ) )
    {
        m_titleLabel = new Calamares::Locale::TranslatedString( label, "title", className );
    }
    if ( label.contains( "subtitle" ) )
    {
        m_subtitleLabel = new Calamares::Locale::TranslatedString( label, "subtitle", className );
    }

    // Lastly, load the groups data
    m_queue = new LoaderQueue( this );

    // There are 2 sources: config file and globalStorage
    auto* gs = Calamares::JobQueue::instance() ? Calamares::JobQueue::instance()->globalStorage() : nullptr;
    if ( gs && gs->contains( "presets" ) )
    {
        QVariantMap presets = gs->value( "presets" ).toMap();
        const QString url = presets.value( "selection" ).toString();
        if ( !url.isEmpty() )
        {
            m_queue->append( SourceItem { QUrl { url }, QVariantList() } );
        }
    }

    const QString key = QStringLiteral( "groupsUrl" );
    const auto& groupsUrlVariant = configurationMap.value( key );
    if ( Calamares::typeOf( groupsUrlVariant ) == Calamares::StringVariantType )
    {
        m_queue->append( SourceItem::makeSourceItem( groupsUrlVariant.toString(), configurationMap ) );
    }
    else if ( Calamares::typeOf( groupsUrlVariant ) == Calamares::ListVariantType )
    {
        for ( const auto& s : groupsUrlVariant.toStringList() )
        {
            m_queue->append( SourceItem::makeSourceItem( s, configurationMap ) );
        }
    }

    setStatus( required() ? Status::FailedNoData : Status::Ok );
    cDebug() << "Loading options from" << m_queue->count() << "alternate sources.";
    connect( m_queue, &LoaderQueue::done, this, &Config::loadingDone );
    m_queue->load();
}

void
Config::finalizeGlobalStorage()
{
    auto options = model()->getOptions();

    QString outputArguments = "";

    for ( const auto& option : options )
    {
        outputArguments += option->toOperation() + " ";
    }
    Calamares::JobQueue::instance()->globalStorage()->insert( "options", outputArguments );
}
