#ifndef STRINGLISTDIALOG_H
#define STRINGLISTDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QStringList>

/**
 * @brief Dialog for modifying, adding and removing items from a string-list
 *
 * @see RizinConfigOptionsDelegate
 */
class StringListDialog : public QDialog
{
public:
    StringListDialog(const QStringList &initialList, QWidget *parent = nullptr);

    QStringList getStringList() const;

private:
    QListWidget *listView;
};

#endif
