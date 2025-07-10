/* === This file is part of Calamares - <https://calamares.io> ===
 *
 *   SPDX-FileCopyrightText: 2017 Kyle Robbertze <kyle@aims.ac.za>
 *   SPDX-FileCopyrightText: 2017 2020, Adriaan de Groot <groot@kde.org>
 *   SPDX-FileCopyrightText: 2025 Bùi Gia Viện (BlissLabs) <shadichy@blisslabs.org>
 *   SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   Calamares is Free Software: see the License-Identifier above.
 *
 */

#include "OptionTreeItem.h"

#include <JobQueue.h>

#include "GlobalStorage.h"
#include "utils/Logger.h"
#include "utils/Variant.h"

// #include <QLineEdit>

/** @brief Should a option be selected, given its parent's state? */
static Qt::CheckState
parentCheckState( OptionTreeItem* parent )
{
    if ( parent )
    {
        // Avoid partially-checked .. a option can't be partial
        return parent->isDistinct() || parent->isSelected() == Qt::Unchecked ? Qt::Unchecked : Qt::Checked;
    }
    else
    {
        return Qt::Unchecked;
    }
}

// static Qt::CheckState initSelected(bool isSelected) {
//     if (!isSelected) {return;}
// };


OptionTreeItem::OptionTreeItem( const QString& optionName, OptionTreeItem* parent )
    : m_parentItem( parent )
    , m_name( optionName )
    , m_optionName( optionName )
    , m_selected( parentCheckState( parent ) )
    , m_description( optionName )
    , m_showReadOnly( parent ? parent->isImmutable() : false )
{
}

OptionTreeItem::OptionTreeItem( const QVariantMap& groupData, OptionTag&& parent )
    : m_parentItem( parent.parent )
    , m_name( Calamares::getString( groupData, "name" ) )
    , m_optionName( Calamares::getString( groupData, "name" ) )
    , m_isHidden( isHiddenException() || Calamares::getBool( groupData, "hidden", false ) )
    , m_selected( Calamares::getBool( groupData, "selected", false ) ? Qt::Checked : parentCheckState( parent.parent ) )
    , m_description( Calamares::getString( groupData, "description" ) )
    , m_editable( Calamares::getBool( groupData, "editable", false ) )
    , m_input( Calamares::getBool( groupData, "editable", false )
                   ? Calamares::getString( groupData, "default", "Value..." )
                   : "" )
    , m_showReadOnly( parent.parent ? parent.parent->isImmutable() : false )
{
}

OptionTreeItem::OptionTreeItem( const QVariantMap& groupData, GroupTag&& parent )
    : m_parentItem( parent.parent )
    , m_name( Calamares::getString( groupData, "name" ) )
    , m_optionName( Calamares::getString( groupData, "name" ) )
    , m_isHidden( isHiddenException() || Calamares::getBool( groupData, "hidden", false ) )
    , m_selected( parentCheckState( parent.parent ) )
    , m_description( Calamares::getString( groupData, "description" ) )
    , m_preScript( Calamares::getString( groupData, "pre-install" ) )
    , m_postScript( Calamares::getString( groupData, "post-install" ) )
    , m_source( Calamares::getString( groupData, "source" ) )
    , m_distinct( Calamares::getBool( groupData, "distinct", false ) )
    , m_isGroup( true )
    , m_showReadOnly( Calamares::getBool( groupData, "immutable", false ) )
    , m_showNoncheckable( Calamares::getBool( groupData, "noncheckable", false ) )
    , m_startExpanded( Calamares::getBool( groupData, "expanded", false ) )
{
}

OptionTreeItem::OptionTreeItem::OptionTreeItem()
    : m_parentItem( nullptr )
    , m_name( QStringLiteral( "<root>" ) )
    , m_selected( Qt::Unchecked )
    , m_isGroup( true )
{
}

OptionTreeItem::~OptionTreeItem()
{
    qDeleteAll( m_childItems );
}

void
OptionTreeItem::appendChild( OptionTreeItem* child )
{
    m_childItems.append( child );
}

OptionTreeItem*
OptionTreeItem::child( int row )
{
    return m_childItems.value( row );
}

int
OptionTreeItem::childCount() const
{
    return m_childItems.count();
}

int
OptionTreeItem::row() const
{
    if ( m_parentItem )
    {
        return m_parentItem->m_childItems.indexOf( const_cast< OptionTreeItem* >( this ) );
    }
    return 0;
}

QVariant
OptionTreeItem::data( int column ) const
{
    switch ( column )
    {
    case 0:
        // options have a optionname, groups don't
        return QVariant( isOption() ? optionName() : name() );
    case 1:
        // options often have a blank description
        return QVariant( description() );
    case 2:
        // if only option requires an input
        return QVariant( input() );
    default:
        return QVariant();
    }
}

OptionTreeItem*
OptionTreeItem::parentItem()
{
    return m_parentItem;
}

const OptionTreeItem*
OptionTreeItem::parentItem() const
{
    return m_parentItem;
}

bool
OptionTreeItem::hiddenSelected() const
{
    if ( !m_isHidden )
    {
        return m_selected != Qt::Unchecked;
    }

    if ( m_selected == Qt::Unchecked )
    {
        return false;
    }

    const OptionTreeItem* currentItem = parentItem();
    while ( currentItem != nullptr )
    {
        if ( !currentItem->isHidden() )
        {
            return currentItem->isSelected() != Qt::Unchecked;
        }
        currentItem = currentItem->parentItem();
    }

    /* Has no non-hidden parents */
    return m_selected != Qt::Unchecked;
}

bool
OptionTreeItem::isHiddenException() const
{
    Calamares::GlobalStorage* gs = Calamares::JobQueue::instance()->globalStorage();
    if ( m_description.contains( "DATA=" ) )
    {
        return gs->value( "partitions" ).toString().contains( "/data" );
    }
    else
    {
        return false;
    }
}

void
OptionTreeItem::setSelected( Qt::CheckState isSelected )
{
    if ( parentItem() == nullptr )
    {
        // This is the root, it is always checked so don't change state
        return;
    }

    m_selected = isSelected;

    // Look for suitable parent item which may change checked-state
    // when one of its children changes.
    OptionTreeItem* currentItem = parentItem();
    if ( currentItem->isDistinct() && isSelected == Qt::Checked )
    {
        currentItem->selectChildren( optionName() );
    }
    else
    {
        setChildrenSelected( isSelected );
    }
    while ( ( currentItem != nullptr ) && ( currentItem->childCount() == 0 ) )
    {
        currentItem = currentItem->parentItem();
    }
    if ( currentItem == nullptr )
    {
        // Reached the root .. don't bother
        return;
    }

    currentItem->updateSelected();
}

void
OptionTreeItem::updateSelected()
{
    // Figure out checked-state based on the children
    int childrenSelected = 0;
    int childrenPartiallySelected = 0;
    for ( int i = 0; i < childCount(); i++ )
    {
        if ( child( i )->isSelected() == Qt::Checked )
        {
            childrenSelected++;
        }
        if ( child( i )->isSelected() == Qt::PartiallyChecked )
        {
            childrenPartiallySelected++;
        }
    }
    if ( !childrenSelected && !childrenPartiallySelected )
    {
        setSelected( Qt::Unchecked );
    }
    else if ( isDistinct() || childrenSelected == childCount() )
    {
        setSelected( Qt::Checked );
    }
    else
    {
        setSelected( Qt::PartiallyChecked );
    }
}

void
OptionTreeItem::setChildrenSelected( Qt::CheckState isSelected )
{
    if ( isSelected == Qt::PartiallyChecked )
    {
        // Children are never root; don't need to use setSelected on them.
        return;
    }

    if ( isDistinct() && isSelected == Qt::Checked )
    {
        for ( auto child : m_childItems )
        {
            if ( child->isSelected() == Qt::Checked )
            {
                return;
            }
        }
        child( 0 )->m_selected = Qt::Checked;
        child( 0 )->setChildrenSelected( isSelected );
        return;
    }
    for ( auto child : m_childItems )
    {
        child->m_selected = isSelected;
        child->setChildrenSelected( isSelected );
    }
}

void
OptionTreeItem::selectChildren( QString optionName )
{
    if ( !isDistinct() )
    {
        return;
    }

    m_selected = Qt::Checked;
    for ( auto child : m_childItems )
    {
        if ( child->optionName().compare( optionName, Qt::CaseInsensitive ) == 0 )
        {
            child->m_selected = Qt::Checked;
            child->setChildrenSelected( Qt::Checked );
        }
        else
        {
            child->m_selected = Qt::Unchecked;
            child->setChildrenSelected( Qt::Unchecked );
        }
    }
}

void
OptionTreeItem::removeChild( int row )
{
    if ( 0 <= row && row < m_childItems.count() )
    {
        m_childItems.removeAt( row );
    }
    else
    {
        cWarning() << "Attempt to remove invalid child in removeChild() at row " << row;
    }
}

int
OptionTreeItem::type() const
{
    return QStandardItem::UserType;
}

QString
OptionTreeItem::toOperation() const
{
    return m_description + m_input;
}

bool
OptionTreeItem::operator==( const OptionTreeItem& rhs ) const
{
    if ( isGroup() != rhs.isGroup() )
    {
        // Different kinds
        return false;
    }

    if ( isGroup() )
    {
        return name() == rhs.name() && description() == rhs.description() && preScript() == rhs.preScript()
            && postScript() == rhs.postScript() && m_showReadOnly == rhs.m_showReadOnly
            && expandOnStart() == rhs.expandOnStart();
    }
    else
    {
        return optionName() == rhs.optionName();
    }
}
