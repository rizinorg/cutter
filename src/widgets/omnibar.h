#ifndef OMNIBAR_H
#define OMNIBAR_H

#include <QLineEdit>

class MainWindow;

class Omnibar : public QLineEdit
{
    Q_OBJECT
public:
    explicit Omnibar(MainWindow *main, QWidget *parent = 0);

    void fillFlags(QString flag);
    void clearFlags();
    QStringList getFlags();
    void setupCompleter();

private slots:
    void on_gotoEntry_returnPressed();

    void restoreCompleter();

public slots:
    void showCommands();
    void clearContents();

private:
    MainWindow      *main;
    QStringList       commands;
    QStringList       flags;
};

#endif // OMNIBAR_H
