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

#ifndef OPTIONTREEITEM_H
#define OPTIONTREEITEM_H

#include <QList>
#include <QStandardItem>
#include <QVariant>

class OptionTreeItem : public QStandardItem {
 public:
  using List = QList<OptionTreeItem*>;

  ///@brief A tag class to distinguish option-from-map from group-from-map
  struct OptionTag {
    OptionTreeItem* parent;
  };
  ///@brief A tag class to distinguish group-from-map from option-from-map
  struct GroupTag {
    OptionTreeItem* parent;
  };

  ///@brief A option (individual option)
  explicit OptionTreeItem(const QString& optionName,
                          OptionTreeItem* parent = nullptr);
  ///@brief A option (individual option with description)
  explicit OptionTreeItem( const QVariantMap& optionData, OptionTag&& parent );
  ///@brief A group (sub-items and sub-groups are ignored)
  explicit OptionTreeItem(const QVariantMap& groupData, GroupTag&& parent);
  ///@brief A root item, always selected, named "<root>"
  explicit OptionTreeItem();
  ~OptionTreeItem() override;

  void appendChild(OptionTreeItem* child);
  OptionTreeItem* child(int row);
  int childCount() const;
  QVariant data( int column ) const override;
  int row() const;

  OptionTreeItem* parentItem();
  const OptionTreeItem* parentItem() const;

  QString name() const { return m_name; }
  QString optionName() const { return m_optionName; }

  QString description() const { return m_description; }
  QString preScript() const { return m_preScript; }
  QString postScript() const { return m_postScript; }
  QString source() const { return m_source; }

  QString input() const { return m_input; }
  void setInput( QString input ) { m_input = input; };
  bool isEditable() const { return m_editable; }

  /** @brief Is this item a group-item?
   *
   * Groups have a (possibly empty) list of options, and a
   * (possibly empty) list of sub-groups, and can be marked.
   * Options, on the other hand, only have a meaningful
   * optionName() and selection status.
   *
   * Root is a group.
   */
  bool isGroup() const { return m_isGroup; }

  /** @brief Is this item hidden?
   *
   * Hidden items (generally only groups) are maintained separately,
   * not shown to the user, but do enter into the package-installation process.
   */
  bool isHidden() const { return m_isHidden; }

  /** @brief Is this hidden item, considered "selected"?
   *
   * This asserts when called on a non-hidden item.
   * A hidden item has its own selected state, but really
   * falls under the selectedness of the parent item.
   */
  bool hiddenSelected() const;

  /** @brief Is this item hidden, as an exception?
   *
   * This test visibility of an item, if it is considered
   * an exceptional option.
   */
  bool isHiddenException() const;

  /// @brief Is this item a single option?
  bool isOption() const { return !isGroup(); }

  /** @brief Is this group distinct?
   *
   * A distinct group is a radio selector that contains
   * a default item. It can only be selected if one of
   * the non-default items is selected
   */
  bool isDistinct() const { return m_distinct; }

  /** @brief Is this group expanded on start?
   *
   * This does not affect installation, only the UI. A group
   * that expands on start is shown expanded (not collapsed)
   * in the treeview when the page is loaded.
   */
  bool expandOnStart() const { return m_startExpanded; }

  /** @brief Is this an immutable item?
   *
   * Groups can be immutable: then you can't toggle the selected
   * state of any of its items.
   */
  bool isImmutable() const { return m_showReadOnly; }

  /** @brief Is this a non-checkable item?
   *
   * Groups can be non-checkable: then you can't toggle the selected
   * state of the group. This does not affect subgroups or options.
   */
  bool isNoncheckable() const { return m_showNoncheckable; }

  /** @brief is this item selected?
   *
   * Groups may be partially selected; options are only on or off.
   */
  Qt::CheckState isSelected() const { return m_selected; }

  /** @brief Turns this item into a variant for OptionOperations use
   *
   * For "plain" items, this is just the option name; items with
   * scripts return a map. See the option module for how it's interpreted.
   */
  QString toOperation() const;

  void setSelected(Qt::CheckState isSelected);
  void setChildrenSelected(Qt::CheckState isSelected);
  void selectChildren(QString optionName);

  void removeChild(int row);

  /** @brief Update selectedness based on the children's states
   *
   * This only makes sense for groups, which might have options
   * or subgroups; it checks only direct children.
   */
  void updateSelected();

  // QStandardItem methods
  int type() const override;

  /** @brief Are two items equal
   *
   * This **disregards** parent-item and the child-items, and compares
   * only the fields for the items-proper (name, .. expanded). Note
   * also that *isSelected()* is a run-time state, and is **not**
   * compared either.
   */
  bool operator==(const OptionTreeItem& rhs) const;
  bool operator!=(const OptionTreeItem& rhs) const { return !(*this == rhs); }

 private:
  OptionTreeItem* m_parentItem;
  List m_childItems;

  // An entry can be a option, or a group.
  QString m_name;
  QString m_description;
  QString m_optionName;
  QString m_input = "";
  Qt::CheckState m_selected = Qt::Unchecked;

  // These are only useful for groups
  QString m_preScript;
  QString m_postScript;
  QString m_source;
  bool m_distinct = false;
  bool m_editable = false;
  bool m_isGroup = false;
  bool m_isHidden = false;
  bool m_showReadOnly = false;
  bool m_showNoncheckable = false;
  bool m_startExpanded = false;
  qint64 m_range_max = 0;
  qint64 m_range_min = 0;
};

#endif  // OPTIONTREEITEM_H
