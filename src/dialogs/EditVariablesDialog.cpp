#include "EditVariablesDialog.h"
#include "ui_EditVariablesDialog.h"

#include <QMetaType>
#include <QComboBox>
#include <QMetaType>
#include <QPushButton>


EditVariablesDialog::EditVariablesDialog(RVA offset, QString initialVar, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditVariablesDialog)
{
    ui->setupUi(this);
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(applyFields()));
    connect(ui->dropdownLocalVars, SIGNAL(currentIndexChanged(int)), SLOT(updateFields()));

    QString fcnName = Core()->cmd(QString("afn @ %1").arg(offset)).trimmed();
    setWindowTitle(tr("Set Variable Types for Function: %1").arg(fcnName));

    variables = Core()->getVariables(offset);
    int currentItemIndex = -1;
    int index = 0;
    for (const VariableDescription &var : variables) {
        ui->dropdownLocalVars->addItem(var.name, QVariant::fromValue(var));
        if (var.name == initialVar) {
            currentItemIndex = index;
        }
        index++;
    }
    ui->dropdownLocalVars->setCurrentIndex(currentItemIndex);
    if (currentItemIndex != -1) {
        ui->nameEdit->setFocus();
    }

    populateTypesComboBox();
    updateFields();
}

EditVariablesDialog::~EditVariablesDialog()
{
    delete ui;
}

bool EditVariablesDialog::empty() const
{
    return ui->dropdownLocalVars->count() == 0;
}

void EditVariablesDialog::applyFields()
{
    if (ui->dropdownLocalVars->currentIndex() < 0) {
        // nothing was selected or list is empty
        return;
    }
    VariableDescription desc = ui->dropdownLocalVars->currentData().value<VariableDescription>();

    Core()->cmdRaw(QString("afvt %1 %2").arg(desc.name).arg(ui->typeComboBox->currentText()));

    // TODO Remove all those replace once r2 command parser is fixed
    QString newName = ui->nameEdit->text().replace(QLatin1Char(' '), QLatin1Char('_'))
            .replace(QLatin1Char('\\'), QLatin1Char('_'))
            .replace(QLatin1Char('/'), QLatin1Char('_'));
    if (newName != desc.name) {
        Core()->cmdRaw(QString("afvn %1 %2").arg(newName).arg(desc.name));
    }

    // Refresh the views to reflect the changes to vars
    emit Core()->refreshCodeViews();
}

void EditVariablesDialog::updateFields()
{
    bool hasSelection = ui->dropdownLocalVars->currentIndex() >= 0;
    auto okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    okButton->setEnabled(hasSelection);
    if (!hasSelection) {
        ui->nameEdit->clear();
        return;
    }
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
