#include "OptionsDialog.h"
#include "dialogs/CreatenewDialog.h"
#include "dialogs/NewfileDialog.h"
#include "ui_NewfileDialog.h"

#include <QFileDialog>
#include <QtGui>
#include <QMessageBox>
#include <QDir>

const int NewFileDialog::MaxRecentFiles;

static QColor getColorFor(const QString& str, int pos)
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

static QIcon getIconFor(const QString& str, int pos)
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

static QString formatBytecount(const long bytecount)
{
    const int exp = log(bytecount) / log(1000);
    constexpr char suffixes[] = {' ', 'k', 'M', 'G', 'T', 'P', 'E'};

    QString str;
    QTextStream stream(&str);
    stream << qSetRealNumberPrecision(3) << bytecount / pow(1000, exp)
           << ' ' << suffixes[exp] << 'B';
    return stream.readAll();
}

NewFileDialog::NewFileDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewFileDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
    ui->recentsList->addAction(ui->actionRemove_item);
    ui->recentsList->addAction(ui->actionClear_all);
    ui->recentsList->setIconSize(QSize(48, 48));

    // Fill list with recent opened files
    QSettings settings;

    QStringList files = settings.value("recentFileList").toStringList();

    QMutableListIterator<QString> it(files);
    int i = 0;
    while (it.hasNext())
    {
        const QString &file = it.next();
        // Get stored files

        // Remove all but the file name
        const QString sep = QDir::separator();
        const QStringList name_list = file.split(sep);
        const QString name = name_list.last();

        // Get file info
        QFileInfo info(file);
        if (!info.exists())
        {
            it.remove();
        }
        else
        {
            QListWidgetItem *item = new QListWidgetItem(
                getIconFor(name, i++),
                file + "\nCreated: " + info.created().toString() + "\nSize: " + formatBytecount(info.size())
            );
            //":/img/icons/target.svg"), name );
            item->setData(Qt::UserRole, file);
            ui->recentsList->addItem(item);
        }
    }
    ui->recentsList->setSortingEnabled(true);

    // Hide "create" button until the dialog works
    ui->createButton->hide();

    // Removes files were deleted from the stringlist. Save it again.
    settings.setValue("recentFileList", files);
}

NewFileDialog::~NewFileDialog()
{
    delete ui;
}

void NewFileDialog::on_loadFileButton_clicked()
{
    // Check that there is a file selected
    QString fname = ui->newFileEdit->text();
    QFileInfo checkfile(fname);
    if (!checkfile.exists() || !checkfile.isFile())
    {
        QMessageBox msgBox(this);
        msgBox.setText(tr("Select a new program or a previous one\nbefore continue"));
        msgBox.exec();
    }
    else
    {
        // Add file to recent file list
        QSettings settings;
        QStringList files = settings.value("recentFileList").toStringList();
        files.removeAll(fname);
        files.prepend(fname);
        while (files.size() > MaxRecentFiles)
            files.removeLast();

        settings.setValue("recentFileList", files);

        close();

        // Close dialog and open MainWindow/OptionsDialog
        MainWindow *main = new MainWindow();
        main->openFile(fname);
        //OptionsDialog *o = new OptionsDialog(fname);
        //o->exec();
    }
}

void NewFileDialog::on_newFileButton_clicked()
{
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setViewMode(QFileDialog::Detail);
    dialog.setDirectory(QDir::home());

    QString fileName;
    fileName = dialog.getOpenFileName(this, tr("Select file"));

    if (!fileName.isEmpty())
    {
        ui->newFileEdit->setText(fileName);
        ui->loadFileButton->setFocus();
    }
}

void NewFileDialog::on_recentsList_itemClicked(QListWidgetItem *item)
{
    QVariant data = item->data(Qt::UserRole);
    QString sitem = data.toString();
    ui->newFileEdit->setText(sitem);
}

void NewFileDialog::on_recentsList_itemDoubleClicked(QListWidgetItem *item)
{
    // Get selected item to send to options dialog
    QVariant data = item->data(Qt::UserRole);
    QString sitem = data.toString();
    // Close dialog and open OptionsDialog
    close();

    MainWindow *main = new MainWindow();
    main->openFile(sitem);
    //OptionsDialog *o = new OptionsDialog(sitem);
    //o->exec();
}

void NewFileDialog::on_cancelButton_clicked()
{
    close();
}

void NewFileDialog::on_actionRemove_item_triggered()
{
    // Remove selected item from recents list
    QListWidgetItem *item = ui->recentsList->currentItem();

    QVariant data = item->data(Qt::UserRole);
    QString sitem = data.toString();

    QSettings settings;
    QStringList files = settings.value("recentFileList").toStringList();
    files.removeAll(sitem);
    settings.setValue("recentFileList", files);

    ui->recentsList->takeItem(ui->recentsList->currentRow());

    ui->newFileEdit->clear();
}

void NewFileDialog::on_createButton_clicked()
{
    // Close dialog and open create new file dialog
    close();
    createNewDialog *n = new createNewDialog(nullptr);
    n->exec();
}

void NewFileDialog::on_actionClear_all_triggered()
{
    // Clear recent file list
    QSettings settings;
    QStringList files = settings.value("recentFileList").toStringList();
    files.clear();

    ui->recentsList->clear();
    // TODO: if called from main window its ok, otherwise its not
    settings.setValue("recentFileList", files);
    ui->newFileEdit->clear();
}
