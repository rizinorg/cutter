#ifndef MAPFILEDIALOG_H
#define MAPFILEDIALOG_H

#include <QDialog>

#include <memory>

namespace Ui {
class MapFileDialog;
}

/**
 * @brief Dialog for mapping an external file into the current IO session at a specific memory
 * address
 */
class MapFileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MapFileDialog(QWidget *parent = nullptr);
    ~MapFileDialog();

private slots:
    void onSelectFileButtonClicked();
    void onButtonBoxAccepted();
    void onButtonBoxRejected();

private:
    std::unique_ptr<Ui::MapFileDialog> ui;
};

#endif // MAPFILEDIALOG_H
