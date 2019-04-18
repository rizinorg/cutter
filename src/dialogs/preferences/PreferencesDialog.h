
#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include "core/Cutter.h"

#include <QDialog>

#include <memory>

class QTreeWidgetItem;

namespace Ui {
class PreferencesDialog;
}

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Section { Appearance, Disassembly };

    explicit PreferencesDialog(QWidget *parent = nullptr);
    ~PreferencesDialog();

    void showSection(Section section);

public slots:
    void changePage(QTreeWidgetItem *current, QTreeWidgetItem *previous);

private:
    std::unique_ptr<Ui::PreferencesDialog> ui;
    void chooseThemeIcons();
};

#endif //PREFERENCESDIALOG_H
