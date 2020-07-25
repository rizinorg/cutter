#ifndef ANALOPTIONSWIDGET_H
#define ANALOPTIONSWIDGET_H

#include <QDialog>
#include <memory>

#include "core/Cutter.h"

class PreferencesDialog;

namespace Ui {
class AnalOptionsWidget;
}

class AnalOptionsWidget : public QDialog
{
    Q_OBJECT

public:
    explicit AnalOptionsWidget(PreferencesDialog *dialog);
    ~AnalOptionsWidget();

private:
    std::unique_ptr<Ui::AnalOptionsWidget> ui;
    struct ConfigCheckbox {
        QCheckBox *checkBox;
        QString config;
    };
    QList<ConfigCheckbox> checkboxes;

private slots:
    void updateAnalOptionsFromVars();
    void analyze();
    static void refreshAll();
    static void checkboxEnabler(QCheckBox *checkbox, const QString &config);
};

#endif // ANALOPTIONSWIDGET_H
