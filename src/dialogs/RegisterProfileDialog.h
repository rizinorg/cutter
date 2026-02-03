#ifndef REGISTERPROFILEDIALOG_H
#define REGISTERPROFILEDIALOG_H

#include <QDialog>
#include <QListWidgetItem>
#include <memory>

namespace Ui {
class RegisterProfileDialog;
}

enum class RegisterProfile {
    Default,
    Rizin,
    GDB,
};

/**
 * @brief Main dialog for selecting, loading, and exporting register profiles
 */
class RegisterProfileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegisterProfileDialog(QWidget *parent = nullptr);
    ~RegisterProfileDialog();

    /**
     * @brief Updates the path display in the UI
     * @param path The file path string
     */
    void setProfilePath(const QString &path);

    /**
     * @brief Internal storage for the profile text.
     * @param data The raw profile content.
     */
    void setProfileData(const QString &data);

    /**
     * @brief Retrieves the current profile content.
     * @return Raw profile string.
     */
    QString getProfileData() const;

    /**
     * @brief Retrieves the current file path from the UI.
     * @return Path string.
     */
    QString getProfilePath() const;

    /**
     * @brief Formats the path for configuration storage (e.g., "type::path").
     * @return Serialized string.
     */
    QString getSerializedProfilePath() const;

    /**
     * @brief Populates the recent profiles list.
     * @param profilePaths List of serialized profile strings.
     */
    void fillProfilePaths(const QStringList &profilePaths);

    /**
     * @brief Identifies which profile type is currently active.
     * @return Active RegisterProfile enum.
     */
    RegisterProfile getLoadedProfile() const;

private slots:
    void loadProfileBtnClicked();
    void loadGDBBtnClicked();
    void editProfileBtnClicked();
    void exportProfileBtnClicked();
    void setFileContents(const QString &filePath);
    void itemClicked(QListWidgetItem *item);

    /**
     * @brief Creates and shows the right-click context menu for the recent list.
     * @param pos The position where the user clicked.
     */
    void showContextMenu(const QPoint &pos);

    /**
     * @brief Removes the selected item from the list and persistent settings.
     */
    void removeItem();

    /**
     * @brief Clears all items from the list and persistent settings.
     */
    void clearAll();

private:
    std::unique_ptr<Ui::RegisterProfileDialog> ui;
    QString profileData;
    RegisterProfile loadedProfile = RegisterProfile::Default;

    void showWarning(const QString &filePath);
    void updateProfile(const QString &path, const QString &data);
};

#endif
