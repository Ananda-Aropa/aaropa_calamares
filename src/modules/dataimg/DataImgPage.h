/* === This file is part of Calamares - <https://calamares.io> ===
 *
 *   SPDX-FileCopyrightText: 2025 Bùi Gia Viện <shadichy@blisslabs.org>
 *   SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   Calamares is Free Software: see the License-Identifier above.
 *
 */

#ifndef DATAIMGPAGE_H
#define DATAIMGPAGE_H

#include "Config.h"

#include "locale/TranslatableConfiguration.h"

#include <QString>
#include <QWidget>

#include <memory>

class QNetworkReply;

namespace Ui
{
class Page_DataImg;
}  // namespace Ui

class DataImgPage : public QWidget
{
    Q_OBJECT
public:
    DataImgPage( Config* config, QWidget* parent = nullptr );
    ~DataImgPage() override;

private:
    Config* m_config;
    Ui::Page_DataImg* ui;
};

#endif  // DATAIMGPAGE_H
