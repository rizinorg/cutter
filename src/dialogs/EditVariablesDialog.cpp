#include "EditVariablesDialog.h"
#include "ui_EditVariablesDialog.h"

#include <QMetaType>
#include <QComboBox>
#include <QMetaType>


EditVariablesDialog::EditVariablesDialog(RVA offset, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditVariablesDialog)
{
    ui->setupUi(this);
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(applyFields()));
    connect(ui->dropdownLocalVars, SIGNAL(currentIndexChanged(int)), SLOT(updateFields()));

    QString fcnName = Core()->cmd(QString("afn @ %1").arg(offset)).trimmed();
    setWindowTitle(tr("Set Variable Types for Function: %1").arg(fcnName));

    variables = Core()->getVariables(offset);
    for (const VariableDescription &var : variables) {
        ui->dropdownLocalVars->addItem(var.name, QVariant::fromValue(var));
    }

    populateTypesComboBox();
    updateFields();
}

EditVariablesDialog::~EditVariablesDialog()
{
    delete ui;
}

void EditVariablesDialog::applyFields()
{
    VariableDescription desc = ui->dropdownLocalVars->currentData().value<VariableDescription>();

    Core()->cmdRaw(QString("afvt %1 %2").arg(desc.name).arg(ui->typeComboBox->currentText()));

    QString newName = ui->nameEdit->text().replace(QLatin1Char(' '), QLatin1Char('_'));
    if (newName != desc.name) {
        Core()->cmdRaw(QString("afvn %1 %2").arg(newName).arg(desc.name));
    }

    // Refresh the views to reflect the changes to vars
    emit Core()->refreshCodeViews();
}

void EditVariablesDialog::updateFields()
{
    VariableDescription desc = ui->dropdownLocalVars->currentData().value<VariableDescription>();
    ui->nameEdit->setText(desc.name);
    ui->typeComboBox->setCurrentText(desc.type);
}


void EditVariablesDialog::populateTypesComboBox()
{
    //gets all loaded types, structures and enums and puts them in a list

    QStringList userStructures;
    QStringList userEnumerations;
    QList<TypeDescription> primitiveTypesTypeList;

    userStructures = Core()->cmdList("ts");
    ui->typeComboBox->addItems(userStructures);
    ui->typeComboBox->insertSeparator(ui->typeComboBox->count());

    primitiveTypesTypeList = Core()->getAllPrimitiveTypes();

    for (const TypeDescription &thisType : primitiveTypesTypeList) {
        ui->typeComboBox->addItem(thisType.type);
    }

    ui->typeComboBox->insertSeparator(ui->typeComboBox->count());

    userEnumerations = Core()->cmdList("te");
    ui->typeComboBox->addItems(userEnumerations);
}
