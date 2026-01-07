#ifndef NATIVEDEBUGDIALOG_H
#define NATIVEDEBUGDIALOG_H

#include <QDialog>
#include <memory>

namespace Ui {
class NativeDebugDialog;
}

enum class DebugConfigMethod { CommandLine, RzRunProfile, RzRunDirectives };

/**
 * @brief Dialog for connecting to native debug
 */
class NativeDebugDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NativeDebugDialog(QWidget *parent = nullptr);
    ~NativeDebugDialog();

    QString getArgs() const;
    void setArgs(const QString &args);

    /**
     * @brief Sets the path to an RzRun profile and loads its contents
     * @param profilePath Path to a .rz or .rrz file
     */
    void setProfilePath(const QString &profilePath);

    QString getProfilePath() const;
    QString getDirectives() const;

    /**
     * @brief Determines which configuration method should be used based on UI state
     * @return The selected DebugConfigMethod
     */
    DebugConfigMethod getSelectedMethod() const;

private slots:
    void profileCheckboxToggled(bool checked);
    void profilePathBtnClicked();
    void directiveListBtnClicked();

private:
    std::unique_ptr<Ui::NativeDebugDialog> ui;

    /**
     * @brief Reads a file from disk and populates the directive edit
     * @param filePath Path of the file to read
     */
    void setFileContents(const QString &filePath);

    void showWarning(const QString &filePath);
    QString profilePath;
    bool showWarningWhenChecked = false;
};

#endif // NATIVE_DEBUG_DIALOG
