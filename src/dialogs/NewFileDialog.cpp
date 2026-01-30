#include "InitialOptionsDialog.h"
#include "core/MainWindow.h"
#include "dialogs/NewFileDialog.h"
#include "dialogs/AboutDialog.h"
#include "ui_NewFileDialog.h"
#include "common/Helpers.h"
#include "common/HighDpiPixmap.h"

#include <QFileDialog>
#include <QtGui>
#include <QMessageBox>
#include <QDir>
#include <QPushButton>
#include <QLineEdit>

const int NewFileDialog::MaxRecentFiles;

static QColor getColorFor(int pos)
{
    static const QList<QColor> colors = {
        QColor(29, 188, 156), // Turquoise
        QColor(52, 152, 219), // Blue
        QColor(155, 89, 182), // Violet
        QColor(52, 73, 94), // Grey
        QColor(231, 76, 60), // Red
        QColor(243, 156, 17) // Orange
    };
    return colors[pos % colors.size()];
}

static QIcon getIconFor(const QString &str, int pos)
{
    // Add to the icon list
    int w = 64;
    int h = 64;

    HighDpiPixmap pixmap(w, h);
    pixmap.fill(Qt::transparent);

    QPainter pixPaint(&pixmap);
    pixPaint.setPen(Qt::NoPen);
    pixPaint.setRenderHint(QPainter::Antialiasing);
    pixPaint.setBrush(getColorFor(pos));
    pixPaint.drawEllipse(1, 1, w - 2, h - 2);
    pixPaint.setPen(Qt::white);
    QFont font = Config()->getBaseFont();
    font.setBold(true);
    font.setPointSize(18);
    pixPaint.setFont(font);
    pixPaint.drawText(0, 0, w, h - 2, Qt::AlignCenter, QString(str).toUpper().mid(0, 2));
    return QIcon(pixmap);
}

NewFileDialog::NewFileDialog(MainWindow *main)
    : QDialog(nullptr), // no parent on purpose, using main causes weird positioning
      ui(new Ui::NewFileDialog),
      main(main)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
    setAcceptDrops(true);
    ui->recentsListWidget->addAction(ui->actionRemove_item);
    ui->recentsListWidget->addAction(ui->actionClear_all);
    ui->projectsListWidget->addAction(ui->actionRemove_project);
    ui->projectsListWidget->addAction(ui->actionClearProjects);
    ui->logoSvgWidget->load(Config()->getLogoFile());

    fillRecentFilesList();
    fillIOPluginsList();
    fillProjectsList();

    // Set last clicked tab
    ui->tabWidget->setCurrentIndex(Config()->getNewFileLastClicked());

    /* Set focus on the TextInput */
    ui->newFileEdit->setFocus();

    /* Install an event filter for shellcode text edit to enable ctrl+return event */
    ui->shellcodeText->installEventFilter(this);

    updateLoadProjectButton();
}

NewFileDialog::~NewFileDialog() {}

void NewFileDialog::on_loadFileButton_clicked()
{
    loadFile(ui->newFileEdit->text());
}

void NewFileDialog::on_selectFileButton_clicked()
{
    QString currentDir = Config()->getRecentFolder();
    const QString &fileName = QDir::toNativeSeparators(
            QFileDialog::getOpenFileName(this, tr("Select file"), currentDir));

    if (!fileName.isEmpty()) {
        ui->newFileEdit->setText(fileName);
        ui->loadFileButton->setFocus();
        Config()->setRecentFolder(QFileInfo(fileName).absolutePath());
    }
}

void NewFileDialog::on_selectProjectFileButton_clicked()
{
    const QString &fileName =
            QDir::toNativeSeparators(QFileDialog::getOpenFileName(this, tr("Open Project")));

    if (!fileName.isEmpty()) {
        ui->projectFileEdit->setText(fileName);
        ui->loadProjectButton->setFocus();
    }
}

void NewFileDialog::on_loadProjectButton_clicked()
{
    loadProject(ui->projectFileEdit->text());
}

void NewFileDialog::on_shellcodeButton_clicked()
{
    QString shellcode = ui->shellcodeText->toPlainText();
    QString extractedCode = "";
    static const QRegularExpression rx("([0-9a-f]{2})", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator i = rx.globalMatch(shellcode);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        extractedCode.append(match.captured(1));
    }
    int size = extractedCode.size() / 2;
    if (size > 0) {
        loadShellcode(extractedCode, size);
    }
}

void NewFileDialog::on_recentsListWidget_itemClicked(QListWidgetItem *item)
{
    updateSelectionFromItem(item);
}

void NewFileDialog::on_recentsListWidget_currentItemChanged(QListWidgetItem *current)
{
    updateSelectionFromItem(current);
}

void NewFileDialog::on_recentsListWidget_itemDoubleClicked(QListWidgetItem *item)
{
    const QStringList sitem = item->data(Qt::UserRole).toStringList();
    loadFile(sitem.at(1));
}

void NewFileDialog::on_projectFileEdit_textChanged()
{
    updateLoadProjectButton();
}

void NewFileDialog::on_projectsListWidget_itemClicked(QListWidgetItem *item)
{
    ui->projectFileEdit->setText(item->data(Qt::UserRole).toStringList().at(1));
}

void NewFileDialog::on_projectsListWidget_itemDoubleClicked(QListWidgetItem *item)
{
    loadProject(item->data(Qt::UserRole).toStringList().at(1));
}

void NewFileDialog::on_aboutButton_clicked()
{
    AboutDialog *a = new AboutDialog(this);
    a->setAttribute(Qt::WA_DeleteOnClose);
    a->open();
}

void NewFileDialog::on_actionRemove_item_triggered()
{
    // Remove selected item from recents list
    QListWidgetItem *item = ui->recentsListWidget->currentItem();
    if (item == nullptr) {
        return;
    }
    QStringList sitem = item->data(Qt::UserRole).toStringList();
    RecentFileEntry file = { sitem.at(0), sitem.at(1) };
    QList<RecentFileEntry> files = Config()->getRecentFiles();
    files.removeAll(file);
    Config()->setRecentFiles(files);
    ui->recentsListWidget->takeItem(ui->recentsListWidget->currentRow());
    ui->newFileEdit->clear();
}

void NewFileDialog::on_actionClear_all_triggered()
{
    Config()->setRecentFiles({});
    ui->recentsListWidget->clear();
    ui->newFileEdit->clear();
}

void NewFileDialog::on_actionRemove_project_triggered()
{
    QListWidgetItem *item = ui->projectsListWidget->currentItem();
    if (item == nullptr) {
        return;
    }
    QString sitem = item->data(Qt::UserRole).toString();
    RecentFileEntry project = { "", sitem };
    QList<RecentFileEntry> files = Config()->getRecentProjects();
    files.removeAll(project);
    Config()->setRecentProjects(files);
    ui->projectsListWidget->takeItem(ui->projectsListWidget->currentRow());
    ui->projectFileEdit->clear();
}

void NewFileDialog::on_actionClearProjects_triggered()
{
    Config()->setRecentProjects({});
    ui->projectsListWidget->clear();
    ui->projectFileEdit->clear();
}

void NewFileDialog::dragEnterEvent(QDragEnterEvent *event)
{
    // Accept drag & drop events only if they provide a URL
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void NewFileDialog::dropEvent(QDropEvent *event)
{
    // Accept drag & drop events only if they provide a URL
    if (event->mimeData()->urls().count() == 0) {
        qWarning() << "No URL in drop event, ignoring it.";
        return;
    }

    event->acceptProposedAction();
    loadFile(event->mimeData()->urls().first().toLocalFile());
}

/*
 * @brief Add the existing files from the list to the widget.
 * @return the list of files that actually exist
 */
static QList<RecentFileEntry> fillFilesList(QListWidget *widget,
                                            const QList<RecentFileEntry> &files)
{
    QList<RecentFileEntry> updatedFiles = files;

    QMutableListIterator<RecentFileEntry> it(updatedFiles);
    int i = 0;
    while (it.hasNext()) {
        // Get the file name
        const RecentFileEntry &file = it.next();
        const QString &path = QDir::toNativeSeparators(file.path);
        const QString &ioMode = file.ioMode;
        const QString homepath = QDir::homePath();
        const QString basename = path.section(QDir::separator(), -1);

        QString filenameHome = path;
        filenameHome.replace(homepath, "~");
        filenameHome.replace(basename, "");
        filenameHome.chop(1); // Remove last character that will be a path separator
        // Get file info
        QFileInfo info(path);
        if (!info.exists()) {
            it.remove();
        } else {
            // Format the text
            const QString text =
                    QString("%1\n%2\nSize: %3")
                            .arg(basename, filenameHome, qhelpers::formatBytecount(info.size()));
            QListWidgetItem *item = new QListWidgetItem(getIconFor(basename, i++), text);

            // add the item to the file list
            QStringList fullpath = { ioMode, path };
            item->setData(Qt::UserRole, fullpath);
            widget->addItem(item);
        }
    }
    return updatedFiles;
}

bool NewFileDialog::fillRecentFilesList()
{
    QList<RecentFileEntry> files = Config()->getRecentFiles();
    files = fillFilesList(ui->recentsListWidget, files);
    // Removed files were deleted from the stringlist. Save it again.
    Config()->setRecentFiles(files);
    return !files.isEmpty();
}

bool NewFileDialog::fillProjectsList()
{
    QList<RecentFileEntry> files = Config()->getRecentProjects();
    files = fillFilesList(ui->projectsListWidget, files);
    Config()->setRecentProjects(files);
    return !files.isEmpty();
}

void NewFileDialog::fillIOPluginsList()
{
    ui->ioPlugin->clear();
    ui->ioPlugin->addItem("file://");
    ui->ioPlugin->setItemData(0, tr("Open a file without additional options/settings."),
                              Qt::ToolTipRole);

    int index = 1;
    QList<RzIOPluginDescription> ioPlugins = Core()->getRIOPluginDescriptions();
    for (const RzIOPluginDescription &plugin : ioPlugins) {
        // Hide debug plugins
        if (plugin.permissions.contains('d')) {
            continue;
        }
        const auto &uris = plugin.uris;
        for (const auto &uri : uris) {
            if (uri == "file://") {
                continue;
            }
            ui->ioPlugin->addItem(uri);
            ui->ioPlugin->setItemData(index, plugin.description, Qt::ToolTipRole);
            index++;
        }
    }
}

void NewFileDialog::updateLoadProjectButton()
{
    ui->loadProjectButton->setEnabled(!ui->projectFileEdit->text().trimmed().isEmpty());
}

void NewFileDialog::loadFile(const QString &filename)
{
    bool isFileless = ui->checkBox_FilelessOpen->isChecked();
    QString ioFile;
    if (!isFileless) {
        const QString &nativeFn = QDir::toNativeSeparators(filename);
        if (ui->ioPlugin->currentIndex() == 0 && !Core()->tryFile(nativeFn, false)) {
            QMessageBox msgBox(this);
            msgBox.setText(tr("Select a new program or a previous one before continuing."));
            msgBox.exec();
            return;
        }

        const QString ioMode = ui->ioPlugin->currentText();
        ioFile = ioMode + nativeFn;

        // Add file to recent file list
        RecentFileEntry file = { ioMode, nativeFn };
        QList<RecentFileEntry> files = Config()->getRecentFiles();
        files.removeAll(file);
        files.prepend(file);
        while (files.size() > MaxRecentFiles)
            files.removeLast();
        Config()->setRecentFiles(files);
    }

    // Close dialog and open MainWindow/InitialOptionsDialog
    InitialOptions options;
    options.filename = ioFile;
    main->openNewFile(options, isFileless);

    close();
}

void NewFileDialog::loadProject(const QString &project)
{
    MainWindow *main = new MainWindow();
    if (!main->openProject(project)) {
        return;
    }
    close();
}

void NewFileDialog::loadShellcode(const QString &shellcode, const int size)
{
    MainWindow *main = new MainWindow();
    InitialOptions options;
    options.filename = QString("malloc://%1").arg(size);
    options.shellcode = shellcode;
    main->openNewFile(options);
    close();
}

void NewFileDialog::on_tabWidget_currentChanged(int index)
{
    Config()->setNewFileLastClicked(index);
}

bool NewFileDialog::eventFilter(QObject * /*obj*/, QEvent *event)
{
    QString shellcode = ui->shellcodeText->toPlainText();
    QString extractedCode = "";
    static const QRegularExpression rx("([0-9a-f]{2})", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator i = rx.globalMatch(shellcode);
    int size = 0;

    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

        // Confirm comment by pressing Ctrl/Cmd+Return
        if ((keyEvent->modifiers() & Qt::ControlModifier)
            && ((keyEvent->key() == Qt::Key_Enter) || (keyEvent->key() == Qt::Key_Return))) {
            while (i.hasNext()) {
                QRegularExpressionMatch match = i.next();
                extractedCode.append(match.captured(1));
            }
            size = extractedCode.size() / 2;
            if (size > 0) {
                loadShellcode(extractedCode, size);
            }
            return true;
        }
    }

    return false;
}

void NewFileDialog::updateSelectionFromItem(QListWidgetItem *item)
{
    if (!item) {
        return;
    }
    QStringList sitem = item->data(Qt::UserRole).toStringList();
    ui->ioPlugin->setCurrentIndex(ui->ioPlugin->findText(sitem.at(0)));
    ui->newFileEdit->setText(sitem.at(1));
}
