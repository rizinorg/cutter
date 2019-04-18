#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <memory>
#include "CutterDockWidget.h"

QT_BEGIN_NAMESPACE
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QJsonObject)
QT_END_NAMESPACE

class MainWindow;

namespace Ui {
class Dashboard;
}

class Dashboard : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit Dashboard(MainWindow *main, QAction *action = nullptr);
    ~Dashboard();

private slots:
    void updateContents();
    void on_certificateButton_clicked();
    void on_versioninfoButton_clicked();

private:
    std::unique_ptr<Ui::Dashboard>   ui;
    void setPlainText(QLineEdit *textBox, const QString &text);
    void setBool(QLineEdit *textBox, const QJsonObject &jsonObject, const QString &key);
};

#endif // DASHBOARD_H
