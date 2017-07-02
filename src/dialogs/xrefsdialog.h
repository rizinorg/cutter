#ifndef XREFSDIALOG_H
#define XREFSDIALOG_H

#include "highlighter.h"
#include "iaitorcore.h"

#include <QDialog>
#include <QTreeWidgetItem>

class MainWindow;

namespace Ui
{
    class XrefsDialog;
}

class XrefsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit XrefsDialog(MainWindow *main, QWidget *parent = 0);
    ~XrefsDialog();

    void fillRefsForAddress(RVA addr, QString name, bool whole_function);

private slots:

    void on_fromTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);

    void on_toTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);

    QString normalizeAddr(QString addr);

    void highlightCurrentLine();
    void on_fromTreeWidget_itemSelectionChanged();

    void on_toTreeWidget_itemSelectionChanged();

private:
    RVA addr;
    QString func_name;

    Ui::XrefsDialog *ui;
    MainWindow *main;

    Highlighter      *highlighter;

    void fillRefs(QList<XrefDescription> refs, QList<XrefDescription> xrefs);
    void updateLabels(QString name);
    void updatePreview(RVA addr);

};

#endif // XREFSDIALOG_H
