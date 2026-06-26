#ifndef PLUGINSOPTIONSWIDGET_H
#define PLUGINSOPTIONSWIDGET_H

#include <QDialog>

class PreferencesDialog;

/**
 * @brief Contains configurable options related to plugins
 */
class PluginsOptionsWidget : public QDialog
{
    Q_OBJECT

public:
    explicit PluginsOptionsWidget(PreferencesDialog *dialog);
    ~PluginsOptionsWidget();
};

#endif // CUTTER_PLUGINSOPTIONSWIDGET_H
