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
    ui->spinBoxDataSize->setValue( c->dataSize() / c->sizeValueRatio() );
    ui->spinBoxDataSize->setMaximum( c->maximumDataSize() );

    ui->comboBoxUnit->clear();
    for ( const QString& unitName : c->unitNames() )
    {
        ui->comboBoxUnit->addItem( unitName );
    }
    ui->comboBoxUnit->setCurrentText( c->unitNames().at( c->unit() ) );

    connect( ui->checkBoxMaximum, &QCheckBox::toggled, this, [ c ]( bool checked ) { c->setUseMaximum( checked ); } );

    connect( ui->spinBoxDataSize,
             &QSpinBox::valueChanged,
             this,
             [ c ]( int value ) { c->setDataSize( value * c->sizeValueRatio() ); } );
    connect( ui->comboBoxUnit,
             &QComboBox::currentTextChanged,
             this,
             [ c ]( const QString& text )
             {
                 if ( text == "MiB" )
                 {
                     c->setUnit( Config::MiB );
                 }
                 else if ( text == "GiB" )
                 {
                     c->setUnit( Config::GiB );
                 }
             } );
}

DataImgPage::~DataImgPage() {}

