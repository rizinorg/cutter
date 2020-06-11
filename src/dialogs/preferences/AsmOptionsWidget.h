
#ifndef ASMOPTIONSWIDGET_H
#define ASMOPTIONSWIDGET_H

#include <QDialog>
#include <QPushButton>
#include <memory>

#include "core/Cutter.h"

class PreferencesDialog;

namespace Ui {
class AsmOptionsWidget;
}

class AsmOptionsWidget : public QDialog
{
    Q_OBJECT

public:
    explicit AsmOptionsWidget(PreferencesDialog *dialog);
    ~AsmOptionsWidget();

private:
    std::unique_ptr<Ui::AsmOptionsWidget> ui;
    struct ConfigCheckbox {
        QCheckBox *checkBox;
        QString config;
    };
    QList<ConfigCheckbox> checkboxes;

    void triggerAsmOptionsChanged();

private slots:
    void resetToDefault();

    void updateAsmOptionsFromVars();

    void on_cmtcolSpinBox_valueChanged(int value);

    void on_syntaxComboBox_currentIndexChanged(int index);
    void on_caseComboBox_currentIndexChanged(int index);
    void on_asmTabsSpinBox_valueChanged(int value);
    void on_asmTabsOffSpinBox_valueChanged(int value);
    void on_nbytesSpinBox_valueChanged(int value);

    void on_bytesCheckBox_toggled(bool checked);
    void on_varsubCheckBox_toggled(bool checked);

    void on_buttonBox_clicked(QAbstractButton *button);

    void commentsComboBoxChanged(int index);
    void asmComboBoxChanged(int index);
    void offsetCheckBoxToggled(bool checked);
    void relOffCheckBoxToggled(bool checked);
    void checkboxEnabler(QCheckBox *checkbox, QString config);
};


#endif //ASMOPTIONSWIDGET_H
