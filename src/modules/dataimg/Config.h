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

    // using UnitsSet = QSet< Unit >;

    /// @brief Lookup table for the enums
    const NamedEnumTable< Unit >& unitNames();

    int unitRatio( Unit unit ) const;

    Unit unit() const { return m_unit; }
    void setUnit( Unit unit );

    bool useMaximum() const { return m_useMaximum; }
    void setUseMaximum( bool useMaximum );

    int dataSize() const { return m_dataSize; }
    void setDataSize( int size ) { m_dataSize = size; }

    int maximumDataSize() const { return m_maximumDataSize; }
    void setMaximumDataSize( int size ) { m_maximumDataSize = size; }

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

    bool m_useMaximum = true;

    int m_dataSize = 0;
    int m_maximumDataSize = (unsigned int)( -1 ) >> 1;

    Unit m_unit = Unit::MiB;
};

#endif
