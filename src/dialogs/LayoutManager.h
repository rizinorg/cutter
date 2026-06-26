#ifndef LAYOUT_MANAGER_H
#define LAYOUT_MANAGER_H

#include "common/CutterLayout.h"

#include <QDialog>

#include <memory>

namespace Ui {
class LayoutManager;
}

/**
 * @brief Dialog for managing custom UI layouts
 */
class LayoutManager : public QDialog
{
    Q_OBJECT

public:
    LayoutManager(QMap<QString, Cutter::CutterLayout> &layouts, QWidget *parent);
    ~LayoutManager();

private:
    void refreshNameList(const QString &selection = "");
    void renameCurrentLayout();
    void deleteLayout();
    void updateButtons();
    std::unique_ptr<Ui::LayoutManager> ui;
    QMap<QString, Cutter::CutterLayout> &layouts;
};

#endif // LAYOUT_MANAGER_H
