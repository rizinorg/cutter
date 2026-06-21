#ifndef DEBUGOPTIONSWIDGET_H
#define DEBUGOPTIONSWIDGET_H

#include <QDialog>

#include <memory>

class PreferencesDialog;

namespace Ui {
class DebugOptionsWidget;
}

/**
 * @brief Contains configurable options related to debugging
 */
class DebugOptionsWidget : public QDialog
{
    Q_OBJECT

public:
    explicit DebugOptionsWidget(PreferencesDialog *dialog);
    ~DebugOptionsWidget();

private:
    std::unique_ptr<Ui::DebugOptionsWidget> ui;

    void debugOptionsChanged() const;
private slots:
    void updateDebugPlugin();
    void updateStackAddr();
    void updateStackSize();
    void onDebugPluginChanged(const QString &index);
};

#endif // DEBUGOPTIONSWIDGET_H
