#include "OptionsDialog.h"
#include "MainWindow.h"
#include "dialogs/NewFileDialog.h"
#include "dialogs/AboutDialog.h"
#include "ui_NewfileDialog.h"
#include "utils/Helpers.h"

#include <QFileDialog>
#include <QtGui>
#include <QMessageBox>
#include <QDir>
#include <QPushButton>
#include <QLineEdit>

const int NewFileDialog::MaxRecentFiles;

static QColor getColorFor(const QString &str, int pos)
{
    Q_UNUSED(str);

    QList<QColor> Colors;
    Colors << QColor(29, 188, 156); // Turquoise
    Colors << QColor(52, 152, 219); // Blue
    Colors << QColor(155, 89, 182); // Violet
    Colors << QColor(52, 73, 94);   // Grey
    Colors << QColor(231, 76, 60);  // Red
    Colors << QColor(243, 156, 17); // Orange

    return Colors[pos % 6];

}

static QIcon getIconFor(const QString &str, int pos)
{
    // Add to the icon list
    int w = 64;
    int h = 64;

    QPixmap pixmap(w, h);
    pixmap.fill(Qt::transparent);

    QPainter pixPaint(&pixmap);
    pixPaint.setPen(Qt::NoPen);
    pixPaint.setRenderHint(QPainter::Antialiasing);
    pixPaint.setBrush(QBrush(QBrush(getColorFor(str, pos))));
    pixPaint.drawEllipse(1, 1, w - 2, h - 2);
    pixPaint.setPen(Qt::white);
    pixPaint.setFont(QFont("Verdana", 24, 1));
    pixPaint.drawText(0, 0, w, h - 2, Qt::AlignCenter, QString(str).toUpper().mid(0, 2));
    return QIcon(pixmap);
}

NewFileDialog::NewFileDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewFileDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
    setAcceptDrops(true);
    ui->recentsListWidget->addAction(ui->actionRemove_item);
    ui->recentsListWidget->addAction(ui->actionClear_all);
    ui->projectsListWidget->addAction(ui->actionRemove_project);
    ui->logoSvgWidget->load(Config()->getLogoFile());

    fillRecentFilesList();
    fillIOPluginsList();

    // Set last clicked tab
    ui->tabWidget->setCurrentIndex(Config()->getNewFileLastClicked());

    ui->loadProjectButton->setEnabled(ui->projectsListWidget->currentItem() != nullptr);

    /* Set focus on the TextInput */
    ui->newFileEdit->setFocus();
}

NewFileDialog::~NewFileDialog() {}

void NewFileDialog::on_loadFileButton_clicked()
{
    loadFile(ui->newFileEdit->text());
}

void NewFileDialog::on_selectFileButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select file"), QDir::homePath());

    if (!fileName.isEmpty()) {
        ui->newFileEdit->setText(fileName);
        ui->loadFileButton->setFocus();
    }
}

void NewFileDialog::on_selectProjectsDirButton_clicked()
{
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::DirectoryOnly);

    auto currentDir = Config()->getDirProjects();

    if (currentDir.startsWith("~")) {
        currentDir = QDir::homePath() + currentDir.mid(1);
    }
    dialog.setDirectory(currentDir);

    dialog.setWindowTitle(tr("Select project path (dir.projects)"));

    if (!dialog.exec()) {
        return;
    }

    QString dir = dialog.selectedFiles().first();
    if (!dir.isEmpty()) {
        Config()->setDirProjects(dir);
        fillProjectsList();
    }
}

void NewFileDialog::on_loadProjectButton_clicked()
{
    QListWidgetItem *item = ui->projectsListWidget->currentItem();

    if (item == nullptr) {
        return;
    }

    loadProject(item->data(Qt::UserRole).toString());
}

void NewFileDialog::on_recentsListWidget_itemClicked(QListWidgetItem *item)
{
    QVariant data = item->data(Qt::UserRole);
    QString sitem = data.toString();
    ui->newFileEdit->setText(sitem);
}

void NewFileDialog::on_recentsListWidget_itemDoubleClicked(QListWidgetItem *item)
{
    loadFile(item->data(Qt::UserRole).toString());
}

void NewFileDialog::on_projectsListWidget_itemSelectionChanged()
{
    ui->loadProjectButton->setEnabled(ui->projectsListWidget->currentItem() != nullptr);
}

void NewFileDialog::on_projectsListWidget_itemDoubleClicked(QListWidgetItem *item)
{
    loadProject(item->data(Qt::UserRole).toString());
}

void NewFileDialog::on_aboutButton_clicked()
{
    AboutDialog *a = new AboutDialog(this);
    a->open();
}

void NewFileDialog::on_actionRemove_item_triggered()
{
    // Remove selected item from recents list
    QListWidgetItem *item = ui->recentsListWidget->currentItem();

    if (item == nullptr)
        return;

    QVariant data = item->data(Qt::UserRole);
    QString sitem = data.toString();

    QSettings settings;
    QStringList files = settings.value("recentFileList").toStringList();
    files.removeAll(sitem);
    settings.setValue("recentFileList", files);

    ui->recentsListWidget->takeItem(ui->recentsListWidget->currentRow());

    ui->newFileEdit->clear();
}

void NewFileDialog::on_actionClear_all_triggered()
{
    // Clear recent file list
    QSettings settings;
    QStringList files = settings.value("recentFileList").toStringList();
    files.clear();

    ui->recentsListWidget->clear();
    // TODO: if called from main window its ok, otherwise its not
    settings.setValue("recentFileList", files);
    ui->newFileEdit->clear();
}

void NewFileDialog::on_actionRemove_project_triggered()
{
    CutterCore *core = Core();

    QListWidgetItem *item = ui->projectsListWidget->currentItem();

    if (item == nullptr)
        return;

    QVariant data = item->data(Qt::UserRole);
    QString sitem = data.toString();

    // Confirmation box
    QMessageBox msgBox(this);
    msgBox.setText(tr("Delete the project \"%1\" from disk ?").arg(sitem));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    int ret = msgBox.exec();

    switch (ret) {
    case QMessageBox::Yes:
        core->deleteProject(sitem);
        ui->projectsListWidget->takeItem(ui->projectsListWidget->currentRow());
        break;
    case QMessageBox::No:
    default:
        break;
    }
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
    loadFile(event->mimeData()->urls().first().path());
}

bool NewFileDialog::fillRecentFilesList()
{
    // Fill list with recent opened files
    QSettings settings;

    QStringList files = settings.value("recentFileList").toStringList();

    QMutableListIterator<QString> it(files);
    int i = 0;
    while (it.hasNext()) {
        const QString &file = it.next();
        // Get stored files

        // Remove all but the file name
        const QString sep = QDir::separator();
        const QStringList name_list = file.split(sep);
        const QString name = name_list.last();

        // Get file info
        QFileInfo info(file);
        if (!info.exists()) {
            it.remove();
        } else {
            QListWidgetItem *item = new QListWidgetItem(
                getIconFor(name, i++),
                file + "\nCreated: " + info.created().toString() + "\nSize: " + qhelpers::formatBytecount(
                    info.size())
            );
            //":/img/icons/target.svg"), name );
            item->setData(Qt::UserRole, file);
            ui->recentsListWidget->addItem(item);
        }
    }

    // Removed files were deleted from the stringlist. Save it again.
    settings.setValue("recentFileList", files);

    return !files.isEmpty();
}

bool NewFileDialog::fillProjectsList()
{
    CutterCore *core = Core();

    auto currentDir = Config()->getDirProjects();

    ui->projectsDirEdit->setText(currentDir);

    QStringList projects = core->getProjectNames();
    projects.sort(Qt::CaseInsensitive);

    ui->projectsListWidget->clear();

    int i = 0;
    for (const QString &project : projects) {
        QString info = core->cmd("Pi " + project);

        QListWidgetItem *item = new QListWidgetItem(getIconFor(project, i++), project + "\n" + info);

        item->setData(Qt::UserRole, project);
        ui->projectsListWidget->addItem(item);
    }

    return !projects.isEmpty();
}

void NewFileDialog::fillIOPluginsList()
{
    ui->ioPlugin->clear();
    ui->ioPlugin->addItem("");
    ui->ioPlugin->setItemData(0, tr("Open a file with no extra treatment."), Qt::ToolTipRole);

    int index = 1;
    QList<RIOPluginDescription> ioPlugins = Core()->getRIOPluginDescriptions();
    for (RIOPluginDescription plugin : ioPlugins) {
        ui->ioPlugin->addItem(plugin.name);
        ui->ioPlugin->setItemData(index, plugin.description, Qt::ToolTipRole);
        index++;
    }
}

void NewFileDialog::loadFile(const QString &filename)
{
    if (ui->ioPlugin->currentIndex() == 0 && !Core()->tryFile(filename, false) && !ui->checkBox_FilelessOpen->isChecked()) {
        QMessageBox msgBox(this);
        msgBox.setText(tr("Select a new program or a previous one before continuing."));
        msgBox.exec();
        return;
    }

    // Add file to recent file list
    QSettings settings;
    QStringList files = settings.value("recentFileList").toStringList();
    files.removeAll(filename);
    files.prepend(filename);
    while (files.size() > MaxRecentFiles)
        files.removeLast();

    settings.setValue("recentFileList", files);

    // Close dialog and open MainWindow/OptionsDialog
    MainWindow *main = new MainWindow();
    QString ioFile = "";
    if (ui->ioPlugin->currentIndex()) {
        ioFile = ui->ioPlugin->currentText() + "://";
    }
    ioFile += filename;
    main->openNewFile(ioFile);

    close();
}

void NewFileDialog::loadProject(const QString &project)
{
    MainWindow *main = new MainWindow();
    main->openProject(project);

    close();
}

void NewFileDialog::on_tabWidget_currentChanged(int index)
{
    Config()->setNewFileLastClicked(index);
}
