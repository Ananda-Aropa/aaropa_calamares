/* === This file is part of Calamares - <https://calamares.io> ===
 *
 *   SPDX-FileCopyrightText: 2025 Bùi Gia Viện <shadichy@blisslabs.org>
 *   SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   Calamares is Free Software: see the License-Identifier above.
 *
 */

#include "Config.h"

#include "GlobalStorage.h"
#include "JobQueue.h"
#include "compat/Variant.h"
#include "utils/Logger.h"
#include "utils/NamedEnum.h"
#include "utils/Retranslator.h"
#include "utils/Variant.h"

Config::Config( QObject* parent )
    : QObject( parent )
{
    CALAMARES_RETRANSLATE_SLOT( &Config::retranslate );
}

Config::~Config() {}

void
Config::retranslate()
{
    emit titleLabelChanged( titleLabel() );
    emit checkboxLabelChanged( checkboxLabel() );
    emit noticeLabelChanged( noticeLabel() );
}

QString
Config::titleLabel() const
{
    return m_titleLabel ? m_titleLabel->get() : QString();
}

QString
Config::checkboxLabel() const
{
    return m_checkboxLabel ? m_checkboxLabel->get() : QString();
}

QString
Config::noticeLabel() const
{
    return m_noticeLabel ? m_noticeLabel->get() : QString();
}

void
Config::loadingDone()
{
    emit statusReady();
}


const NamedEnumTable< Config::Unit >&
Config::unitNames()
{
    static const NamedEnumTable< Config::Unit > names { { QStringLiteral( "MiB" ), Config::Unit::MiB },
                                                        { QStringLiteral( "GiB" ), Config::Unit::GiB } };

    return names;
}

int
Config::unitRatio( Config::Unit unit ) const
{
    if ( unit == Config::Unit::GiB )
    {
        return 1024;
    }
    else if ( unit == Config::Unit::MiB )
    {
        return 1;
    }
    return 0;
}

void
Config::setUnit( Config::Unit unit )
{
    m_unit = unit;
    m_dataSize /= unitRatio( m_unit );
}

void
Config::setUseMaximum( bool useMaximum )
{
    m_useMaximum = useMaximum;
    m_dataSize = useMaximum ? 0 : m_dataSize;
}

void
Config::setConfigurationMap( const QVariantMap& configurationMap )
{
    bool bogus = false;
    auto label = Calamares::getSubMap( configurationMap, "label", bogus );
    static const char className[] = "DataImgViewStep";
    if ( label.contains( "notice" ) )
    {
        m_noticeLabel = new Calamares::Locale::TranslatedString( label, "notice", className );
    }
    if ( label.contains( "title" ) )
    {
        m_titleLabel = new Calamares::Locale::TranslatedString( label, "title", className );
    }
    if ( label.contains( "checkbox" ) )
    {
        m_checkboxLabel = new Calamares::Locale::TranslatedString( label, "checkbox", className );
    }
}

void
Config::finalizeGlobalStorage()
{
    auto* gs = Calamares::JobQueue::instance() ? Calamares::JobQueue::instance()->globalStorage() : nullptr;
    if ( gs )
    {
        QVariantMap m = gs->value( "dataimg" ).toMap();
        m.insert( "useMaximum", m_useMaximum );
        m.insert( "dataSize", m_dataSize );
        gs->insert( "dataimg", m );
    }
}
