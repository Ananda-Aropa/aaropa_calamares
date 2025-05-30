/* === This file is part of Calamares - <https://calamares.io> ===
 *
 *   SPDX-FileCopyrightText: 2016 Luca Giambonini <almack@chakraos.org>
 *   SPDX-FileCopyrightText: 2016 Lisa Vitolo     <shainer@chakraos.org>
 *   SPDX-FileCopyrightText: 2017 Kyle Robbertze  <krobbertze@gmail.com>
 *   SPDX-FileCopyrightText: 2017-2018 2020, Adriaan de Groot <groot@kde.org>
 *   SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   Calamares is Free Software: see the License-Identifier above.
 *
 */

#ifndef OPTIONSPAGE_H
#define OPTIONSPAGE_H

#include "Config.h"
#include "OptionModel.h"
#include "OptionTreeItem.h"

#include "locale/TranslatableConfiguration.h"

#include <QString>
#include <QWidget>

#include <memory>

class QNetworkReply;

namespace Ui
{
class Page_Options;
}  // namespace Ui

class OptionsPage : public QWidget
{
    Q_OBJECT
public:
    OptionsPage( Config* config, QWidget* parent = nullptr );
    ~OptionsPage() override;

    void onActivate();

    /** @brief Expand entries that should be pre-expanded.
     *
     * Follows the *expanded* key / the startExpanded field in the
     * group entries of the model. Call this after filling up the model.
     */
    void expandGroups();

private:
    Config* m_config;
    Ui::Page_Options* ui;
};

#endif  // OPTIONSPAGE_H
