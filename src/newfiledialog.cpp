#include "newfiledialog.h"
#include "optionsdialog.h"
#include "ui_newfiledialog.h"
#include "createnewdialog.h"

#include <QFileDialog>
#include <QtGui>
#include <QMessageBox>
#include <QDir>

const int NewFileDialog::MaxRecentFiles;

static QColor getColorFor(QString str, int pos) {
    QNOTUSED(str);

    QList<QColor> Colors;
    Colors << QColor(29, 188, 156); // Turquoise
    Colors << QColor(52, 152, 219); // Blue
    Colors << QColor(155, 89, 182); // Violet
    Colors << QColor(52, 73, 94);   // Grey
    Colors << QColor(231, 76, 60);  // Red
    Colors << QColor(243, 156, 17); // Orange

    return Colors[pos % 6];

}

static QIcon getIconFor(QString str, int pos) {
    // Add to the icon list
    int w = 64;
    int h = 64;

    QPixmap pixmap(w,h);
    pixmap.fill(Qt::transparent);

    QPainter pixPaint(&pixmap);
    pixPaint.setPen(Qt::NoPen);
    pixPaint.setRenderHint(QPainter::Antialiasing);
    pixPaint.setBrush(QBrush(QBrush(getColorFor(str, pos))));
    pixPaint.drawEllipse(1,1,w-2,h-2);
    pixPaint.setPen(Qt::white);
    pixPaint.setFont(QFont("Verdana",24,1));
    pixPaint.drawText(0, 0, w, h-2, Qt::AlignCenter, QString(str).toUpper().mid(0,2));
    return QIcon(pixmap);
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

    int numRecentFiles = qMin(files.size(), (int)MaxRecentFiles);

    for (int i = 0; i < numRecentFiles; ++i) {
        // Get stored files
        //QString text = tr("&%1 %2").arg(i + 1).arg(strippedName(files[i]));

        // Remove all but the file name
        QString sep = QDir::separator();
        QStringList name_list = files[i].split( sep );
        QString name = name_list.last();

        // Get file info
        QFileInfo info(files[i]);

        QListWidgetItem* item = new QListWidgetItem(getIconFor(name, i),files[i] + "\nCreated: " + info.created().toString() + "\nSize: " +  QString::number(info.size()));
        //":/new/prefix1/img/icons/target.png"), name );
        item->setData(Qt::UserRole, files[i]);
        ui->recentsList->addItem(item);
    }
    ui->recentsList->setSortingEnabled(true);

    // Hide "create" button until the dialog works
    ui->createButton->hide();
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
    if (!checkfile.exists() || !checkfile.isFile()) {
        QMessageBox msgBox;
        msgBox.setText("Select a new program or a previous one\nbefore continue");
        msgBox.exec();
    } else {
        // Add file to recent file list
        QSettings settings;
        QStringList files = settings.value("recentFileList").toStringList();
        files.removeAll(fname);
        files.prepend(fname);
        while (files.size() > MaxRecentFiles)
            files.removeLast();

        settings.setValue("recentFileList", files);
                
        // Close dialog and open OptionsDialog
        close();
        OptionsDialog* o = new OptionsDialog(fname);
        o->exec();
    }
}

void NewFileDialog::on_newFileButton_clicked()
{
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setViewMode(QFileDialog::Detail);
    dialog.setDirectory(QDir::home());

    QString fileName;
    fileName = dialog.getOpenFileName(this, "Select file");

    if (!fileName.isEmpty()) {
        ui->newFileEdit->setText(fileName);
        ui->loadFileButton->setFocus();        
    }
}

void NewFileDialog::on_recentsList_itemClicked(QListWidgetItem *item)
{
    QVariant data = item->data( Qt::UserRole );
    QString sitem = data.toString();
    ui->newFileEdit->setText(sitem);
}

void NewFileDialog::on_recentsList_itemDoubleClicked(QListWidgetItem *item)
{
    // Get selected item to send to options dialog
    QVariant data = item->data( Qt::UserRole );
    QString sitem = data.toString();
    // Close dialog and open OptionsDialog
    close();
    OptionsDialog* o = new OptionsDialog(sitem);
    o->exec();
}

void NewFileDialog::on_cancelButton_clicked()
{
    close();
}

void NewFileDialog::on_actionRemove_item_triggered()
{
    // Remove selected item from recents list
    QListWidgetItem* item = ui->recentsList->currentItem();

    QVariant data = item->data( Qt::UserRole );
    QString sitem = data.toString();

    QSettings settings;
    QStringList files = settings.value("recentFileList").toStringList();
    files.removeAll(sitem);
    settings.setValue("recentFileList", files);

    ui->recentsList->takeItem( ui->recentsList->currentRow() );

    ui->newFileEdit->clear();
}

void NewFileDialog::on_createButton_clicked()
{
    // Close dialog and open create new file dialog
    close();
    createNewDialog* n = new createNewDialog(this);
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
