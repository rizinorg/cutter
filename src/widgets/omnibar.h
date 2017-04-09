#ifndef OMNIBAR_H
#define OMNIBAR_H

#include <QLineEdit>

class MainWindow;

class Omnibar : public QLineEdit
{
    Q_OBJECT
public:
    explicit Omnibar(MainWindow *main, QWidget *parent = 0);

    QStringList       commands;
    QStringList       flags;

    void fillFlags(QString flag);
    void clearFlags();
    QStringList getFlags();
    void setupCompleter();

private:
    MainWindow      *main;

private slots:
    void on_gotoEntry_returnPressed();

    void restoreCompleter();

signals:

public slots:
    void showCommands();
    void clearContents();
};

#endif // OMNIBAR_H
