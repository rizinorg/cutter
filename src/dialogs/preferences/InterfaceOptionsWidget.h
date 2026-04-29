#ifndef INTERFACEOPTIONSWIDGET_H
#define INTERFACEOPTIONSWIDGET_H

#include <QDialog>
#include <memory>

namespace Ui {
class InterfaceOptionsWidget;
};

class PreferencesDialog;

/**
 * @brief Widget containing options for interface related settings
 *
 * Uses groupboxes to seperate options available for different widgets/views
 * Groupboxes must be in alphabetical order according to their titles
 */
class InterfaceOptionsWidget : public QDialog
{
public:
    explicit InterfaceOptionsWidget(PreferencesDialog *dialog);
    ~InterfaceOptionsWidget();

signals:
    void interfaceOptionsChanged();

private:
    std::unique_ptr<Ui::InterfaceOptionsWidget> ui;

    void setUpFunctions();
    void setUpOmnibar();
    void setUpQuickFilter();
};

#endif // INTERFACEOPTIONSWIDGET_H
