/* === This file is part of Calamares - <https://calamares.io> ===
 *
 *   SPDX-FileCopyrightText: 2025 Bùi Gia Viện <shadichy@blisslabs.org>
 *   SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   Calamares is Free Software: see the License-Identifier above.
 *
 */

#include "DataImgViewStep.h"

#include "DataImgPage.h"

#include "GlobalStorage.h"
#include "JobQueue.h"

#include "utils/Logger.h"
#include "utils/QtCompat.h"
#include "utils/Retranslator.h"
#include "utils/Variant.h"
#include "widgets/TranslationFix.h"

#include <QMessageBox>

CALAMARES_PLUGIN_FACTORY_DEFINITION( DataImgViewStepFactory, registerPlugin< DataImgViewStep >(); )

DataImgViewStep::DataImgViewStep( QObject* parent )
    : Calamares::ViewStep( parent )
    , m_widget( new DataImgPage( &m_config ) )
    , m_nextEnabled( true )
{
    connect( &m_config, &Config::statusReady, this, &DataImgViewStep::nextIsReady );
}


DataImgViewStep::~DataImgViewStep()
{
    if ( m_widget && m_widget->parent() == nullptr )
    {
        m_widget->deleteLater();
    }
}


QString
DataImgViewStep::prettyName() const
{
    return m_config.titleLabel();
}


QWidget*
DataImgViewStep::widget()
{
    return m_widget;
}


bool
DataImgViewStep::isNextEnabled() const
{
    return true;
}


bool
DataImgViewStep::isBackEnabled() const
{
    return true;
}


bool
DataImgViewStep::isAtBeginning() const
{
    return true;
}


bool
DataImgViewStep::isAtEnd() const
{
    return true;
}


Calamares::JobList
DataImgViewStep::jobs() const
{
    return Calamares::JobList();
}


void
DataImgViewStep::onActivate()
{
    // from GS, if partition step has /data mountpoint, skip
    auto* gs = Calamares::JobQueue::instance() ? Calamares::JobQueue::instance()->globalStorage() : nullptr;
    if ( gs && gs->contains( "partitions" ) )
    {
        QVariantList partitions = gs->value( "partitions" ).toList();
        for ( const QVariant& partition : partitions )
        {
            QVariantMap partitionData = partition.toMap();
            if ( partitionData.value( "mountPoint" ).toString() == "/data" )
            {
                cDebug() << "Skipping DataImgViewStep because /data mountpoint is already set in partition step.";
                return;
            }
            // set upper limit as size of / -10GiB
            // if ( partitionData.value( "mountPoint" ).toString() == "/" )
            // {
            // 		unsigned int size = partitionData.value( "size" ).toInt();
            // 		if ( size > 10 * 1024 * 1024 )  // 10 GiB
            // 		{
            // 				m_config.setMaximumDataSize( size - 10 * 1024 * 1024 );  // Set maximum size to / size - 10 GiB
            // 				cDebug() << "Setting maximum data size to" << m_config.maximumDataSize() << "bytes.";
            // 		}
            // 		else
            // 		{
            // 				cWarning() << "Partition size is too small for /data, skipping DataImgViewStep.";
            // 				return;
            // 		}
            // }
        }
    }
    QMessageBox mb( QMessageBox::Warning, m_config.titleLabel(), m_config.noticeLabel(), QMessageBox::Yes, m_widget );
    mb.addButton( QMessageBox::No );
    Calamares::fixButtonLabels( &mb );
    mb.exec();
}

void
DataImgViewStep::onLeave()
{
    m_config.finalizeGlobalStorage();
}

void
DataImgViewStep::nextIsReady()
{
    m_nextEnabled = true;
    emit nextStatusChanged( true );
}

void
DataImgViewStep::setConfigurationMap( const QVariantMap& configurationMap )
{
    m_config.setConfigurationMap( configurationMap );
}

void
DataImgViewStep::updateNextEnabled( bool enabled )
{
    m_nextEnabled = enabled;
    emit nextStatusChanged( enabled );
}
