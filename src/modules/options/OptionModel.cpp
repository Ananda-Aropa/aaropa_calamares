/* === This file is part of Calamares - <https://calamares.io> ===
 *
 *   SPDX-FileCopyrightText: 2017 Kyle Robbertze <kyle@aims.ac.za>
 *   SPDX-FileCopyrightText: 2017-2018 2020, Adriaan de Groot <groot@kde.org>
 *   SPDX-FileCopyrightText: 2023 Vladislav Nepogodin <nepogodin.vlad@gmail.com>
 *   SPDX-FileCopyrightText: 2025 Bùi Gia Viện (BlissLabs) <shadichy@blisslabs.org>
 *   SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   Calamares is Free Software: see the License-Identifier above.
 *
 */

#include "OptionModel.h"

#include "compat/Variant.h"
#include "utils/Logger.h"
#include "utils/Variant.h"
#include "utils/Yaml.h"
#include "widgets/TranslationFix.h"

#include <algorithm>
#include <array>
#include <string_view>
#include <utility>

#include <QMessageBox>

static bool gShowConfError {};

/// Recursive helper for setSelections()
static void
setSelections( const QStringList& selectNames, OptionTreeItem* item )
{
    for ( int i = 0; i < item->childCount(); i++ )
    {
        auto* child = item->child( i );
        setSelections( selectNames, child );
    }
    if ( item->isGroup() && selectNames.contains( item->name() ) )
    {
        item->setSelected( Qt::CheckState::Checked );
    }
}

/** @brief Collects all the "source" values from @p groupList
 *
 * Iterates over @p groupList and returns all nonempty "source"
 * values from the maps.
 *
 */
static QStringList
collectSources( const QVariantList& groupList )
{
    QStringList sources;
    for ( const QVariant& group : groupList )
    {
        QVariantMap groupMap = group.toMap();
        if ( !groupMap[ "source" ].toString().isEmpty() )
        {
            sources.append( groupMap[ "source" ].toString() );
        }
    }

    return sources;
}

QStringList
OptionModel::getOptionNames( OptionTreeItem* item ) const
{
    QStringList optionNames;
    if ( item->isOption() )  // option
    {
        optionNames << item->optionName();
    }
    else
    {
        const auto itemOptions = getItemOptions( item );
        for ( const auto& itemOption : itemOptions )
        {
            optionNames << getOptionNames( itemOption );
        }
    }

    return optionNames;
}

QStringList
OptionModel::getOptionNames( const OptionTreeItem::List& itemList ) const
{
    QStringList optionNames;
    for ( const auto& itemOption : itemList )
    {
        optionNames << getOptionNames( itemOption );
    }

    return optionNames;
}

void
OptionModel::setUpdateNextCall( std::function< void( bool ) > fn )
{
    m_nextUpdateCall = std::move( fn );
}

OptionModel::OptionModel( QObject* parent )
    : QAbstractItemModel( parent )
{
}

OptionModel::~OptionModel()
{
    delete m_rootItem;
}

QModelIndex
OptionModel::index( int row, int column, const QModelIndex& parent ) const
{
    if ( !m_rootItem || !hasIndex( row, column, parent ) )
    {
        return QModelIndex();
    }

    OptionTreeItem* parentItem;

    if ( !parent.isValid() )
    {
        parentItem = m_rootItem;
    }
    else
    {
        parentItem = static_cast< OptionTreeItem* >( parent.internalPointer() );
    }

    OptionTreeItem* childItem = parentItem->child( row );
    if ( childItem )
    {
        return createIndex( row, column, childItem );
    }
    else
    {
        return QModelIndex();
    }
}

QModelIndex
OptionModel::parent( const QModelIndex& index ) const
{
    if ( !m_rootItem || !index.isValid() )
    {
        return QModelIndex();
    }

    OptionTreeItem* child = static_cast< OptionTreeItem* >( index.internalPointer() );
    OptionTreeItem* parent = child->parentItem();

    if ( parent == m_rootItem )
    {
        return QModelIndex();
    }
    return createIndex( parent->row(), 0, parent );
}

int
OptionModel::rowCount( const QModelIndex& parent ) const
{
    if ( !m_rootItem || ( parent.column() > 0 ) )
    {
        return 0;
    }

    OptionTreeItem* parentItem;
    if ( !parent.isValid() )
    {
        parentItem = m_rootItem;
    }
    else
    {
        parentItem = static_cast< OptionTreeItem* >( parent.internalPointer() );
    }

    return parentItem->childCount();
}

int
OptionModel::columnCount( const QModelIndex& ) const
{
    return 3;
}

QVariant
OptionModel::data( const QModelIndex& index, int role ) const
{
    if ( !m_rootItem || !index.isValid() )
    {
        return QVariant();
    }

    OptionTreeItem* item = static_cast< OptionTreeItem* >( index.internalPointer() );
    switch ( role )
    {
    case Qt::CheckStateRole:
        return index.column() == NameColumn ? ( item->isImmutable() ? QVariant() : item->isSelected() ) : QVariant();
    case Qt::DisplayRole:
        return item->data( index.column() );
    case MetaExpandRole:
        return item->expandOnStart();
    case Qt::EditRole:
        return item->isEditable() ? item->data( index.column() ) : QVariant();
    default:
        return QVariant();
    }
}

bool
OptionModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
    if ( !m_rootItem )
    {
        return false;
    }

    if ( role == Qt::CheckStateRole && index.isValid() )
    {
        OptionTreeItem* item = static_cast< OptionTreeItem* >( index.internalPointer() );
        const auto checkedStateInfo = static_cast< Qt::CheckState >( value.toInt() );
        item->setSelected( checkedStateInfo );

        emit dataChanged( this->index( 0, 0 ),
                          index.sibling( index.column(), index.row() + 1 ),
                          QVector< int >( Qt::CheckStateRole ) );
    }
    else if ( role == Qt::EditRole && index.isValid() )
    {
        OptionTreeItem* item = static_cast< OptionTreeItem* >( index.internalPointer() );
        if ( !item->isEditable() )
        {
            return true;
        }

        const auto inputString = static_cast< QString >( value.toString() );
        item->setInput( inputString );

        emit dataChanged(
            this->index( 0, 0 ), index.sibling( index.column(), index.row() + 1 ), QVector< int >( Qt::EditRole ) );
        return true;
    }
    return true;
}

Qt::ItemFlags
OptionModel::flags( const QModelIndex& index ) const
{
    if ( !m_rootItem || !index.isValid() )
    {
        return Qt::ItemFlags();
    }
    if ( index.column() == NameColumn )
    {
        OptionTreeItem* item = static_cast< OptionTreeItem* >( index.internalPointer() );
        return item->isImmutable() || item->isNoncheckable()
            ? QAbstractItemModel::flags( index )
            : Qt::ItemIsUserCheckable | QAbstractItemModel::flags( index );
    }
    if ( index.column() == InputColumn )
    {
        return static_cast< OptionTreeItem* >( index.internalPointer() )->isEditable()
            ? Qt::ItemIsEditable | QAbstractItemModel::flags( index )
            : QAbstractItemModel::flags( index );
    }
    return QAbstractItemModel::flags( index );
}

QVariant
OptionModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if ( orientation == Qt::Horizontal && role == Qt::DisplayRole )
    {
        switch ( section )
        {
        case NameColumn:
            return tr( "Name" );
        case DescriptionColumn:
            return tr( "Description" );
        default:
            return tr( "Input (Optional)" );
        }
    }
    return QVariant();
}

void
OptionModel::setSelections( const QStringList& selectNames )
{
    if ( m_rootItem )
    {
        ::setSelections( selectNames, m_rootItem );
    }
}

OptionTreeItem::List
OptionModel::getOptions() const
{
    return m_rootItem ? getItemOptions( m_rootItem ) : OptionTreeItem::List();
}

OptionTreeItem::List
OptionModel::getItemOptions( OptionTreeItem* item ) const
{
    OptionTreeItem::List selectedOptions;
    for ( int i = 0; i < item->childCount(); i++ )
    {
        auto* child = item->child( i );
        if ( child->isSelected() == Qt::Unchecked )
        {
            continue;
        }

        if ( child->isOption() )  // option
        {
            selectedOptions.append( child );
        }
        else
        {
            selectedOptions.append( getItemOptions( child ) );
        }
    }
    return selectedOptions;
}

void
OptionModel::setupModelData( const QVariantList& groupList, OptionTreeItem* parent )
{
    for ( const auto& group : groupList )
    {
        QVariantMap groupMap = group.toMap();
        if ( groupMap.isEmpty() )
        {
            continue;
        }

        OptionTreeItem* item = new OptionTreeItem( groupMap, OptionTreeItem::GroupTag { parent } );
        if ( groupMap.contains( "selected" ) )
        {
            item->setSelected( Calamares::getBool( groupMap, "selected", false ) ? Qt::Checked : Qt::Unchecked );
        }
        if ( groupMap.contains( "options" ) )
        {
            for ( const auto& optionName : groupMap.value( "options" ).toList() )
            {
                if ( Calamares::typeOf( optionName ) == Calamares::StringVariantType )
                {
                    item->appendChild( new OptionTreeItem( optionName.toString(), item ) );
                }
                else
                {
                    QVariantMap m = optionName.toMap();
                    if ( !m.isEmpty() )
                    {
                        item->appendChild( new OptionTreeItem( m, OptionTreeItem::OptionTag { item } ) );
                    }
                }
            }
            if ( !item->childCount() )
            {
                cWarning() << "*options* under" << item->name() << "is empty.";
            }
            else
            {
                item->updateSelected();
            }
        }
        if ( groupMap.contains( "subgroups" ) )
        {
            bool haveWarned = false;
            const auto& subgroupValue = groupMap.value( "subgroups" );
            if ( !subgroupValue.canConvert< QVariantList >() )
            {
                cWarning() << "*subgroups* under" << item->name() << "is not a list.";
                haveWarned = true;
            }

            QVariantList subgroups = groupMap.value( "subgroups" ).toList();
            if ( !subgroups.isEmpty() )
            {
                setupModelData( subgroups, item );
                // The children might be checked while the parent isn't (yet).
                // Children are added to their parent (below) without affecting
                // the checked-state -- do it manually. Items with subgroups
                // but no children have only hidden children -- those get
                // handled specially.
                if ( item->childCount() > 0 )
                {
                    item->updateSelected();
                }
            }
            else
            {
                if ( !haveWarned )
                {
                    cWarning() << "*subgroups* list under" << item->name() << "is empty.";
                }
            }
        }
        item->setCheckable( true );
        parent->appendChild( item );
    }
}

void
OptionModel::setupModelData( const QVariantList& l )
{
    beginResetModel();
    delete m_rootItem;
    m_rootItem = new OptionTreeItem();
    setupModelData( l, m_rootItem );
    endResetModel();
}

void
OptionModel::appendModelData( const QVariantList& groupList )
{
    if ( m_rootItem )
    {
        beginResetModel();

        const QStringList sources = collectSources( groupList );

        if ( !sources.isEmpty() )
        {
            // Prune any existing data from the same source
            QList< int > removeList;
            for ( int i = 0; i < m_rootItem->childCount(); i++ )
            {
                OptionTreeItem* child = m_rootItem->child( i );
                if ( sources.contains( child->source() ) )
                {
                    removeList.insert( 0, i );
                }
            }
            for ( const int& item : qAsConst( removeList ) )
            {
                m_rootItem->removeChild( item );
            }
        }

        // Add the new data to the model
        setupModelData( groupList, m_rootItem );

        endResetModel();
    }
}
