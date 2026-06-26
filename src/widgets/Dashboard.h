#ifndef DASHBOARD_H
#define DASHBOARD_H

#include "CutterDockWidget.h"

#include <QFormLayout>

#include <memory>

QT_BEGIN_NAMESPACE
QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QJsonObject)
QT_END_NAMESPACE

class MainWindow;

namespace Ui {
class Dashboard;
}

/**
 * @brief Dock widget that contains info about the loaded binary
 */
class Dashboard : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit Dashboard(MainWindow *main);
    ~Dashboard();

private slots:
    void updateContents();
    void onCertificateButtonClicked();
    void onVersioninfoButtonClicked();

private:
    std::unique_ptr<Ui::Dashboard> ui;
    QWidget *hashesWidget = nullptr;

    /**
     * @brief Set the text of a QLineEdit. If no text, then "N/A" is set.
     * @param textBox LineEdit to set @p text to
     * @param text The text to set to @p textBox
     */
    void setPlainText(QLineEdit *textBox, const QString &text);
    /**
     * @brief Setting boolean values of binary information in dashboard
     * @param RzBinInfo
     */
    void setRzBinInfo(const RzBinInfo *binInfo);
    /**
     * @brief Set the text of a QLineEdit as True, False
     * @param boolean value
     */
    QString setBoolText(bool value);
};

#endif // DASHBOARD_H
