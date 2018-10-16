
#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QTreeWidget>
#include <memory>

#include "Cutter.h"

namespace Ui {
class PreferencesDialog;
}

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Section { General, Disassembly };

    explicit PreferencesDialog(QWidget *parent = nullptr);
    ~PreferencesDialog();

    void showSection(Section section);

public slots:
    void changePage(QTreeWidgetItem *current, QTreeWidgetItem *previous);

private slots:
    void buttonBoxClicked(QAbstractButton *b);

private:
    std::unique_ptr<Ui::PreferencesDialog> ui;

    int showNotAppliedDialog() const;

    void apply(const QString &category, QWidget *currentWidget);
    void discard(const QString &category, QWidget *currentWidget);
};

#endif //PREFERENCESDIALOG_H
