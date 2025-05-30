/* === This file is part of Calamares - <https://calamares.io> ===
 *
 *   SPDX-FileCopyrightText: 2025 Bùi Gia Viện <shadichy@blisslabs.org>
 *   SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   Calamares is Free Software: see the License-Identifier above.
 *
 */

#ifndef DATAIMG_CONFIG_H
#define DATAIMG_CONFIG_H

#include "locale/TranslatableConfiguration.h"
#include "modulesystem/InstanceKey.h"
#include "utils/NamedEnum.h"

#include <QObject>
#include <QVariantMap>

#include <memory>

class LoaderQueue;

class Config : public QObject
{
    Q_OBJECT

    // Translations, of the module name (for sidebar) and above the list
    Q_PROPERTY( QString titleLabel READ titleLabel NOTIFY titleLabelChanged FINAL )
    Q_PROPERTY( QString checkboxLabel READ checkboxLabel NOTIFY checkboxLabelChanged FINAL )
    Q_PROPERTY( QString noticeLabel READ noticeLabel NOTIFY noticeLabelChanged FINAL )

public:
    Config( QObject* parent = nullptr );
    ~Config() override;

    void setConfigurationMap( const QVariantMap& configurationMap );

    QString titleLabel() const;
    QString checkboxLabel() const;
		QString noticeLabel() const;
		
    enum Unit
    {
        MiB,
        GiB,
    };
    Q_ENUM( Unit )
    using UnitsSet = QSet< Unit >;

    const UnitsSet& Units() const { return m_units; }
		QStringList unitNames() const { return m_unitNames; }
		void setUnitNames();

    Unit unit() const { return m_unit; }
    void setUnit( Unit unit );

    unsigned int sizeValueRatio() const { return m_unit == GiB ? 1024 : 1; }

    bool useMaximum() const { return m_useMaximum; }
    void setUseMaximum( bool useMaximum );

    unsigned int dataSize() const { return m_dataSize; }
    void setDataSize( unsigned int size ) { m_dataSize = size; }

		unsigned int maximumDataSize() const { return m_maximumDataSize; }
		void setMaximumDataSize( unsigned int size ) { m_maximumDataSize = size; }

    /** @brief Write the selected option lists to global storage
     *
     * Since the config doesn't know what module it is for,
     * pass in an instance key.
     */
    void finalizeGlobalStorage();

Q_SIGNALS:
    // void statusChanged( QString status );  ///< Something changed
    void titleLabelChanged( QString label );
    void checkboxLabelChanged( QString label );
		void noticeLabelChanged( QString label ); 
    void statusReady();  ///< Loading groups is complete

private Q_SLOTS:
    void retranslate();
    void loadingDone();

private:
    Calamares::Locale::TranslatedString* m_titleLabel = nullptr;
    Calamares::Locale::TranslatedString* m_checkboxLabel = nullptr;
		Calamares::Locale::TranslatedString* m_noticeLabel = nullptr;

    UnitsSet m_units;
		QStringList m_unitNames;

    bool m_useMaximum = true;

    unsigned int m_dataSize = 0;
		unsigned int m_maximumDataSize = -1;
    
		Unit m_unit = MiB;
};

#endif
