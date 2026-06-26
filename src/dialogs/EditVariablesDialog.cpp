#include "EditVariablesDialog.h"

#include "Cutter.h"
#include "ui_EditVariablesDialog.h"

#include <QComboBox>
#include <QMetaType>
#include <QPushButton>

EditVariablesDialog::EditVariablesDialog(RVA offset, const QString &initialVar, QWidget *parent)
    : QDialog(parent), ui(new Ui::EditVariablesDialog), functionAddress(RVA_INVALID)
{
    ui->setupUi(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &EditVariablesDialog::applyFields);
    connect<void (QComboBox::*)(int)>(ui->dropdownLocalVars, &QComboBox::currentIndexChanged, this,
                                      &EditVariablesDialog::updateFields);

    auto core = Core()->lock();
    const RzAnalysisFunction *f = rz_analysis_get_function_at(core->analysis, offset);
    const QString fcnName = f->name;
    functionAddress = offset;
    setWindowTitle(tr("Edit Variables in Function: %1").arg(fcnName));

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

EditVariablesDialog::~EditVariablesDialog() {}

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
    const auto desc = ui->dropdownLocalVars->currentData().value<VariableDescription>();

    RzCoreLocked core(Core());
    auto *fcn = const_cast<RzAnalysisFunction *>(Core()->functionIn(core->offset));
    if (!fcn) {
        return;
    }

    RzAnalysisVar *v = rz_analysis_function_get_var_byname(fcn, desc.name.toUtf8().constData());
    if (!v) {
        return;
    }

    char *errorMsg;
    RzType *vType = rz_type_parse_string_single(
            rz_analysis_get_type_db(core->analysis)->parser,
            ui->typeComboBox->currentText().toUtf8().constData(), &errorMsg);
    if (!vType || errorMsg) {
        return;
    }
    rz_analysis_var_set_type(v, vType, true);

    // TODO Remove all those replace once rizin command parser is fixed
    const QString newName = ui->nameEdit->text()
                                    .replace(QLatin1Char(' '), QLatin1Char('_'))
                                    .replace(QLatin1Char('\\'), QLatin1Char('_'))
                                    .replace(QLatin1Char('/'), QLatin1Char('_'));
    if (newName != desc.name) {
        Core()->renameFunctionVariable(newName, desc.name, functionAddress);
    }

    // Refresh the views to reflect the changes to vars
    emit Core()->refreshCodeViews();
}

void EditVariablesDialog::updateFields()
{
    const bool hasSelection = ui->dropdownLocalVars->currentIndex() >= 0;
    auto okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    okButton->setEnabled(hasSelection);
    if (!hasSelection) {
        ui->nameEdit->clear();
        return;
    }
    const auto desc = ui->dropdownLocalVars->currentData().value<VariableDescription>();
    ui->nameEdit->setText(desc.name);
    ui->typeComboBox->setCurrentText(desc.type);
}

static void addTypeDescriptionsToComboBox(QComboBox *comboBox, const QList<TypeDescription> &list)
{
    for (const TypeDescription &thisType : list) {
        comboBox->addItem(thisType.type);
    }
    comboBox->insertSeparator(comboBox->count());
}

void EditVariablesDialog::populateTypesComboBox()
{
    addTypeDescriptionsToComboBox(ui->typeComboBox, Core()->getAllStructs());
    addTypeDescriptionsToComboBox(ui->typeComboBox, Core()->getAllPrimitiveTypes());
    addTypeDescriptionsToComboBox(ui->typeComboBox, Core()->getAllEnums());
    addTypeDescriptionsToComboBox(ui->typeComboBox, Core()->getAllTypedefs());
}
