/* === This file is part of Calamares - <https://calamares.io> ===
 *
 *   SPDX-FileCopyrightText: 2017 Kyle Robbertze <kyle@aims.ac.za>
 *   SPDX-FileCopyrightText: 2017 Adriaan de Groot <groot@kde.org>
 *   SPDX-FileCopyrightText: 2023 Vladislav Nepogodin <nepogodin.vlad@gmail.com>
 *   SPDX-FileCopyrightText: 2025 Bùi Gia Viện (BlissLabs) <shadichy@blisslabs.org>
 *   SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   Calamares is Free Software: see the License-Identifier above.
 *
 */

#ifndef OPTIONMODEL_H
#define OPTIONMODEL_H

#include "OptionTreeItem.h"

#include <functional>

#include <QAbstractItemModel>
#include <QObject>
#include <QString>

namespace YAML
{
class Node;
}  // namespace YAML

class OptionModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    // Names for columns (unused in the code)
    static constexpr const int NameColumn = 0;
    static constexpr const int DescriptionColumn = 1;
    static constexpr const int InputColumn = 2;

    /* The only interesting roles are DisplayRole (with text depending
     * on the column, and MetaExpandRole which tells if an index
     * should be initially expanded.
     */
    static constexpr const int MetaExpandRole = Qt::UserRole + 1;

    explicit OptionModel( QObject* parent = nullptr );
    ~OptionModel() override;

    void setupModelData( const QVariantList& l );

    QVariant data( const QModelIndex& index, int role ) const override;
    bool setData( const QModelIndex& index, const QVariant& value, int role = Qt::DisplayRole ) override;
    Qt::ItemFlags flags( const QModelIndex& index ) const override;

    QModelIndex index( int row, int column, const QModelIndex& parent = QModelIndex() ) const override;
    QModelIndex parent( const QModelIndex& index ) const override;

    QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const override;
    int rowCount( const QModelIndex& parent = QModelIndex() ) const override;
    int columnCount( const QModelIndex& parent = QModelIndex() ) const override;

    /** @brief Sets the checked flag on matching groups in the tree
     *
     * Recursively traverses the tree pointed to by m_rootItem and
     * checks if a group name matches any of the items in @p selectNames.
     * If a match is found, set check the box for that group and it's children.
     *
     * Individual options will not be matched.
     *
     */
    void setSelections( const QStringList& selectNames );

    OptionTreeItem::List getOptions() const;
    OptionTreeItem::List getItemOptions( OptionTreeItem* item ) const;

    QStringList getOptionNames( OptionTreeItem* item ) const;
    QStringList getOptionNames( const OptionTreeItem::List& itemList ) const;

    /** @brief Appends groups to the tree
     *
     * Uses the data from @p groupList to add elements to the
     * existing tree that m_rootItem points to.  If m_rootItem
     * is not valid, it does nothing
     *
     * Before adding anything to the model, it ensures that there
     * is no existing data from the same source.  If there is, that
     * data is pruned first
     *
     */
    void appendModelData( const QVariantList& groupList );

    void setUpdateNextCall( std::function<void(bool)> fn );

private:
    friend class ItemTests;

    void setupModelData( const QVariantList& l, OptionTreeItem* parent );

    std::function<void(bool)> m_nextUpdateCall{};

    OptionTreeItem* m_rootItem = nullptr;
};

#endif  // OPTIONMODEL_H
