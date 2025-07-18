/* === This file is part of Calamares - <https://calamares.io> ===
 *
 *   SPDX-FileCopyrightText: 2025 Bùi Gia Viện <shadichy@blisslabs.org>
 *   SPDX-License-Identifier: GPL-3.0-or-later
 *
 *   Calamares is Free Software: see the License-Identifier above.
 *
 */

#include "DataImgPage.h"
#include "ui_dataimg.h"

#include "utils/Logger.h"
#include "utils/Retranslator.h"
#include "utils/Yaml.h"

DataImgPage::DataImgPage( Config* c, QWidget* parent )
    : QWidget( parent )
    , m_config( c )
    , ui( new Ui::Page_DataImg )
{
    ui->setupUi( this );
    connect( c,
             &Config::titleLabelChanged,
             [ ui = this->ui ]( const QString title )
             {
                 ui->label->setVisible( !title.isEmpty() );
                 ui->label->setText( title );
             } );
    connect( c,
             &Config::checkboxLabelChanged,
             [ ui = this->ui ]( const QString title )
             {
                 ui->checkBoxMaximum->setVisible( !title.isEmpty() );
                 ui->checkBoxMaximum->setText( title );
             } );

    ui->checkBoxMaximum->setChecked( c->useMaximum() );
    ui->spinBoxDataSize->setMaximum( c->maximumDataSize() );
    ui->spinBoxDataSize->setValue( c->useMaximum() ? 0 : c->dataSize() / c->unitRatio( c->unit() ) );

    ui->spinBoxDataSize->setEnabled( !c->useMaximum() );
    ui->comboBoxUnit->setEnabled( !c->useMaximum() );

    ui->comboBoxUnit->clear();
    const auto& names = c->unitNames();
    for ( const auto& pair : names.table )
    {
        ui->comboBoxUnit->addItem( pair.first );
    }
    ui->comboBoxUnit->setCurrentIndex( c->unit() );

    connect( ui->checkBoxMaximum,
             &QCheckBox::toggled,
             [ ui = this->ui, c ]( bool checked )
             {
                 if ( checked )
                 {
                     ui->spinBoxDataSize->setValue( 0 );
                 }
                 else
                 {
                     ui->spinBoxDataSize->setValue( c->dataSize() / c->unitRatio( c->unit() ) );
                 }

                 ui->spinBoxDataSize->setEnabled( !checked );
                 ui->comboBoxUnit->setEnabled( !checked );

                 c->setUseMaximum( checked );
             } );

    connect( ui->spinBoxDataSize,
             &QSpinBox::valueChanged,
             [ c ]( int value ) { c->setDataSize( value * c->unitRatio( c->unit() ) ); } );
    connect( ui->comboBoxUnit,
             &QComboBox::currentTextChanged,
             [ ui = this->ui, c ]( const QString& text )
             {
                 bool ok = false;
                 auto it = c->unitNames().find( text, ok );
                 if ( ok )
                 {
                     c->setUnit( it );
                     ui->spinBoxDataSize->setValue( c->dataSize() / c->unitRatio( it ) );
                 }
             } );
}

DataImgPage::~DataImgPage() {}
