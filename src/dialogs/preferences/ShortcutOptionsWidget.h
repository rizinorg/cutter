#ifndef SHORTCUTOPTIONSWIDGET_H
#define SHORTCUTOPTIONSWIDGET_H

#include <QDialog>

#include <memory>

class QTreeWidgetItem;

namespace Ui {
class ShortcutOptionsWidget;
}

/**
 * @brief Lists default shortcuts in preferences
 */
class ShortcutOptionsWidget : public QDialog
{
    Q_OBJECT

public:
    explicit ShortcutOptionsWidget(QWidget *parent = nullptr);
    ~ShortcutOptionsWidget();

private:
    std::unique_ptr<Ui::ShortcutOptionsWidget> ui;

    void setupUiElements();
    void populateShortcutTree();
    QTreeWidgetItem *createCategoryItem(const QString &label);

private slots:
    void filterShortcutTree(const QString &input);
};

#endif // SHORTCUTOPTIONSWIDGET_H
