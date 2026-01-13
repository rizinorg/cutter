#include "ProfileDirectivesDialog.h"
#include "ui_ProfileDirectivesDialog.h"

#include <QPushButton>

ProfileDirectivesDialog::ProfileDirectivesDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::ProfileDirectivesDialog)
{
    ui->setupUi(this);

    setWindowTitle(tr("Profile Directives"));

    model = new QStandardItemModel(this);
    model->setHorizontalHeaderLabels({ tr("Key"), tr("Description") });

    addDirective("arg[0-511]", tr("Set value for argument N passed to the program"));
    addDirective("aslr", tr("Enable or disable ASLR"));
    addDirective("bits", tr("Set 32 or 64 bit (if the architecture supports it)"));
    addDirective("chdir", tr("Change directory before executing the program"));
    addDirective("chroot", tr("Run the program in chroot. requires some previous setup"));
    addDirective("connect", tr("Connect stdin/stdout/stderr to a socket"));
    addDirective("core", tr("Set no limit the core file size"));
    addDirective("daemon",
                 tr("Set to false by default, otherwise it will run the program in background, "
                    "detached from the terminal"));
    addDirective("envfile", tr("Set a file with lines like `var=value` to be used as env"));
    addDirective("fork",
                 tr("Used with the listen option, allow to spawn a different process for each "
                    "connection. Ignored when debugging."));
    addDirective("input", tr("Set string to be passed to the program via stdin"));
    addDirective("libpath",
                 tr("Override path where the dynamic loader will look for shared libraries"));
    addDirective("listen", tr("Bound stdin/stdout/stderr to a listening socket"));
    addDirective("maxfd", tr("Set the maximum number of file descriptors"));
    addDirective("maxproc", tr("Set the maximum number of processes"));
    addDirective("maxstack", tr("Set the maximum size for the stack"));
    addDirective("nice", tr("Set the niceness level of the process"));
    addDirective("pid", tr("Set to true to print the PID of the process to stderr"));
    addDirective("pidfile", tr("Print the PID of the process to the specified file"));
    addDirective("preload", tr("Preload a library (not supported on Windows, only linux,osx,bsd)"));
    addDirective("program", tr("Path to program to be executed"));
    addDirective("pty", tr("Use a pty for connection over socket (with connect/listen)"));
    addDirective("runlib", tr("Path to the library to be executed"));
    addDirective("runlib.fcn", tr("Function name to call from runlib library"));
    addDirective("rzpreload",
                 tr("Preload with librz, kill -USR1 to get an rizin shell or -USRZ to spawn a "
                    "webserver in a thread"));
    addDirective("setegid", tr("Set effective process group id"));
    addDirective("setenv", tr("Set value for given environment variable (setenv=FOO=BAR)"));
    addDirective("seteuid", tr("Set effective process uid"));
    addDirective("setgid", tr("Set process group id"));
    addDirective("setuid", tr("Set process uid"));
    addDirective("sleep", tr("Sleep for the given amount of seconds"));
    addDirective("sterr", tr("Select file to replace stderr file descriptor"));
    addDirective("stdin", tr("Select file to read data from stdin"));
    addDirective(
            "stdio",
            tr("Select io stream to redirect data from/to. Redirect input/output to the process "
               "created by the command prefixed by '!' (stdio=!cmd)"));
    addDirective("stdout", tr("Select file to replace stdout file descriptor"));
    addDirective("system", tr("Execute the given command"));
    addDirective("timeout", tr("Set a timeout"));
    addDirective("timeoutsig",
                 tr("Signal to use when killing the child because the timeout happens"));
    addDirective("unsetenv", tr("Unset one environment variable"));

    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    proxyModel->setFilterKeyColumn(0);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    ui->treeView->setModel(proxyModel);
    ui->treeView->setSortingEnabled(true);
    ui->treeView->resizeColumnToContents(1);
    ui->treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(ui->filterEdit, &QLineEdit::textChanged, proxyModel,
            &QSortFilterProxyModel::setFilterFixedString);
    connect(ui->closeBtn, &QPushButton::clicked, this, &QDialog::close);
}

ProfileDirectivesDialog::~ProfileDirectivesDialog() {}

void ProfileDirectivesDialog::addDirective(const QString &key, const QString &description)
{
    QList<QStandardItem *> items;
    items << new QStandardItem(key);
    items << new QStandardItem(description);
    model->appendRow(items);
}
